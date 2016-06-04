#include <unistd.h>
#include <sstream>

#include "service.h"
#include "globallogger.h"
#include "utils.h"
#include "servicehook.h"
#include "sh_servicevars.h"
#include "drunnercompose.h"
#include "globalcontext.h"

service::service(const std::string & servicename, std::string imagename /*= ""*/) :
   servicepaths(servicename),
   mImageName( loadImageName(servicename,imagename) ),
   mEnvironment(*this)
{
}

servicepaths::servicepaths(const std::string & servicename) :
   mName(servicename)
{
}

std::string service::loadImageName(const std::string & servicename, std::string imagename)
{
   std::string v = servicepaths(servicename).getPath();
   if (utils::fileexists(v))
   {
      if (imagename.length() == 0)
      { // if imagename override not provided and variables.sh exists then load it from variables.sh
         sh_servicevars shv;
         if (!shv.readSettings( shv.getPathFromParent(v) ))
         {
            ::logmsg(kLWARN, "Couldn't read servicevars.sh from " + v);
            ::logmsg(kLWARN, "Service old/broken. Recover with:");
            ::logmsg(kLWARN, "   drunner recover " + servicename + " IMAGENAME");
            ::logmsg(kLERROR, "Unable to determine the image name. Exiting.");
         }
         imagename = shv.getImageName();
      }
      else
         ::logmsg(kLDEBUG, "Forcing dService main image to be " + imagename);
   }

   if (imagename.length() == 0)
      ::logmsg(kLERROR, "Couldn't determine imagename.");

   return imagename;
}


std::string servicepaths::getPath() const
{
   return GlobalContext::getSettings()->getPath_dServices() + "/" + mName;
}

std::string servicepaths::getPathdRunner() const
{
   return getPath() + "/" + "drunner";
}

std::string servicepaths::getPathTemp() const
{
   return getPath() + "/" + "temp";
}

std::string servicepaths::getPathHostVolume() const
{
   return GlobalContext::getSettings()->getPath_HostVolumes() + "/" + mName;
}

std::string servicepaths::getPathHostVolume_servicerunner() const
{
   return getPathHostVolume() + "/servicerunner";
}

std::string servicepaths::getPathHostVolume_environment() const
{
   return getPathHostVolume() + "/environment";
}

std::string servicepaths::getPathServiceRunner() const
{
   return getPathdRunner() + "/servicerunner";
}

std::string servicepaths::getPathDockerCompose() const
{
   return getPathdRunner() + "/docker-compose.yml";
}


std::string servicepaths::getName() const
{
   return mName;
}

void service::ensureDirectoriesExist() const
{
   // create service's drunner and temp directories on host.
   utils::makedirectory(getPath(), S_755);
   utils::makedirectory(getPathdRunner(), S_777);
   utils::makedirectory(getPathTemp(), S_777);
   utils::makedirectory(getPathHostVolume_environment(), S_700);
   utils::makedirectory(getPathHostVolume_servicerunner(), S_777);
}

bool service::isValid() const
{
   if (!utils::fileexists(getPath()) || !utils::fileexists(getPathdRunner()) || !utils::fileexists(getPathTemp()))
      return false;

   drunnerCompose drc(*this);
   if (drc.readOkay()==kRError)
   {
      logmsg(kLDEBUG, "docker-compose.yml is broken for service "+getName());
      return false;
   }
   sh_servicevars shv;
   if (!shv.readSettings(getPath()))
   {
      logmsg(kLDEBUG, "Couldn't read servicevars.sh from service " + getName());
      return false;
   }

   return true;
}

cResult service::servicecmd()
{
   const params & p(*GlobalContext::getParams());

   if (p.numArgs() < 1)
      fatal("Programming error - number of arguments should never be 0 in service::servicecmd");

   std::vector<std::string> cargs( p.getArgs().begin() + 1, p.getArgs().end() );

   if (p.numArgs() < 2 || utils::stringisame(mName, "help"))
   {
      cargs.push_back("help");
      serviceRunnerCommand(cargs);
      return kRSuccess;
   }

   std::string command = p.getArgs()[1];
   std::string reservedwords = " install backupstart backupend backup restore update enter uninstall obliterate recover ";
   if (utils::findStringIC(reservedwords, " " + command + " ")) // spaces are to ensure whole word match.
   {
      if (command != "enter")
         logmsg(kLERROR, command + " is a reserved word. You might want  drunner " + command + " " + mName + "  instead.");
      enter(); // uses execl so never returns.
      logmsg(kLERROR, "Should never get here. Enter command shouldn't return.");
   }

   servicehook hook(this, "servicecmd", cargs);
   hook.starthook();

   cResult rval( serviceRunnerCommand(cargs) );

   hook.endhook();

   return rval;
}

