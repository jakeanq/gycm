#ifndef YCMD_HPP
#define YCMD_HPP

#include <geanyplugin.h>
#include <json/json.h>
#include <string>
#include "utils.hpp"
#include <neon/ne_session.h>

#define HMAC_SECRET_LENGTH 16

class Ycmd {
public:
	Ycmd(GeanyData*,GeanyFunctions*);
	~Ycmd();
	bool startServer();
	bool isAlive();
	void shutdown();
	void complete(GeanyDocument*);
	bool assertServer();
	bool restart();
	int handler(const char *, size_t);
private:
	gchar * b64HexHMAC(std::string& data);
	void jsonRequestBuild(GeanyDocument*, std::string&);
	void send(std::string&,std::string=std::string("/completions"));
	GeanyData* geany;
	GeanyFunctions* geany_functions;
	char hmac[HMAC_SECRET_LENGTH];
	int ycmd_stderr_fd, ycmd_stdout_fd;
	pid_t pid;
	int port;
	ne_session * http;
	bool running;
	std::string returned_data;
	Json::Reader doc;
	Json::Value getUnsavedBuffers();
};

#endif