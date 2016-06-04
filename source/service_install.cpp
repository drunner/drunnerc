#include <sys/stat.h>

#include "command_dev.h"
#include "utils.h"
#include "utils_docker.h"
#include "globallogger.h"
#include "globalcontext.h"
#include "exceptions.h"
#include "settingsbash.h"
#include "command_setup.h"
#include "generate_utils_sh.h"
#include "service.h"
#include "servicehook.h"
#include "sh_servicevars.h"
#include "drunnercompose.h"

//using namespace utils;


std::string service::getUserID(std::string imagename) const
{
	std::string runcmd = "docker run --rm -i " + imagename + R"EOF( /bin/bash -c "id -u | tr -d '\r\n'")EOF";
	std::string op;
	if (0 != utils::bashcommand(runcmd, op))
		logmsg(kLERROR, "Unable to determine the user id in container " + imagename);

	logmsg(kLDEBUG, imagename+" is running under userID " + op + ".");
	return op;
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

   std::string dname = "docker-volume-maker";

   for (const auto & svc : drc->getServicesInfo())
   {
      // each service may be running under a different userid.
      std::string userid = getUserID(svc.mImageName);
      if (userid == "0")
         logmsg(kLERROR, svc.mImageName + " is running as root user! Verboten."); // should never happen as we've validated the image already.

      for (const auto & entry : svc.mVolumes)
      {
         if (utils::dockerVolExists(entry.mDockerVolumeName))
            logmsg(kLINFO, "A docker volume already exists for " + entry.mDockerVolumeName + ", reusing it for " + svc.mImageName + ".");
         else
         {
            std::string op;
            int rval = utils::bashcommand("docker volume create --name=\"" + entry.mDockerVolumeName + "\"", op);
            if (rval != 0)
               logmsg(kLERROR, "Unable to create docker volume " + entry.mDockerVolumeName);
            logmsg(kLDEBUG, "Created docker volume " + entry.mDockerVolumeName + " for " + svc.mImageName);
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

         utils::dockerrun dr("/usr/bin/docker", args, dname);

         logmsg(kLDEBUG, "Set permissions to allow user " + userid + " access to volume " + entry.mDockerVolumeName);
      }
   }
}

void service::recreate(bool updating)
{
   if (updating)
      utils_docker::pullImage(getImageName());
   
   try
   {
      // nuke any existing dService files on host (but preserve volume containers!).
      if (utils::fileexists(getPath()))
         utils::deltree(getPath());

      // notice for hostVolumes.
      if (utils::fileexists(getPathHostVolume()))
         logmsg(kLINFO, "A drunner hostVolume already exists for " + getName() + ", reusing it.");

      // create the basic directories.
      ensureDirectoriesExist();

      // copy files to service directory on host.
      std::string op;
      int r = utils::bashcommand("docker run --rm -i -v " +
         getPathdRunner() + ":/tempcopy " + getImageName() + " /bin/bash -c \"cp -r /drunner/* /tempcopy/ && chmod a+rx /tempcopy/*\"", op);
      if (r != 0)
         logmsg(kLERROR, "Couldn't copy the service files. You will need to reinstall the service.");

      // write out variables.sh for the dService.
      drunnerCompose drc(*this);
      if (drc.readOkay()==kRError)
         fatal("Unexpected error - docker-compose.yml is broken.");
      
      // write out servicevars.sh for ourselves.
      sh_servicevars svcvars;
      svcvars.create(getImageName());
      if (!svcvars.writeSettings(svcvars.getPathFromParent(getPath())))
         fatal("Unexpected error - couldn't write out servicevars.sh.");

      // make sure we have the latest of all exra containers.
      for (const auto & entry : drc.getServicesInfo())
         if (entry.mImageName != getImageName()) // don't pull main image again.
            utils_docker::pullImage(entry.mImageName);

      // create the utils.sh file for the dService.
      generate_utils_sh(getPathdRunner());

      // create launch script
      createLaunchScript();

      // create volumes
      createVolumes(&drc);
   }

   catch (const eExit & e) {
      // tidy up.
      if (utils::fileexists(getPath()))
         utils::deltree(getPath());

      throw (e);
   }
}

void service::install()
{
   logmsg(kLDEBUG, "Installing " + getName() + " at " + getPath() + ", using image " + getImageName());
	if (utils::fileexists(getPath()))
		logmsg(kLERROR, "Service already exists. Try:   drunner update " + getName());

	// make sure we have the latest version of the service.
   utils_docker::pullImage(getImageName());

	logmsg(kLDEBUG, "Attempting to validate " + getImageName());
   validateImage(getImageName());

   recreate(false);

   servicehook hook(this, "install");
   hook.endhook();

   logmsg(kLINFO, "Installation complete - try running " + getName()+ " now!");
}

eResult service::uninstall()
{
   if (!utils::fileexists(getPath()))
      logmsg(kLERROR, "Can't uninstall " + getName() + " - it does not exist.");

   servicehook hook(this, "uninstall");
   hook.starthook();

   utils::deltree(getPath());

   if (utils::fileexists(getPath()))
      logmsg(kLERROR, "Uninstall failed - couldn't delete " + getPath());

   hook.endhook();
   return kRSuccess;
}

eResult service::obliterate()
{
   if (!utils::fileexists(getPath()))
      logmsg(kLERROR, "Can't obliterate " + getName() + " - it does not exist.");

   servicehook hook(this, "obliterate");
   hook.starthook();

   logmsg(kLDEBUG, "Obliterating all the docker volumes - data will be gone forever.");
   {// [start] deleting docker volumes.
      drunnerCompose drc(*this);
      if (drc.readOkay()!=kRError)
      {
         tVecStr docvolnames;
         drc.getDockerVolumeNames(docvolnames);
         for (const auto & vol : docvolnames)
         {
            logmsg(kLINFO, "Obliterating docker volume " + vol);
            std::string op;
            if (0 != utils::bashcommand("docker volume rm " + vol, op))
               logmsg(kLWARN, "Failed to remove " + vol + " -- " + op);
         }
      }
      else
         logmsg(kLDEBUG, "Couldn't read configuration to delete the associated docker volumes. :/");
   }// [end] deleting docker volumes.

   // delete the host volumes
   logmsg(kLINFO, "Obliterating the hostVolumes (environment and servicerunner)");
   utils::deltree(getPathHostVolume());

   // delete the service tree.
   logmsg(kLINFO, "Obliterating all of the dService files");
   utils::deltree(getPath());

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
