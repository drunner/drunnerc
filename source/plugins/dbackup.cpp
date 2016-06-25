#include <string>
#include <vector>
#include <fstream>

#include "utils.h"
#include "globallogger.h"
#include "globalcontext.h"
#include "timez.h"
#include "service.h"

#include "dbackup.h"
#include "backupConfig.h"

// -----------------------------------------------------------------------------------------------------------

dbackup::dbackup()
{
}

std::string dbackup::getName() const
{
   return std::string("dbackup");
}

eResult dbackup::runCommand() const
{
   if (GlobalContext::getParams()->numArgs() == 0)
      return showhelp();

   std::string cmd = GlobalContext::getParams()->getArg(0);

   logmsg(kLDEBUG, "Running command " + cmd);

   switch (str2int(cmd.c_str()))
   {
   case str2int("help") :
      return showhelp();

   default:
      fatal("Unrecognised command " + cmd);
      return kRError;
   }
}


// -----------------------------------------------------------------------------------------------------------

eResult dbackup::include(std::string servicename) const
{
   backupConfig config;
   if (!config.load())
      logmsg(kLERROR, "Backups are not yet configured. Run dbackup configure first.");

   if (config.enable(servicename) == kRNoChange)
      return kRNoChange;

   if (!config.save())
      logmsg(kLERROR, "Failed to write to configuration file.");

   return kRSuccess;
}

eResult dbackup::exclude(std::string servicename) const
{
   backupConfig config;
   if (!config.load())
      logmsg(kLERROR, "Backups are not yet configured. Run dbackup configure first.");

   if (config.disable(servicename) == kRNoChange)
      return kRNoChange;

   if (!config.save())
      logmsg(kLERROR, "Failed to write to configuration file.");

   return kRSuccess;
}

eResult dbackup::run() const
{
   backupConfig config;
   if (!config.load())
   {
      logmsg(kLINFO, "Backups are not yet configured.  Run dbackup configure first.");
      return kRError;
   }

   std::vector<std::string> services;
   utils::getAllServices(services);

   logmsg(kLINFO, "Backing up services.");
   for (auto const & s : services)
      if (config.isEnabled(s))
      {
         // backup service s.
         std::string path = config.getBackupPath() + "/" + timeutils::getArchiveName(s);

         service svc(s);
         svc.backup(path);
      }

   logmsg(kLINFO, "Pruning old backups");
   //bool getFolders(const std::string & parent, std::vector<std::string> & folders)


   return kRSuccess;
}

eResult dbackup::configure(std::string path) const
{
   backupConfig config;
   config.load();

   // TODO: check path exists and make canonical.
   if (!utils::fileexists(path))
      logmsg(kLERROR, "The path " + path + " does not exist.");
   path = utils::getcanonicalpath(path);

   if (utils::stringisame(config.getBackupPath(), path))
   {
      logmsg(kLINFO, "Path unchanged.");
      return kRNoChange;
   }

   config.update(path);
   if (!config.save())
      logmsg(kLERROR, "Unable to save backup configuration file.");

   return kRSuccess;
}

eResult dbackup::info() const
{
   backupConfig config;
   if (!config.load())
   {
      logmsg(kLINFO, "Backups are not yet configured.");
      return kRNoChange;
   }

   std::vector<std::string> services;
   utils::getAllServices(services);

   logmsg(kLINFO, "SERVICES TO BE BACKED UP:");
   for (auto const & s : services)
   {
      if (config.isEnabled(s))
         logmsg(kLINFO, "\e[32m\u2714\e[0m " + s);
      else
         logmsg(kLINFO, "\e[31m\e[1m\u2718\e[0m " + s);
   }
   logmsg(kLINFO, " ");
   logmsg(kLINFO, "Backups will be saved to:");
   logmsg(kLINFO, " " + config.getBackupPath());

   return kRNoChange;
}

eResult dbackup::showhelp() const
{
   std::string help = R"EOF(
NAME
   dbackup

DESCRIPTION
   A dRunner plugin which provides backups across all installed dServices.

SYNOPSIS
   dbackup[COMMAND][ARGS]...

COMMANDS
   dbackup configure BACKUPPATH
   dbackup include SERVICENAME
   dbackup exclude SERVICENAME
   dbackup info
   dbackup run
)EOF";

   logmsg(kLINFO, help);

   return kRError;
}