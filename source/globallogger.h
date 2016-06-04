#ifndef __GLOBALLOGGER_H
#define __GLOBALLOGGER_H

#include "enums.h"
#include "params.h"
#include <string>
#include <memory>

void logmsg(eLogLevel level, std::string s);
void logverbatim(eLogLevel level, std::string s);
void fatal(std::string s);

std::string getheader(eLogLevel level);

#endif
