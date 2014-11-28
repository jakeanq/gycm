#include <geany.h>
#include <geanyplugin.h>
#include "ycmd.hpp"
GeanyPlugin *geany_plugin;
GeanyData *geany_data;
GeanyFunctions *geany_functions;

PLUGIN_VERSION_CHECK(217); /* May be overly high */

PLUGIN_SET_INFO("GYCM", "YouCompleteMe smart code completion for Geany", "0.1", "Jake Bott <jake.anq\100gmail.com>");

Ycmd * y;

extern "C" void plugin_init(GeanyData*){
	y = new Ycmd(geany,geany_functions);
	y->startServer();
	guint i;
	foreach_document(i){
		y->complete(documents[i]);
	}
}

extern "C" void plugin_cleanup(void) {
	y->shutdown();
	delete y;
}
