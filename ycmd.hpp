#ifndef YCMD_HPP
#define YCMD_HPP

#include <geanyplugin.h>
#include <json/json.h>
#include <string>
#include "utils.hpp"

#define HMAC_SECRET_LENGTH 16

class Ycmd {
public:
	Ycmd(GeanyData*,GeanyFunctions*);
	~Ycmd();
	bool startServer();
	bool isAlive();
	void shutdown();
private:
	void loadSettings();
	void createStashGroup();
	GeanyData* geany;
	GeanyFunctions* geany_functions;
	char hmac[HMAC_SECRET_LENGTH];
	int ycmd_stderr_fd, ycmd_stdout_fd;
	pid_t pid;
	int port;
	int getFreePort();
};

#endif