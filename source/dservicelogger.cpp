#include <iostream>
#include <ctime>
#include <sstream>
#include <fstream>

#include "dservicelogger.h"
#include "globallogger.h"
#include "globalcontext.h"


dServiceLogger::dServiceLogger(bool cerr, bool isServiceCmd) :
   mCErr(cerr), mInitialised(false)
{
   if (isServiceCmd)
      mOutputMode = GlobalContext::getParams()->getServiceOutput_servicecmd();
   else
      mOutputMode = GlobalContext::getParams()->getServiceOutput_hooks();
}

void dServiceLogger::log(const char * const buf, int n)
{
   if (mOutputMode == kOSuppressed)
      return;
   if (n == 0)
      return;

   if (mOutputMode == kORaw)
   { // output raw.
      if (mCErr)
         std::cerr.write(buf, n).flush();
      else
         std::cout.write(buf, n).flush();
      return;
   }

   // buffer and log.
   std::ostringstream oss;
   oss.write(buf, n).flush();
   processbuffer(oss.str());
}

void dServiceLogger::nonrawoutput(std::string s)
{
   if (mCErr)
      logverbatim(kLWARN, s);
   else
      logverbatim(kLINFO, s);
}

void dServiceLogger::processbuffer(std::string buffer)
{
   mEscapeFilter.strip(buffer);
   while (buffer.length() > 0) {
      if (!mInitialised)
      {
         mInitialised = true;
         nonrawoutput(getheader(mCErr ? kLWARN : kLINFO));
      }

      size_t pos = buffer.find('\n');
      if (pos == std::string::npos)
      {
         nonrawoutput(buffer);
         return;
      }
      nonrawoutput(buffer.substr(0, pos + 1));
      buffer.erase(0, pos + 1);
      mInitialised = false;
   }
}

escapefilter::escapefilter() : mStage(kSearching)
{
}

// This class strips standard escape sequences used for terminal color
// from a stream (e.g. raw reads that can stop at any character, including
// in the middle of an escape sequence). It stores state across calls
// to know what to strip.
void escapefilter::strip(std::string & buffer)
{
   mPos = 0; // new string so if we're triggered etc consider from beginning.

   while (true)
   { // breakable loop

      if (mStage == kSearching)
      {
         mPos = buffer.find('\033');
         if (mPos == std::string::npos)
            return;
         buffer.erase(mPos, 1);
         mStage = kGotEscape;
      }

      if (mStage == kGotEscape)
      {
         if (mPos >= buffer.length())
            return;

         if (buffer[mPos] != '[')
            mStage = kSearching;
         else
            mStage = kTriggered;
      }

      if (mStage == kTriggered)
      {
         size_t epos = buffer.find('m', mPos);
         if (epos == std::string::npos)
         {
            buffer.erase(mPos);
            return;
         }
         buffer.erase(mPos, epos - mPos + 1);
         mStage = kSearching;
      }
   }
}
