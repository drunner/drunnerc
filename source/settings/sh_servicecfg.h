#ifndef __SH_SERVICECFG_H
#define __SH_SERVICECFG_H

#include "service.h"

   class sh_servicecfg : public settingsbash
   {
   public:
      // read ctor
      sh_servicecfg(const params & p, const service & svc)
         :  settingsbash(p,svc.getPathServiceCfg())
      {
         std::vector<std::string> nothing;
         setVec("VOLUMES",nothing);
         setVec("EXTRACONTAINERS",nothing);

         bool readok = readSettings();
         if (!readok)
            logmsg(kLERROR,"Not a valid dService. Couldn't read "+svc.getPathServiceCfg(),p);
      } // ctor

      const std::vector<std::string> & getVolumes() const
         {return getVec("VOLUMES");}
      const std::vector<std::string> & getExtraContainers() const
         {return getVec("EXTRACONTAINERS");}
   }; //class



#endif
