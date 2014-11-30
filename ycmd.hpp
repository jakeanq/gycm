#ifndef YCMD_HPP
#define YCMD_HPP

#include <geanyplugin.h>
#include <json/json.h>
#include <string>
#include "utils.hpp"
#include <neon/ne_session.h>

#define HMAC_SECRET_LENGTH 16

#define DEFINED_SUBCOMMANDS_HANDLER "/defined_subcommands"
#define CODE_COMPLETIONS_HANDLER "/completions"
#define COMPLETER_COMMANDS_HANDLER "/run_completer_command"
#define EVENT_HANDLER "/event_notification"
#define EXTRA_CONF_HANDLER "/load_extra_conf_file"

class Ycmd {
public:
	Ycmd(GeanyData*,GeanyFunctions*);
	~Ycmd();
	bool startServer();
	bool isAlive();
	void shutdown();
	bool assertServer();
	bool restart();
	// Neon rcv handler
	int handler(const char *, size_t);
	
	// Geany signal handlers
	void handleDocumentLoad(GObject*, GeanyDocument*);
	void handleDocumentUnload(GObject*, GeanyDocument*);
	void handleDocumentVisit(GObject*, GeanyDocument*);
	void complete(GObject*,GeanyDocument*);
private:
	gchar * b64HexHMAC(std::string& data);
	void jsonRequestBuild(GeanyDocument*, Json::Value&, Json::Value&);
	void jsonRequestBuild(GeanyDocument*, Json::Value&);
	void send(Json::Value&,std::string);
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
	Json::Value getUnsavedBuffers(GeanyDocument*);
	Json::Value currentMessage;
	ScintillaObject * currentEditor;
};

#endif