#include <iostream>

#include "compress.h"
#include "utils.h"
#include "globallogger.h"
#include "params.h"

namespace compress
{
   void _rundocker(std::string src, std::string dst, std::string passwd, std::string ctrcmd)
   {
      std::string cmd("docker");
      std::vector<std::string> args;

      args.push_back(cmd);
      args.push_back("run");
      args.push_back("-i");
      args.push_back("--name=\"dr_compress\"");
      if (passwd.length() > 0)
      {
         logmsg(kLDEBUG, "Using password supplied.");
         args.push_back("-e");
         args.push_back("PASS=\"" + passwd + "\"");
      }
      args.push_back("-v");
      args.push_back(src + ":/src");
      args.push_back("-v");
      args.push_back(dst + ":/dst");
      args.push_back("drunner/rootutils");
      args.push_back("bash");
      args.push_back("-c");
      args.push_back(ctrcmd);
      
      utils::dockerrun dr(cmd, args, "dr_compress");
   }

   // --------------------------------------

   bool _compress(std::string password, std::string volumename,
      std::string archivefolder, std::string archivename)
   {
      if (!utils::fileexists(archivefolder))
         fatal("Can't archive to non-existant folder " + archivefolder);

      if (utils::fileexists(archivefolder+archivename))
         fatal("Can't compress to already existing " + archivename);

      _rundocker(volumename, archivefolder, password,
         "/usr/local/bin/dr_compress " + archivename + " && chmod 0666 /dst/" + archivename );

      return true;
   }

   // --------------------------------------

   bool _decompress(std::string password, std::string volumename,
      std::string archivefolder, std::string archivename)
   {
      if (!utils::fileexists(archivefolder) || !utils::fileexists(archivefolder + archivename))
         fatal("Can't decompress missing archive " + archivefolder + archivename);

      _rundocker(archivefolder, volumename, password,
         "/usr/local/bin/dr_decompress "+archivename );

      return true;
   }

   // --------------------------------------


   bool compress_volume(std::string password, std::string volumename,
      std::string archivefolder, std::string archivename)
   {
      if (!utils::dockerVolExists(volumename))
         fatal("Can't compress non-existant volume " + volumename);

      return _compress(password, volumename, archivefolder+"/", archivename);
   }

   // --------------------------------------

   bool compress_folder(std::string password, std::string foldername,
      std::string archivefolder, std::string archivename)
   {
      if (!utils::fileexists(foldername))
         fatal("Can't archive non-existant folder " + foldername);
      std::string ap = utils::getcanonicalpath(foldername);

      return _compress(password, ap + "/", archivefolder + "/", archivename);
   }

   // --------------------------------------

   bool decompress_volume(std::string password, std::string targetvolumename,
      std::string archivefolder, std::string archivename)
   {
      if (!utils::dockerVolExists(targetvolumename))
         fatal("Can't restore into volume " + targetvolumename + " because it doesn't exist.");

      return _decompress(password, targetvolumename, archivefolder + "/", archivename);
   }


   // --------------------------------------

   bool decompress_folder(std::string password, std::string targetfoldername,
      std::string archivefolder, std::string archivename)
   {
      if (!utils::fileexists(targetfoldername))
         fatal("Can't archive to non-existant folder " + targetfoldername);
      std::string ap = utils::getcanonicalpath(targetfoldername);

      return _decompress(password, ap + "/", archivefolder + "/", archivename);
   }


} // namespace