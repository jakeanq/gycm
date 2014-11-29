#include "ycmd.hpp"
#include <geanyplugin.h>
#include <stdio.h>
#include <limits>
#include <fstream>
#include <sstream>
#include <cstring>
#include <errno.h>
#include <neon/ne_socket.h>
#include <openssl/hmac.h>
#include <neon/ne_request.h>
#include <unistd.h>

Ycmd::Ycmd(GeanyData* _gd, GeanyFunctions* _gf) : geany(_gd), geany_functions(_gf), running(false) {
	// Generate HMAC secret
	for(size_t i=0; i<HMAC_SECRET_LENGTH; i++)
		hmac[i] = (char) (rand() % 256);
	
	if(ne_sock_init() != 0){
		msgwin_status_add("Neon initialization failed! This is very bad.");
	}
}

Ycmd::~Ycmd(){
	ne_sock_exit();
}

bool Ycmd::startServer(){
	if(running)
		return true;
	Json::Value ycmdsettings;
	std::string cf = confPath(geany,"ycmd.json");
	std::ifstream conf(cf.c_str());
	if(!conf.good()){
		msgwin_status_add("ycmd startup failed: Error opening file %s", cf.c_str());
		return false;
	}
	if(!doc.parse(conf,ycmdsettings)){
		msgwin_status_add("ycmd startup failed: %s", doc.getFormattedErrorMessages().c_str());
		return false;
	}
	gchar* hmac64 = g_base64_encode((guchar*) hmac,HMAC_SECRET_LENGTH);
	//printf("HMAC Secret: %s\n",hmac64);
	ycmdsettings["hmac_secret"] = std::string(hmac64);
	g_free(hmac64);
	
	const gchar* _tmpdir = g_get_tmp_dir();
	std::string tempdir(_tmpdir);
	gchar* tmpfname = g_build_filename(tempdir.c_str(),"ycmdXXXXXX",NULL);
	
	std::string jsonout = Json::FastWriter().write(ycmdsettings);
	
	int fd = mkstemp(tmpfname);
	if(fd == -1){
		msgwin_status_add("ycmd startup failed: Could not write config: %s (mkstemp)", strerror(errno));
		return false;
	}
	FILE* temp = fdopen(fd,"w");
	if(temp == NULL){
		msgwin_status_add("ycmd startup failed: Could not write config: %s (fdopen)", strerror(errno));
		return false;
	}
	if(fwrite(jsonout.c_str(),sizeof(char),jsonout.length(),temp) != jsonout.length()){
		msgwin_status_add("ycmd startup failed: Could not write config: %s (fwrite)", strerror(errno));
		return false;
	}
	
	
	
	port = getFreePort();
	if(port == -1){
		msgwin_status_add("ycmd startup failed: Could not get free port: %s", strerror(errno));
		return false;
	}
	
	gchar* cwd = g_get_current_dir();
	gchar py[] = "python";
	gchar iss[] = "--idle_suicide_seconds=10800";
	gchar * args[6] = { py, NULL, NULL, NULL, iss, NULL }; /* python; ycmd path; port, config; iss */ // TODO: Add log-level option
	// Port:
	std::stringstream _port; _port << "--port=" << port;
	args[2] = new char[_port.str().length()];
	strcpy(args[2],_port.str().c_str());
	// Options
	std::string optf = "--options_file=" + std::string(tmpfname);
	args[3] = new char[optf.length()];
	strcpy(args[3],optf.c_str());
	// ycmd path
	args[1] = g_build_filename(ycmdsettings["ycmd_path"].asString().c_str(),"ycmd",NULL);
	
	GError * err = NULL;
	bool ret = g_spawn_async_with_pipes(cwd,args,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&pid,NULL,&ycmd_stdout_fd, &ycmd_stderr_fd,&err);
	
	fclose(temp);
	delete[] args[2];
	delete[] args[3];
	free(cwd);
	g_free(args[1]);
	
	if(!ret){
		g_assert(err != NULL);
		msgwin_status_add("ycmd startup failed: %s", err->message);
		g_error_free(err);
		return false;
	}
	if(isAlive()){
		msgwin_status_add("ycmd started successfully");
	} else {
		msgwin_status_add("ycmd startup failed: ycmd server has gone AWOL!");
		return false;
	}
	
	http = ne_session_create("http", "127.0.0.1", port);
	sleep(1);
	running = true;
	return true;
}

bool Ycmd::isAlive(){
	return kill(pid,0) == 0;
}

void Ycmd::shutdown(){
	if(!running)
		return;
	msgwin_status_add("Shutting down ycmd");
	running = false;
	kill(pid,SIGTERM);
	ne_close_connection(http);
	ne_session_destroy(http);
}

void Ycmd::complete(GeanyDocument* _g){
	if(sci_get_length(_g->editor->sci) == 0) return;
	assertServer();
	Json::Value json;
	Json::Value extrad;
	extrad["event_name"] = "FileReadyToParse";
	jsonRequestBuild(_g,json,extrad);
	send(json,EVENT_HANDLER);
	send(json,CODE_COMPLETIONS_HANDLER);
}

void Ycmd::jsonRequestBuild(GeanyDocument * _g, Json::Value& request, Json::Value& extra_data){
	jsonRequestBuild(_g,request);
	Json::Value::Members x = extra_data.getMemberNames();
	for(auto it = x.begin(); it != x.end(); it++)
		request[*it] = extra_data[*it];
}

