#ifndef __sh_drunnercfg_H
#define __sh_drunnercfg_H

#include <map>

#include "settingsbash.h"
#include "params.h"

class sh_drunnercfg : protected settingsbash
{
public:
   sh_drunnercfg(const std::string & rootpath); // sets defaults, loads if able.

   std::string getPath_Root()         const { return getString("ROOTPATH"); }
   std::string getPath_dServices()    const { return getPath_Root() + "/dServices"; }
   std::string getPath_Support()      const { return getPath_Root() + "/support"; }
   std::string getPath_Temp()         const { return getPath_Root() + "/temp"; }
   std::string getPath_HostVolumes()  const { return getPath_Root() + "/hostVolumes"; }

   std::string getRootUtilImage()     const { return getString("ROOTUTILIMAGE");       }
   std::string getdrunnerInstallURL() const { return getString("DRUNNERINSTALLURL");   }
   std::string getdrunnerInstallTime()const { return getString("DRUNNERINSTALLTIME");  }
   bool getPullImages() const               { return getBool("PULLIMAGES"); }

   std::string getPath_drunnercfg_sh() const { return getPath_Root() + "/drunnercfg.sh"; }

   bool readSettings();
   bool writeSettings() const;

   mutable bool mReadOkay;
};

#endif