void service::update()
{ // update the service (recreate it)
   servicehook hook(this, "update");
   hook.starthook();

   recreate(true);
   
   hook.endhook();
}

const std::string service::getImageName() const
{
   return mImageName;
}

void service::enter()
{
   servicehook hook(this, "enter");
   hook.starthook();

   execl(getPathServiceRunner().c_str(), "servicerunner", "enter", NULL);
}

int service::status()
{
   servicehook hook(this, "status");
   hook.starthook();

   if (!utils::fileexists(getPath()))
   {
      logmsg(kLINFO, getName() + " is not installed.");
      hook.endhook();
      return 1;
   }
   if (!isValid())
   {
      logmsg(kLINFO, getName() + " is not a valid service. Try  drunner recover "+getName());
      hook.endhook();
      return 1;
   }
   logmsg(kLINFO, getName() + " is installed and valid.");
   hook.endhook();
   return 0;
}



void validateImage(std::string imagename)
{
   if (!utils::fileexists(GlobalContext::getSettings()->getPath_Root())) 
      logmsg(kLERROR, "ROOTPATH not set.");

   if (utils::imageisbranch(imagename))
      logmsg(kLDEBUG, imagename + " looks like a development branch (won't be pulled).");
   else
      logmsg(kLDEBUG, imagename + " should be a production image.");

   std::string op;
   int rval = utils::bashcommand(
      "docker run --rm -v \"" + GlobalContext::getSettings()->getPath_Support() +
      ":/support\" \"" + imagename + "\" /support/validator-image 2>&1", op);

   if (rval != 0)
   {
      if (utils::findStringIC(op, "Unable to find image"))
         logmsg(kLERROR, "Couldn't find image " + imagename);
      else
         logmsg(kLERROR, op);
   }
   logmsg(kLINFO, "\u2714  " + imagename + " is dRunner compatible.");
}


cResult service::serviceRunnerCommand(const std::vector<std::string> & args) const
{
   // set environment.
   drunnerCompose drc(*this);
   if (drc.readOkay()==kRError)
      logmsg(kLWARN, "Couldn't set up drunnerCompose. servicerunner will not have environment set correctly.");
   else
      drc.setServiceRunnerEnv();
 
   if (args.size() > 0 && utils::stringisame(args[0],"servicerunner"))
      logmsg(kLERROR, "Programming error - someone gave serviceRunnerCommand the first arg: servicerunner :/");
   auto newargs(args);
   newargs.insert(newargs.begin(), std::string("servicerunner"));

   cResult rval(utils::dServiceCmd(getPathServiceRunner(), newargs, true));
   return rval;
}

cServiceEnvironment & service::getEnvironment()
{
   return mEnvironment;
}

const cServiceEnvironment & service::getEnvironmentConst() const
{
   return mEnvironment;
}

cServiceEnvironment::cServiceEnvironment(const servicepaths & paths) :
   settingsbash(true)
{
   mPath = paths.getPathHostVolume_environment() + "/servicerunner_env.sh";
   if (utils::fileexists(mPath))
   {
      if (!readSettings(mPath))
         fatal("servicerunner_env.sh is corrupt.");
   }
}

void cServiceEnvironment::save_environment(std::string key, std::string value)
{
   setString(key, value);
   if (!writeSettings(mPath))
      fatal("Failed to write environment file " + mPath);
}

unsigned int cServiceEnvironment::getNumVars() const
{
   return mElements.size();
}

std::string cServiceEnvironment::index2key(unsigned int i) const
{
   if (i >= getNumVars()) fatal("Invalid index into cServiceEnvironment.");
   return mElements[i].getkey();
}

std::string cServiceEnvironment::get_value(const std::string & key) const
{ 
   return getString(key); 
}
