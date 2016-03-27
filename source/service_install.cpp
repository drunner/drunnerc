#include <sys/stat.h>

#include "command_dev.h"
#include "utils.h"
#include "logmsg.h"
#include "exceptions.h"
#include "settingsbash.h"
#include "command_setup.h"
#include "generate_utils_sh.h"
#include "service.h"
#include "servicehook.h"
#include "sh_servicevars.h"
#include "drunnercompose.h"

//using namespace utils;


std::string service::getUserID()
{
	std::string runcmd = "docker run --rm -i " + getImageName() + R"EOF( /bin/bash -c "id -u | tr -d '\r\n'")EOF";
	std::string op;
	if (0 != utils::bashcommand(runcmd, op))
		logmsg(kLERROR, "Unable to determine the user id in container " + getImageName());

	logmsg(kLDEBUG, "Container is running under userID " + op + ".");
	return op;
}

void service::logmsg(eLogLevel level, std::string s) const
{
   ::logmsg(level, s, mParams.getLogLevel());
}

void service::createLaunchScript()
{
	std::string target = utils::get_usersbindir() + "/" + getName();

	// remove stale script if present
	if (utils::fileexists(target))
		if (remove(target.c_str()) != 0)
			logmsg(kLERROR, "Couldn't remove stale launch script at " + target);

	// write out new one.
	std::ofstream ofs;
	ofs.open(target);
	if (!ofs.is_open())
		logmsg(kLERROR, "Couldn't write launch script at " + target);
	ofs << "#!/bin/bash" << std::endl;
	ofs << "drunner servicecmd " << getName() << " \"$@\"" << std::endl;
	ofs.close();

	// fix permissions
	if (chmod(target.c_str(), S_700) != 0)
		logmsg(kLERROR, "Unable to change permissions on " + target);

	logmsg(kLDEBUG, "Created launch script at " + target);
}



void service::createVolumes(const drunnerCompose * const drc)
{
   if (drc == NULL)
      logmsg(kLERROR, "createVolumes passed NULL drunnerCompose.");

	std::string userid = getUserID();
	if (userid == "0")
		logmsg(kLERROR, "Container is running as root user!"); // should never happen as we've validated the image already.

   std::string dname = "docker-volume-maker";

   for (const auto & entry : drc->getVolumes())
      {
         if (utils::dockerVolExists(entry.mDockerVolumeName))
            logmsg(kLINFO, "A docker volume already exists for " + entry.mDockerVolumeName + ", reusing it.");
         else
         {
            std::string op;
            int rval = utils::bashcommand("docker volume create --name=\"" + entry.mDockerVolumeName + "\"", op);
            if (rval != 0)
               logmsg(kLERROR, "Unable to create docker volume " + entry.mDockerVolumeName);
         }

         // set permissions on volume.
         tVecStr args;
         args.push_back("docker");
         args.push_back("run");
         args.push_back("--name=\"" + dname + "\"");
         args.push_back("-v");
         args.push_back(entry.mDockerVolumeName + ":" + "/tempmount");
         args.push_back("drunner/rootutils");
         args.push_back("chown");
         args.push_back(userid + ":root");
         args.push_back("/tempmount");

         utils::dockerrun dr("/usr/bin/docker", args, dname, mParams);
      }
}

void service::recreate(bool updating)
{
   if (updating)
      command_setup::pullImage(mParams, mSettings, getImageName());
   
   try
   {
      // nuke any existing dService files on host (but preserve volume containers!).
      if (utils::fileexists(getPath()))
         utils::deltree(getPath(), mParams);

      // create the basic directories.
      ensureDirectoriesExist();

      // copy files to service directory on host.
      std::string op;
      int r = utils::bashcommand("docker run --rm -i -v " +
         getPathdRunner() + ":/tempcopy " + getImageName() + " /bin/bash -c \"cp -r /drunner/* /tempcopy/ && chmod a+rx /tempcopy/*\"", op);
      if (r != 0)
         logmsg(kLERROR, "Couldn't copy the service files. You will need to reinstall the service.");

      // write out variables.sh for the dService.
      drunnerCompose drc(*this, mParams);
      if (drc.readOkay()==kRError)
         fatal("Unexpected error - docker-compose.yml is broken.");
      
      // write out servicevars.sh for ourselves.
      sh_servicevars svcvars(getPath());
      svcvars.create(getImageName());
      if (!svcvars.write())
         fatal("Unexpected error - couldn't write out servicevars.sh.");

      // make sure we have the latest of all exra containers.
      for (const auto & entry : drc.getServicesInfo())
         command_setup::pullImage(mParams, mSettings, entry.mImageName);

      // create the utils.sh file for the dService.
      generate_utils_sh(getPathdRunner(), mParams);

      // create launch script
      createLaunchScript();

      // create volumes
      createVolumes(&drc);
   }

   catch (const eExit & e) {
      // tidy up.
      if (utils::fileexists(getPath()))
         utils::deltree(getPath(), mParams);

      throw (e);
   }
}

void service::install()
{
   logmsg(kLDEBUG, "Installing " + getName() + " at " + getPath() + ", using image " + getImageName());
	if (utils::fileexists(getPath()))
		logmsg(kLERROR, "Service already exists. Try:   drunner update " + getName());

	// make sure we have the latest version of the service.
	command_setup::pullImage(mParams, mSettings, getImageName());

	logmsg(kLDEBUG, "Attempting to validate " + getImageName());
   validateImage(mParams,mSettings,getImageName());

   recreate(false);

   tVecStr args;
   servicehook hook(this, "install", args, mParams);
   hook.endhook();
}

eResult service::uninstall()
{
   if (!utils::fileexists(getPath()))
      logmsg(kLERROR, "Can't uninstall " + getName() + " - it does not exist.");

   tVecStr args;
   servicehook hook(this, "uninstall", args, mParams);
   hook.starthook();

   utils::deltree(getPath(), mParams);

   if (utils::fileexists(getPath()))
      logmsg(kLERROR, "Uninstall failed - couldn't delete " + getPath());

   hook.endhook();
   return kRSuccess;
}

eResult service::obliterate()
{
   if (!utils::fileexists(getPath()))
      logmsg(kLERROR, "Can't obliterate " + getName() + " - it does not exist.");

   tVecStr args;
   servicehook hook(this, "obliterate", args, mParams);
   hook.starthook();

   logmsg(kLDEBUG, "Obliterating all the docker volumes - data will be gone forever.");
   {// [start] deleting docker volumes.
      drunnerCompose drc(*this, mParams);
      if (drc.readOkay())
      {
         for (const auto & vol : drc.getVolumes())
         {
            logmsg(kLINFO, "Obliterating docker volume " + vol.mDockerVolumeName);
            std::string op;
            if (0 != utils::bashcommand("docker volume rm " + vol.mDockerVolumeName, op))
               logmsg(kLWARN, "Failed to remove " + vol.mDockerVolumeName + " -- " + op);
         }
      }
      else
         logmsg(kLDEBUG, "Couldn't read configuration to delete the associated docker volumes. :/");
   }// [end] deleting docker volumes.

   // delete the host volumes
   logmsg(kLDEBUG, "Deleting the host volume.");
   utils::deltree(getPathHostVolume(), mParams);

   // delete the service tree.
   logmsg(kLDEBUG, "Deleting the service files.");
   utils::deltree(getPath(), mParams);

   logmsg(kLINFO, "Obliterated " + getName());
   return kRSuccess;
}

eResult service::recover()
{
   if (utils::fileexists(getPath()))
      uninstall();

   install();

   logmsg(kLINFO, getName() + " recovered.");
   return kRSuccess;
}
