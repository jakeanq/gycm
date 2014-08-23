#include "ycmd.hpp"
#include <geanyplugin.h>
#include <stdio.h>
#include <limits>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

Ycmd::Ycmd(GeanyData* _gd, GeanyFunctions* _gf) : geany(_gd), geany_functions(_gf) {
	// Generate HMAC secret
	for(size_t i=0; i<HMAC_SECRET_LENGTH; i++)
		hmac[i] = (char) (rand() % 256);
}

Ycmd::~Ycmd(){
	
}
int Ycmd::getFreePort(){
	int sockfd;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0){
		return -1;
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = 0;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	if (bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(struct sockaddr_in)) < 0){
		return -1;
	}
	if(listen(sockfd,1) == -1) {
		return -1;
	}
	socklen_t len = sizeof(struct sockaddr_in);
	if(getsockname(sockfd, (struct sockaddr *) &serv_addr, &len) < 0){
		return -1;
	}
	close(sockfd);
	return ntohs(serv_addr.sin_port);
}
bool Ycmd::startServer(){
	Json::Value ycmdsettings;
	Json::Reader doc;
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
	
	GError * err = NULL;
	bool ret = g_spawn_async_with_pipes(cwd,args,NULL,G_SPAWN_SEARCH_PATH,NULL,NULL,&pid,NULL,NULL,NULL,&err);
	
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
		return true;
	} else {
		msgwin_status_add("ycmd startup failed: ycmd server has gone AWOL!");
		return false;
	}
}

bool Ycmd::isAlive(){
	return kill(pid,0) == 0;
}

void Ycmd::shutdown(){
	kill(pid,SIGTERM);
}