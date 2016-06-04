#include "enums.h"
#include "service.h"
#include "servicehook.h"
#include "globallogger.h"
#include "utils.h"
#include "drunnercompose.h"
#include "cresult.h"

servicehook::servicehook(const service * const svc, std::string actionname, const std::vector<std::string> & hookparams) :
   mService(svc), mActionName(actionname), mHookParams(hookparams)
{
   setHookCmds();
}

servicehook::servicehook(const service * const svc, std::string actionname) :
   mService(svc), mActionName(actionname)
{
   setHookCmds();
}

cResult servicehook::starthook()
{
   if (mStartCmd.length() > 0)
      return runHook(mStartCmd);

   logmsg(kLDEBUG, "dService doesn't require hook for " + mActionName + " start");
   return kRNoChange;
}

cResult servicehook::endhook()
{
   if (mEndCmd.length()>0)
      return runHook(mEndCmd);

   logmsg(kLDEBUG, "dService doesn't require hook for " + mActionName + " end");
   return kRNoChange;
}

cResult servicehook::runHook(std::string se)
{
   if (!utils::fileexists(mServiceRunner))
   {
      logmsg(kLWARN, "Couldn't run hook " + se + " because dService's servicerunner is not installed.");
      return kRError;
   }

   std::vector<std::string> args;
   //args.push_back("servicerunner");
   args.push_back(se);
   for (const auto & entry : mHookParams)
      args.push_back(entry);

   cResult rval(mService->serviceRunnerCommand(args));
   //cResult rval = utils::dServiceCmd(mServiceRunner, args, mParams);
   if (rval.isNOIMPL())
      rval=kRNoChange; // not implemented is perfectly fine for hooks.

   if (rval.isError())
      logmsg(kLWARN, "dService hook " + se + " returned error.");
   else
      logmsg(kLDEBUG, "dService hook for " + se + " complete");

   return rval;
}


void servicehook::setHookCmds()
{      
   mServiceRunner = mService->getPathServiceRunner();
   mStartCmd = "";
   mEndCmd = "";

   mStartCmd = mActionName + "_start";
   mEndCmd   = mActionName + "_end";

   // some hooks don't make sense because the dService won't exist at that point.
   // spaces are to ensure whole word match.
   if (utils::findStringIC(" install_start "," "+mStartCmd+" "))
      mStartCmd = "";
   if (utils::findStringIC(" uninstall_end obliterate_end enter_end "," "+mEndCmd+" "))
      mEndCmd = "";
}

