#include "exceptions.h"
#include "termcolor.h"

eExit::eExit(int exitCode) : mExitCode(exitCode)
{
}

eExit::eExit(const char * msg, int exitCode) : mExitCode(exitCode)
{
   std::cerr << termcolor::red << msg << termcolor::reset << std::endl;
}

eExit::eExit(std::string msg, int exitCode) : mExitCode(exitCode)
{
   std::cerr << termcolor::red << msg << termcolor::reset << std::endl;
}

int eExit::exitCode() const throw() // we guarentee not to throw an exception.
{
   return mExitCode;
}
