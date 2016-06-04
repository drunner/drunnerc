#ifndef __EXCEPTIONS_H
#define __EXCEPTIONS_H

#include <exception>
#include <string.h>
#include <iostream>

#include "termcolor.h"

class eExit: public std::exception
{
   public:
      eExit(int exitCode = 1);
      eExit(const char * msg, int exitCode = 1);
      eExit(std::string msg, int exitCode = 1);
      int exitCode() const throw(); // we guarentee not to throw an exception.

   private:
      int mExitCode;
};


#endif