void Ycmd::jsonRequestBuild(GeanyDocument * _g, Json::Value& request){
	ScintillaObject * sci = _g->editor->sci;
	std::string fpath = _g->real_path?std::string(_g->real_path):"";
	//Json::Value request;
	request["line_num"] = sci_get_current_line(sci) + 1;
	request["column_num"] = sci_get_col_from_position(sci,sci_get_current_position(sci)) + 1;
	request["filepath"] = fpath;
	//request["file_data"][fpath]["filetypes"][0] = strToLower(_g->file_type->name);
	//gchar * document = sci_get_contents(sci,sci_get_length(sci));
	//request["file_data"][fpath]["contents"] = std::string(document);
	request["file_data"] = getUnsavedBuffers();
	
	std::cout << "Built request: " << Json::StyledWriter().write(request); // Debug! :D
}
int block_reader(void * userdata, const char * buf, size_t len){
	return ((Ycmd*)userdata)->handler(buf,len);
}

int Ycmd::handler(const char * buf, size_t len){ // TODO: Validate HMAC
	if(len != 0){
		size_t start = returned_data.size();
		returned_data.resize(start + len);
		memcpy(&(returned_data[start]),buf,len);
		return 0;
	}
	// We have a complete set of data
	
	//for(size_t i=0; i<returned_data.size(); i++){
	//	printf("%c",returned_data[i]);
	//}
	//printf("\n\n");
	
	//std::cout << "Handling response: " << returned_data;
	
	Json::Value returned;
	if(!doc.parse(returned_data,returned)){
		msgwin_status_add("Bad JSON from ycmd: %s", doc.getFormattedErrorMessages().c_str());
		returned_data = "";
		return 0;
	}
	
	returned_data = "";
	
	if(returned.isMember("exception")){
		msgwin_status_add("[ycmd] %s: %s", returned["exception"]["TYPE"].asCString(), returned["message"].asCString());
		#ifndef NDEBUG
		std::cout << returned.toStyledString();
		#endif
		return 0;
	}
	
	std::cout << returned.toStyledString();
	
	
	return 0; // Success! // was 0
}
#define HMAC_LENGTH (256/8)
gchar * Ycmd::b64HexHMAC(std::string& data)
{
	unsigned char * digest = HMAC(EVP_sha256(), hmac, HMAC_SECRET_LENGTH,(unsigned char *) data.c_str(),data.length(), NULL, NULL);
	
	char hex_str[]= "0123456789abcdef";
	unsigned char * digest_enc = new unsigned char[HMAC_LENGTH*2];

	for (int i = 0; i < HMAC_LENGTH; i++){
		digest_enc[i * 2 + 0] = hex_str[digest[i] >> 4  ];
		digest_enc[i * 2 + 1] = hex_str[digest[i] & 0x0F];
	}
	//free(digest); No idea why this isn't needed
	gchar * ret = g_base64_encode(digest_enc,HMAC_LENGTH*2);
	delete[] digest_enc;
	return ret;
}

void Ycmd::send(Json::Value& _json, std::string _handler){
	ne_request* req = ne_request_create(http,"POST",_handler.c_str());
	ne_add_request_header(req,"content-type","application/json");
	
	std::string json = Json::FastWriter().write(_json);
	
	gchar * digest_enc = b64HexHMAC(json);
//	printf("HMAC: %s\n", digest_enc);
	ne_add_request_header(req,"X-Ycm-Hmac",digest_enc);
	g_free(digest_enc);
	std::ofstream s("temp.file"); s << json; s.close();
	ne_set_request_body_buffer(req,json.c_str(),json.length());
	ne_add_response_body_reader(req,ne_accept_always,block_reader,this);
	if(ne_request_dispatch(req)){
		msgwin_status_add("HTTP request error: %s",ne_get_error(http));
	}
}

bool Ycmd::assertServer(){
	if(isAlive())
		return true;
	return restart();
}

bool Ycmd::restart(){
	shutdown();
	return startServer();
}

Json::Value Ycmd::getUnsavedBuffers(){
	guint i;
	Json::Value v;
	gchar * document;
	ScintillaObject * sci;
	std::string fpath;
	
	foreach_document(i){
		if(!documents[i]->changed)
			continue;
		fpath = documents[i]->real_path?std::string(documents[i]->real_path):"";
		
		sci = documents[i]->editor->sci;
		v[fpath]["filetypes"][0] = strToLower(documents[i]->file_type->name);
		document = sci_get_contents(sci,sci_get_length(sci));
		v[fpath]["contents"] = std::string(document);
		g_free(document);
	}
	return v;
}

void Ycmd::handleDocumentLoad(GObject*, GeanyDocument* doc){
	Json::Value json;
	Json::Value extrad;
	extrad["event_name"] = "FileReadyToParse";
	jsonRequestBuild(doc,json,extrad);
	send(json,EVENT_HANDLER);
}

void Ycmd::handleDocumentUnload(GObject*, GeanyDocument* doc){
	Json::Value json;
	Json::Value extrad;
	extrad["event_name"] = "BufferUnload";
	std::string fpath = doc->real_path?std::string(doc->real_path):"";
	extrad["unloaded_buffer"] = fpath;
	jsonRequestBuild(doc,json,extrad);
	send(json,EVENT_HANDLER);
}