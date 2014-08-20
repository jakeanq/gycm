#include <geany.h>
#include <geanyplugin.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;
GeanyFunctions *geany_functions;

PLUGIN_VERSION_CHECK(217); /* May be overly high */

PLUGIN_SET_INFO("GYCM", "YouCompleteMe smart code completion for Geany", "0.1", "Jake Bott <jake.anq\064gmail.com>");

extern "C" void plugin_init(GeanyData* _data){
}

extern "C" void plugin_cleanup(void) {
}
