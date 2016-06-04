#include "globalcontext.h"
#include "globallogger.h"
#include "utils.h"

std::shared_ptr<const params> GlobalContext::s_params = 0;
std::shared_ptr<const sh_drunnercfg> GlobalContext::s_settings = 0;

void GlobalContext::init(int argc, char **argv)
{
   // parse the command line parameters.
   s_params = std::make_shared<const params>(argc,argv);

   // load the dRunner settings from the config file.
   {
      std::string rootpath;
      if (s_params->getCommand() == c_setup)
      { // if we're setting up we expect the user to specifiy the path to install to on the command line.
         if (s_params->getArgs().size() < 1)
            fatal("Usage:\n   drunner setup ROOTPATH");
         rootpath = utils::getabsolutepath(s_params->getArgs()[0]);
      }
      else // otherwise the install path is the location of this executable
         rootpath = utils::get_exepath();

      s_settings = std::make_shared<const sh_drunnercfg>(rootpath);
      if (!s_settings->mReadOkay && s_params->getCommand() != c_setup)
         fatal("Failed to read settings file. Try running drunner setup.");
   }
}

bool GlobalContext::hasParams()
{
   return s_params.get() != NULL;
}
bool GlobalContext::hasSettings()
{
   return s_settings.get() != NULL;
}

std::shared_ptr<const params> GlobalContext::getParams()
{
   if (!hasParams())
      fatal("Attempted to retrieve parameters from GlobalContext when not yet initialised.");
   return s_params;
}
std::shared_ptr<const sh_drunnercfg> GlobalContext::getSettings()
{
   if (!hasSettings())
      fatal("Attempted to retrieve settings from GlobalContext when settings have not been read.");
   return s_settings;
}

