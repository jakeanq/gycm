#include "utils.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <algorithm>

std::string confPath(GeanyData* geany, std::string name){
	char * t = g_build_filename(geany->app->configdir, "plugins", "gycm", name.c_str(), NULL);
	
	std::string x(t);
	g_free(t);
	
	return x;
}

bool fileExists(std::string name){
	return access(name.c_str(), F_OK) != -1;
}

std::string slurp(std::string fname){
	FILE* f = fopen(fname.c_str(), "r");
	
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);

	char* data = new char[size];

	rewind(f);
	fread(data, sizeof(char), size, f);

	std::string s(data,size);
	delete[] data;
	return s;
}

int getFreePort(){
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

std::string strToLower(std::string s){
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}