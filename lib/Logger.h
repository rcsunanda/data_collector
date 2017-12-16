#pragma once

//Boost logger headers
#include <boost/log/trivial.hpp>

#include <string>

void initializeLog(int logLevel, int rotationSizeMB, int maxFileCount, std::string logFilename);
