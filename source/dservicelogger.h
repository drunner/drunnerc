#ifndef __DSERVICELOGGER_H
#define __DSERVICELOGGER_H

#include "enums.h"
#include "params.h"
#include <string>


enum eStage
{
   kSearching,
   kGotEscape,
   kTriggered
};

class escapefilter
{
public:
   escapefilter();
   void strip(std::string & buffer);
   eStage mStage;
   size_t mPos;
};

class dServiceLogger
{
public:
   dServiceLogger(bool cerr, bool isServiceCmd);
   void log(const char * const buf, int n);

private:
   void init();
   void finish();
   void processbuffer(std::string buffer);
   void nonrawoutput(std::string s);

   edServiceOutput mOutputMode;

   bool mCErr;
   bool mInitialised;
   escapefilter mEscapeFilter;
};


#endif
