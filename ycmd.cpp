#include "ycmd.hpp"
#include <geanyplugin.h>
#include <stdio.h>
#include <limits>
#include <fstream>
#include <sstream>
#include <cstring>
#include <errno.h>
//#include <neon/ne_socket.h>
#include <openssl/hmac.h>
//#include <neon/ne_request.h>
#include <unistd.h>


Ycmd::Ycmd(GeanyData* _gd, GeanyFunctions* _gf) : geany(_gd), geany_functions(_gf), running(false) {
	// Generate HMAC secret
	for(size_t i=0; i<HMAC_SECRET_LENGTH; i++)
		hmac[i] = (char) (rand() % 256);
	gchar* hmac64 = g_base64_encode((guchar*) hmac,HMAC_SECRET_LENGTH);
}

Ycmd::~Ycmd(){
	g_free(hmac64);
}

bool Ycmd::startServer(){
	// Check if running already
	if(isAlive())
		return true;
	
	// Grab the settings file
	Json::Value ycmdsettings;
	Json::Reader doc;
	// TODO: Improve
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
	// END TODO
	
	// Add the HMAC to the settings
	//printf("HMAC Secret: %s\n",hmac64);
	ycmdsettings["hmac_secret"] = std::string(hmac64);
	std::string jsonout = Json::FastWriter().write(ycmdsettings);
	
	// Create a temp file to use for passing the settings to ycmd
	//const gchar* _tmpdir = g_get_tmp_dir();
	std::string tempdir(g_get_tmp_dir());
	gchar* tmpfname = g_build_filename(tempdir.c_str(),"ycmdXXXXXX",NULL);
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
	
	// Decide on a port to use
	port = getFreePort();
	if(port == -1){
		msgwin_status_add("ycmd startup failed: Could not get free port: %s", strerror(errno));
		return false;
	}
	
	// Compile ycmd arguments
	gchar* cwd = g_get_current_dir();
	gchar py[] = "python"; // TODO: Figure out if this is reliable; it probably isn't
	gchar iss[] = "--idle_suicide_seconds=10800";
	gchar * args[6] = { py, NULL, NULL, NULL, iss, NULL }; /* python; ycmd path; port, config; iss */
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
	
	// Launch ycmd!
	GError * err = NULL;
	bool ret = g_spawn_async_with_pipes(cwd,args,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&pid,NULL,&ycmd_stdout_fd, &ycmd_stderr_fd,&err);
	
	// Cleanup
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
	
	http.connect(Poco::Net::SocketAddress("127.0.0.1", port));
	sleep(1); // TODO: Actually check if server is ready
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
	if(kill(pid,SIGTERM)==0)
		msgwin_status_add("ycmd shut down successfully");
	else
		msgwin_status_add("oops, ycmd was already shut down!");
}

void Ycmd::complete(GeanyDocument* _g){
	if(sci_get_length(_g->editor->sci) == 0) return;
	assertServer();
	std::string json;
	jsonRequestBuild(_g,json);
	send(json);
}

void Ycmd::jsonRequestBuild(GeanyDocument * _g, std::string& result){
	ScintillaObject * sci = _g->editor->sci;
	std::string fpath = _g->real_path?std::string(_g->real_path):"";
	Json::Value request;
	request["line_num"] = sci_get_current_line(sci);
	request["column_num"] = sci_get_col_from_position(sci,sci_get_current_position(sci)) + 1;
	request["filepath"] = fpath;
	request["file_data"][fpath]["filetypes"][0] = std::string(_g->file_type->name);
	gchar * document = sci_get_contents(sci,sci_get_length(sci));
	request["file_data"][fpath]["contents"] = std::string(document);
	std::cout << Json::StyledWriter().write(request);
	result = Json::FastWriter().write(request);
}
int block_reader(void * userdata, const char * buf, size_t len){
	return ((Ycmd*)userdata)->handler(buf,len);
}

int Ycmd::handler(const char * buf, size_t len){
	if(len != 0){
		size_t start = returned_data.size();
		returned_data.resize(start + len);
		memcpy(&(returned_data[start]),buf,len);
		return 0;
	}
	// We have a complete set of data
	
	for(size_t i=0; i<returned_data.size(); i++){
		printf("%c",returned_data[i]);
	}
	printf("\n\n");
	return 0; // Success!
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

void Ycmd::send(std::string& json, std::string _handler){
	Poco::HTTPRequest req("POST",_handler);
	req.setContentType("application/json");
	
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