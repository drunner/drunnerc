#include <sstream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "yaml-cpp/yaml.h"

#include "drunnercompose.h"
#include "utils.h"
#include "globallogger.h"
#include "service.h"

// ---------------------------------------------------------------------------------------------------
//

drunnerCompose::drunnerCompose(const service & svc) : 
   mService(svc), 
   mReadOkay(kRError)
{
   load_docker_compose_yml();
}

void drunnerCompose::setenv_log(std::string key, std::string val) const
{
   setenv(key.c_str(), val.c_str(), 1);
   logmsg(kLDEBUG, "environment: "+key + "=" + val);
}

void drunnerCompose::setServiceRunnerEnv() const
{
   // load the custom env variables set by the dService.
   const cServiceEnvironment & customEnv(getService().getEnvironmentConst());

   for (unsigned int i = 0; i < customEnv.getNumVars(); ++i)
   {
      std::string key, value;
      key = customEnv.index2key(i);
      value = customEnv.get_value(key);
      setenv_log(key.c_str(), value.c_str());
   }

   // load some standard ones that we make available.
   setenv_log("SERVICENAME", getService().getName().c_str());
   setenv_log("IMAGENAME", getService().getImageName().c_str());
   setenv_log("SERVICETEMPDIR", getService().getPathTemp().c_str());
   setenv_log("SERVICEHOSTVOL", getService().getPathHostVolume_servicerunner().c_str());
   setenv_log("HOSTIP", utils::getHostIP().c_str());
}

cResult drunnerCompose::readOkay() const
{
   return mReadOkay;
}

const std::vector<cServiceInfo>& drunnerCompose::getServicesInfo() const
{
   return mServicesInfo;
}

const void drunnerCompose::getVolumes(std::vector<cVolInfo> & vecvols) const
{
   for (auto entry : mServicesInfo)
      for (auto vol : entry.mVolumes)
         vecvols.push_back(vol);
}

void drunnerCompose::getDockerVolumeNames(tVecStr & dv) const
{
   for (auto entry : mServicesInfo)
      for (auto vol : entry.mVolumes)
         dv.push_back(vol.mDockerVolumeName);
}

std::string drunnerCompose::getImageName() const
{
   return mService.getImageName();
}

const service & drunnerCompose::getService() const
{
   return mService;
}

void drunnerCompose::load_docker_compose_yml()
{
   if (!utils::fileexists(mService.getPathDockerCompose()))
   {
      mReadOkay = kRNotImplemented;
      return;
   }

   std::vector<cVolInfo> VolumesInfo;

   logmsg(kLDEBUG, "Parsing " + mService.getPathDockerCompose());

   YAML::Node config = YAML::LoadFile(mService.getPathDockerCompose());
   if (!config)
      logmsg(kLERROR, "Failed to load the docker-compose.yml file. Parse error?");

   if (!config["version"] || config["version"].as<int>() != 2)
      logmsg(kLERROR, "dRunner requires the docker-compose.yml file to be Version 2 format.");

   // parse volumes.
   if (config["volumes"])
   {
      YAML::Node volumes = config["volumes"];
      if (volumes.Type() != YAML::NodeType::Map)
         logmsg(kLERROR, "docker-compose.yml is malformed - volumes is not a map.\n (Do you have an empty volumes: section? If so, delete it!).");
      for (auto it = volumes.begin(); it != volumes.end(); ++it)
      {
         cVolInfo volinfo;
         volinfo.mLabel = it->first.as<std::string>();
         YAML::Node external = it->second["external"];
         if (!external || !utils::findStringIC(volinfo.mLabel,"drunner"))
            logmsg(kLDEBUG, "Volume " + volinfo.mLabel + " is not managed by dRunner.");
         else
         {
            if (!external["name"]) logmsg(kLERROR, "Volume " + volinfo.mLabel + " is missing a required name:");
            volinfo.mDockerVolumeName = external["name"].as<std::string>();
            // need to var substitute ${SERVICENAME} in it.
            volinfo.mDockerVolumeName = utils::replacestring(volinfo.mDockerVolumeName, "${SERVICENAME}", mService.getName());
            logmsg(kLDEBUG, "dRunner managed volume: " + volinfo.mDockerVolumeName);
            VolumesInfo.push_back(volinfo);
         }
      }

      if (VolumesInfo.size() == 0)
         logmsg(kLDEBUG, "There are no dRunner managed volumes.");
   }
   else
      logmsg(kLDEBUG, "No volumes are specified in the docker-compose.yml file.\n That means dRunner won't manage any.");

   // parse services.
   if (config["services"])
   {
      YAML::Node services = config["services"];
      if (services.Type() != YAML::NodeType::Map)
         logmsg(kLERROR, "docker-compose.yml is malformed - services is not a map.");

      for (auto it = services.begin(); it != services.end(); ++it)
      {
         cServiceInfo sinf;

         sinf.mDockerServiceName = it->first.as<std::string>();
         logmsg(kLDEBUG, "Docker-compose service " + sinf.mDockerServiceName + " found.");

         if (!it->second["image"])
            logmsg(kLERROR, "Docker-compose service " + sinf.mDockerServiceName + " is missing image definition.");
         sinf.mImageName = it->second["image"].as<std::string>();
         logmsg(kLDEBUG, sinf.mDockerServiceName + " - Image:  " + sinf.mImageName);

         YAML::Node volumes = it->second["volumes"];
         if (volumes.IsNull())
            logmsg(kLDEBUG, sinf.mDockerServiceName + " - No volumes defined.");
         else
            for (auto vol = volumes.begin(); vol != volumes.end(); ++vol)
            {
               cServiceVolInfo volinfo;
               std::string v = vol->as<std::string>();
               size_t pos = v.find(':');
               if (pos == std::string::npos || pos==0 || pos==v.length()-1)
                  logmsg(kLERROR, "Couldn't parse volume info from service " + sinf.mDockerServiceName + " - missing : in " + v);

               volinfo.mLabel = v.substr(0, pos);
               volinfo.mMountPath = v.substr(pos + 1);
               volinfo.mDockerVolumeName = "";
               for (auto entry : VolumesInfo)
                  if (entry.mLabel == volinfo.mLabel)
                     volinfo.mDockerVolumeName = entry.mDockerVolumeName;
               if (volinfo.mDockerVolumeName.length() == 0)
                  logmsg(kLDEBUG, "Volume " + volinfo.mLabel + " is not managed by dRunner. Skipped.");
               else
               { // it's a dRunner volume.
                  logmsg(kLDEBUG, sinf.mDockerServiceName + " - Volume " + volinfo.mDockerVolumeName + " is to be mounted at " + volinfo.mMountPath);
                  sinf.mVolumes.push_back(volinfo);
               }
            }
         mServicesInfo.push_back(sinf);
      }
   }
   else
      logmsg(kLDEBUG, "No services are specified in the docker-compose.yml file.\n That means dRunner will just use "+mService.getImageName());

   for (auto vol : VolumesInfo)
   {
      bool matched = false;
      for (auto svc : mServicesInfo)
         for (auto svol : svc.mVolumes)
            if (svol.mLabel == vol.mLabel)
               matched = true;

      if (!matched)
         logmsg(kLERROR, "dRunner volume " + vol.mLabel + " does not appear in any service defition.\nThis is treated as an error (no known use case).");
   }

   mReadOkay = kRSuccess;
}

// std::string dvol = "drunner-" + drc.getService().getName() + "-" + entry.mServiceName + "-" + utils::alphanumericfilter(vol.mMountPath[i], false);


