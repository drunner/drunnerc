#ifndef __DOCKER_COMPOSE_H
#define __DOCKER_COMPOSE_H

#include <string>

#include "params.h"
#include "service.h"
#include "utils.h"

//void InstallDockerCompose(const params & p);

class cVolInfo {
public:
   std::string mLabel;
   std::string mDockerVolumeName;
};

class cServiceVolInfo : public cVolInfo {
public:
   std::string mMountPath;
};

class cServiceInfo {
public:
   std::string mDockerServiceName;
   std::string mImageName;
   std::vector<cServiceVolInfo> mVolumes;
};

class drunnerCompose {
public:
   // reads in docker-compose.yml - if present.
   drunnerCompose(const service & svc);

   // Set the variables in the environemnt for servicerunner.
   void setServiceRunnerEnv() const;

   // was the docker-compose.yml file read without errors.
   // returns kRNotImplemented if there was no docker-compose.yml
   cResult readOkay() const;

   // Services are as defined in the docker-compose.yml. These are not dServices.
   const std::vector<cServiceInfo> & getServicesInfo() const;

   // Volumes are as defined in the docker-compose.yml. These include all the volumes
   // defined in services, but may include extras.
   const void getVolumes(std::vector<cVolInfo> & vecvols) const;

   // get a vector of mDockerVolumeName's (for convenience) - straight from mVolumes.
   void getDockerVolumeNames(tVecStr & dv) const;

   // just queries the stored service for conveninece.
   std::string getImageName() const;

   const service & getService() const;

private:
   void load_docker_compose_yml();
   //void setvecenv(const sb_vec & v) const;
   void setenv_log(std::string key, std::string val) const;

   std::vector<cServiceInfo> mServicesInfo;

   const service & mService;
   cResult mReadOkay;
};

#endif
