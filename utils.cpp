#include "utils.hpp"

std::string confPath(GeanyData* geany, std::string name){
	char * t = g_build_filename(geany->app->configdir, "plugins", "gycm", name.c_str(), NULL);
	
	std::string x(t);
	if(access(t, F_OK ) == -1){
		char * t2 = g_get_current_dir();
		char * t3 = g_build_filename(t2,name.c_str(), NULL);
		x = t3;
		free(t2);
		free(t3);
	}
		
	g_free(t);
	
	return x;
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