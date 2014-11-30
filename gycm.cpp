#include <geany.h>
#include <geanyplugin.h>
#include "ycmd.hpp"
GeanyPlugin *geany_plugin;
GeanyData *geany_data;
GeanyFunctions *geany_functions;

PLUGIN_VERSION_CHECK(217); /* May be overly high */

PLUGIN_SET_INFO("GYCM", "YouCompleteMe smart code completion for Geany", "0.1", "Jake Bott <jake.anq\100gmail.com>");

Ycmd * y;

extern "C" void handle_document_load(GObject *obj, GeanyDocument *doc, gpointer user_data){
	((Ycmd*)user_data)->handleDocumentLoad(obj,doc);
}

extern "C" void handle_document_unload(GObject *obj, GeanyDocument *doc, gpointer user_data){
	((Ycmd*)user_data)->handleDocumentUnload(obj,doc);
}

extern "C" void handle_document_visit(GObject *obj, GeanyDocument *doc, gpointer user_data){
	((Ycmd*)user_data)->handleDocumentVisit(obj,doc);
}
extern "C" gboolean handle_sci_event(GObject *obj, GeanyEditor *edit, SCNotification *nt, gpointer user_data){
	switch(nt->nmhdr.code){
		case SCN_CHARADDED:
			((Ycmd*)user_data)->complete(obj,edit->document);
			return false;
	}
	return false;
}

extern "C" void plugin_init(GeanyData*){
	y = new Ycmd(geany,geany_functions);
	y->startServer();
	guint i;
	foreach_document(i){
		y->handleDocumentLoad(NULL,documents[i]);
		break;
	}
	plugin_signal_connect(geany_plugin, NULL, "document-open",     TRUE,  (GCallback) &handle_document_load,   y);
	plugin_signal_connect(geany_plugin, NULL, "document-reload",   TRUE,  (GCallback) &handle_document_load,   y);
	plugin_signal_connect(geany_plugin, NULL, "document-close",    FALSE, (GCallback) &handle_document_unload, y);
	plugin_signal_connect(geany_plugin, NULL, "document-activate", FALSE, (GCallback) &handle_document_visit,  y);
	plugin_signal_connect(geany_plugin, NULL, "editor-notify",     FALSE, (GCallback) &handle_sci_event,       y);
}

extern "C" void plugin_cleanup(void) {
	y->shutdown();
	delete y;
}