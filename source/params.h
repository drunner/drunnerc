#ifndef __PARAMS_H
#define __PARAMS_H

#include <vector>
#include <string>

#include "enums.h"

class params {
public:
   params(int argc, char **argv);
   std::string substitute( const std::string & source ) const;

   const std::string & getVersion() const { return mVersion; }
   eCommand getCommand() const { return mCmd; }
   eLogLevel getLogLevel() const { return mLogLevel; }

   edServiceOutput getServiceOutput_hooks() const { return mServiceOutput_hooks; }
   edServiceOutput getServiceOutput_servicecmd() const { return mServiceOutput_servicecmd; }

   const std::vector<std::string> & getArgs() const { return mArgs; }
   int numArgs() const { return mArgs.size(); }
   const std::string & getArg(int n) const;
  
   const std::vector<std::string> & getOptions() const { return mOptions; }
   bool isDevelopmentMode() const { return mDevelopmentMode; }

private:
   std::string mVersion;
   eCommand mCmd;
   std::vector<std::string> mArgs;
   eLogLevel mLogLevel;
   bool mDevelopmentMode;
   
   edServiceOutput mServiceOutput_hooks;
   edServiceOutput mServiceOutput_servicecmd;

   std::vector<std::string> mOptions;
   eCommand parsecmd(std::string s) const;
   params();
   void setdefaults();
};

#endif
