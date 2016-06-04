#include <iostream>
#include <ctime>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include "enums.h"
#include "params.h"
#include "globallogger.h"
#include "globalcontext.h"
#include "exceptions.h"
#include "utils.h"
#include "termcolor.h"

std::string timestamp()
{
        //Notice the use of a stringstream, yet another useful stream medium!
        std::ostringstream stream;
        time_t rawtime;
        tm * timeinfo;

        time(&rawtime);
        timeinfo = localtime( &rawtime );

        stream << (timeinfo->tm_year)+1900<<"/"<<timeinfo->tm_mon
        <<"/"<<timeinfo->tm_mday<<" "<<timeinfo->tm_hour
        <<":"<<timeinfo->tm_min;
        // The str() function of output stringstreams return a std::string.
        return stream.str();
}

std::string levelname(eLogLevel level)
{
   switch (level)
   {
      case kLDEBUG: return "DEBUG";
      case kLINFO:  return "INFO ";
      case kLWARN:  return "WARN ";
      case kLERROR: return "ERROR";
      default: return " ??? ";
   }
}

void logverbatim(eLogLevel level, std::string s)
{
   if (level < GlobalContext::getParams()->getLogLevel())
      return;

   // we use stdout for normal messages, stderr for warn and error.
   switch (level)
   {
      case kLDEBUG: std::cout << termcolor::cyan << s << termcolor::reset; break;
      case kLINFO:  std::cout << termcolor::blue << s << termcolor::reset; break;
      case kLWARN:  std::cerr << termcolor::yellow <<  s << termcolor::reset; break;
      case kLERROR: 
         std::cerr << termcolor::red << s << termcolor::reset; 
         throw eExit();
         break;
      default:      std::cerr << termcolor::green <<  s << termcolor::reset; break;
   }
}

std::string getheader(eLogLevel level)
{
   std::ostringstream ost;
   ost << "|" << levelname(level) << "|" << timestamp() << "| ";
   return ost.str();
}


void logmsg(eLogLevel level, std::string s)
{   
   if (level < GlobalContext::getParams()->getLogLevel())
      return;

   std::string info = getheader(level);
   std::string s2=utils::replacestring(s,"\n","\n"+info);
   boost::erase_all(s2, "\r");

   logverbatim(level,info+s2+"\n");
}


void fatal(std::string s)
{
   logmsg(kLERROR, s);
}

// ----------------------------------------------------------------------------------------------------

