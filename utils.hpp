#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <geanyplugin.h>

std::string confPath(GeanyData*,std::string);

std::string slurp(std::string);

int getFreePort();

std::string strToLower(std::string);

bool fileExists(std::string);
#endif