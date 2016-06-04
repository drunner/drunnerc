
#include "utils.h"
#include "utils_docker.h"
#include "globalcontext.h"
#include "globallogger.h"

namespace utils_docker
{

   void pullImage(const std::string & image)
   {
      if (GlobalContext::getParams()->isDevelopmentMode())
      {
         logmsg(kLDEBUG, "In developer mode - not pulling " + image);
         return;
      }

      if (!GlobalContext::getSettings()->getPullImages())
      {
         logmsg(kLDEBUG, "Pulling images disabled in global options.");
         return;
      }

      if (utils::imageisbranch(image))
      {
         logmsg(kLDEBUG, image+" is not on the master branch, so assuming dev environment and not pulling.");
         return;
      }

      logmsg(kLINFO, "Pulling Docker image " + image + ".\n This may take some time...");
      eResult rslt = utils::pullimage(image);

      switch (rslt) 
      {
      case kRError:
         logmsg(kLERROR, "Couldn't pull " + image);
         break;
      case kRNoChange:
         logmsg(kLDEBUG, "No change to Docker image (it's already up to date).");
         break;
      default:
         logmsg(kLINFO, "Successfully pulled " + image);
      }
   }


} // namespace