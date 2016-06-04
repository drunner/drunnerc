#include <boost/filesystem.hpp>
#include <sstream>

#include "command_setup.h"
#include "utils.h"
#include "utils_docker.h"
#include "sh_drunnercfg.h"
#include "globallogger.h"
#include "globalcontext.h"
#include "generate_validator_image.h"
#include "drunnercompose.h"

namespace command_setup
{

   int setup()
   {
      const params & p(*GlobalContext::getParams().get());
      const sh_drunnercfg & settings(*GlobalContext::getSettings().get());

      // -----------------------------------------------------------------------------
      // create rootpath if it doesn't exist.
      utils::makedirectory(settings.getPath_Root(), S_755);

      // -----------------------------------------------------------------------------
      // Update settings on disk.
      if (!settings.writeSettings())
         logmsg(kLERROR, "Couldn't write settings file!");

      // -----------------------------------------------------------------------------
      // move this executable to the directory.
      //int result = rename( utils::get_exefullpath().c_str(), (rootpath+"/drunner").c_str());
      if (!utils::copyfile(utils::get_exefullpath(), settings.getPath_Root() + "/drunner"))
         logmsg(kLERROR, "Couldn't copy drunner executable from " + utils::get_exefullpath() + " to " + settings.getPath_Root() + ".");

      // -----------------------------------------------------------------------------
      // create bin directory
      std::string bindir = utils::get_usersbindir();
      utils::makedirectory(bindir, S_700);

      // -----------------------------------------------------------------------------
      // create symlink
      utils::makesymlink(settings.getPath_Root() + "/drunner", bindir + "/drunner");

      // sort out docker-compose - now expected to be present.
      //InstallDockerCompose(p);

      // get latest root util image.
      //std::cerr << "ROOTUITILIMAGE = " << settings.getRootUtilImage() << std::endl;
      utils_docker::pullImage(settings.getRootUtilImage());

      // -----------------------------------------------------------------------------
      // create services, support and temp directories
      utils::makedirectory(settings.getPath_dServices(), S_755);
      utils::makedirectory(settings.getPath_Support(), S_755);
      utils::makedirectory(settings.getPath_Temp(), S_755);
      utils::makedirectory(settings.getPath_HostVolumes(), S_755);

      // create the validator script that is run inside containers
      generate_validator_image(settings.getPath_Support());

      // -----------------------------------------------------------------------------
      // Finished!
      if (settings.mReadOkay)
         logmsg(kLINFO, "Update of drunner to " + p.getVersion() + " completed succesfully.");
      else
         logmsg(kLINFO, "Setup of drunner " + p.getVersion() + " completed succesfully.");

      return 0;
   }

   int update()
   {
      const params & p(*GlobalContext::getParams().get());
      const sh_drunnercfg & s(*GlobalContext::getSettings().get());

      logmsg(kLDEBUG, "Updating dRunner in " + s.getPath_Root());

      std::string url(s.getdrunnerInstallURL()), trgt(s.getPath_Root() + "/drunner-install");
      utils::downloadexe(url, trgt);

      logmsg(kLINFO, "Updating...");

      tVecStr args;
      args.push_back("drunner-install");
      for (auto opt : p.getOptions())
         args.push_back(opt);
      args.push_back("setup");
      args.push_back(s.getPath_Root());
      
      std::ostringstream oss;
      for (auto arg : args)
         oss << arg << " ";
      logmsg(kLDEBUG, utils::trim_copy(oss.str()));

      utils::execv(trgt, args);

      logmsg(kLERROR, "Exec failed.");
      return 1;
   }

}
