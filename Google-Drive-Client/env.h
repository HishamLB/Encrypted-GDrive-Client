#ifndef ENV_H
#define ENV_H

#include <string>

std::string getEnv(const std::string &key, const std::string &defaultValue = "");

#endif
