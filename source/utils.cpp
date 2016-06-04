#include <string>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <memory>
#include <utility>
#include <errno.h>
#include <stdlib.h>

#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>

#include <sys/stat.h>
#include <unistd.h>

#include "pstream.h"
#include "utils.h"
#include "exceptions.h"
#include "dservicelogger.h"
#include "globallogger.h"
#include "globalcontext.h"
#include "enums.h"

namespace utils
{

   bool fileexists (const std::string& name)
   {
      struct stat buffer;
      return (stat (name.c_str(), &buffer) == 0);
   }

   bool stringisame(const std::string & s1, const std::string &s2 )
   {
      return boost::iequals(s1,s2);
   }

   // trim from left
   inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v")
   {
       s.erase(0, s.find_first_not_of(t));
       return s;
   }

   // trim from right
   inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v")
   {
       s.erase(s.find_last_not_of(t) + 1);
       return s;
   }

   // trim from left & right
   std::string& trim(std::string& s, const char* t)
   {
       return ltrim(rtrim(s, t), t);
   }

   std::string doquote(std::string s)
   {
      return "\""+s+"\"";
   }

   // copying versions

   inline std::string ltrim_copy(std::string s, const char* t = " \t\n\r\f\v")
   {
       return ltrim(s, t);
   }

   inline std::string rtrim_copy(std::string s, const char* t = " \t\n\r\f\v")
   {
       return rtrim(s, t);
   }

   std::string trim_copy(std::string s, const char* t)
   {
       return utils::trim(s, t);
   }

   int execv(std::string command, tVecStr & args)
   {
      std::vector<char*> argv;
      for (auto & arg : args)
         argv.push_back( &arg.front() );
      argv.push_back(NULL);
      return ::execv(command.c_str(), argv.data());
   }

   int bashcommand(std::string command, std::string & output)
   {
      redi::ipstream in(command);
      std::string str;
      while (in >> str)
         output+=trim_copy(str)+" ";
      in.close();
      utils::trim(output);
      int status = in.rdbuf()->status();
      return WEXITSTATUS(status);
   }

   int dServiceCmd(std::string command, const std::vector<std::string> & args, bool isServiceCmd)
   { // non-blocking streaming
      { // sanity check parameters.
         if (args.size() < 1)
            fatal("dServiceCmd: arguments must include the actual command being run.");
         boost::filesystem::path bfp(command);
         if (bfp.filename().string() != args[0])
            fatal("dServiceCmd: command doesn't match args - " + bfp.filename().string() + " versus " + args[0]);
      }

      { // log the command, getting the args right is non-trivial in some cases so this is useful.
         std::string cmd;
         for (const auto & entry : args)
            cmd += "[" + entry + "] ";
         logmsg(kLDEBUG, "dServiceCmd: " + cmd);
      }

      dServiceLogger logcout(false, isServiceCmd);
      dServiceLogger logcerr(true, isServiceCmd);

      const redi::pstreams::pmode mode = redi::pstreams::pstdout | redi::pstreams::pstderr;
      redi::ipstream child(command, args, mode);
      if (!child.is_open())
         fatal("Couldn't run " + command);

      char buf[1024];
      std::streamsize n;
      bool finished[2] = { false, false };
      while (!finished[0] || !finished[1])
      {
         if (!finished[0])
         {
            while ((n = child.err().readsome(buf, sizeof(buf))) > 0)
               logcerr.log(buf, n);
//                  std::cerr.write(buf, n).flush();
            if (child.eof())
            {
               finished[0] = true;
               if (!finished[1])
                  child.clear();
            }
         }

         if (!finished[1])
         {
            while ((n = child.out().readsome(buf, sizeof(buf))) > 0)
               logcout.log(buf, n);
//                  std::cout.write(buf, n).flush();
            if (child.eof())
            {
               finished[1] = true;
               if (!finished[0])
                  child.clear();
            }
         }
      }

      child.rdbuf()->close();
      int status = child.rdbuf()->status();
      int rval= WEXITSTATUS(status); // return child status.

      std::ostringstream oss;
      oss << args[0] << " returned " << rval;
      logmsg(kLDEBUG, oss.str());

      return rval;
   }

   std::string getabsolutepath(std::string path)
   {
      boost::filesystem::path rval;
      try
      {
         rval = boost::filesystem::absolute(path);
      }
      catch(...)
      {
         return path;
      }
      return rval.string();
   }

   std::string getcanonicalpath(std::string path)
   {
      boost::filesystem::path rval;
      try
      {
         rval = boost::filesystem::canonical(path);
      }
      catch(...)
      {
         return "";
      }
      return rval.string();
   }


   eResult mkdirp(std::string path)
   {
      if (fileexists(path))
         return kRNoChange;
      try
      {
         boost::filesystem::create_directories(path);
      }
      catch (...)
      {
         return kRError;
      }
      return kRSuccess;
   }


   std::string replacestring(std::string subject, const std::string& search,
        const std::string& replace)
   {
      size_t pos = 0;
      if (search.empty() || subject.empty())
         return "";
      while((pos = subject.find(search, pos)) != std::string::npos)
      {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
      }
      return subject;
   }

   bool isindockergroup(std::string username)
   {
      std::string op;
      int rval = bashcommand("groups $USER | grep docker",op);
      return (rval==0);
   }

   bool canrundocker(std::string username)
   {
      std::string op;
      int rval = bashcommand("groups | grep docker",op);
      return (rval==0);
   }

   std::string getUSER()
   {
      std::string op;
      int rval = bashcommand("echo $USER",op);
      if (rval!=0)
         logmsg(kLERROR,"Couldn't get current user.");
      return op;
   }

   bool commandexists(std::string command)
   {
      std::string op;
      int rval = bashcommand("command -v "+command,op);
      return (rval==0);
   }

   std::string get_exefullpath()
   {
      char buff[PATH_MAX];
      ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
      if (len != -1)
      {
         buff[len] = '\0';
         return std::string(buff);
      }
      logmsg(kLERROR,"Couldn't get path to drunner executable!");
      return "";
   }

   std::string get_exename()
   {
      boost::filesystem::path p( get_exefullpath() );
      return p.filename().string();
   }

   std::string get_exepath()
   {
      boost::filesystem::path p( get_exefullpath() );
      return p.parent_path().string();
   }

   std::string get_usersbindir()
   {
      std::string op;
      int rval = bashcommand("echo $HOME",op);
      if (rval!=0)
         logmsg(kLERROR,"Couldn't get current user's home directory.");
      return op+"/bin";
   }

   bool imageisbranch(const std::string & imagename)
   {
      std::size_t pos = imagename.find_last_of("/:");
      if (pos == std::string::npos || imagename[pos] != ':')
         return false;

      std::string branchname=imagename.substr(pos+1);
      if (stringisame(branchname,"master"))
         return false;

      return true;
   }

   eResult pullimage(const std::string & imagename)
   {
      std::string op;

      int rval = bashcommand("docker pull "+imagename, op);

      if (rval==0 && op.find("Image is up to date",0) != std::string::npos)
         return kRNoChange;

      return (rval==0) ? kRSuccess : kRError;
   }



   bool getFolders(const std::string & parent, std::vector<std::string> & services)
   {
      boost::filesystem::path dir_path(parent);
      if ( ! boost::filesystem::exists( dir_path ) ) return false;

      boost::filesystem::directory_iterator itr(dir_path),end_itr; // default construction yields past-the-end
      for ( ; itr != end_itr; ++itr )
      {
         if ( boost::filesystem::is_directory(itr->status()) )
            services.push_back(itr->path().filename().string());
      }
      return true;
   }

   // quick crude check to see if we're installed.
   bool isInstalled()
   {
      std::string rootpath = get_exepath();
      return (boost::filesystem::exists(rootpath + "/" + "drunnercfg.sh"));
   }


   /// Try to find in the Haystack the Needle - ignore case
   bool findStringIC(const std::string & strHaystack, const std::string & strNeedle)
   {
     auto it = std::search(
       strHaystack.begin(), strHaystack.end(),
       strNeedle.begin(),   strNeedle.end(),
       [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
     );
     return (it != strHaystack.end() );
   }


   void makedirectory(const std::string & d, mode_t mode)
   {
      eResult rslt = utils::mkdirp(d);
      if (rslt==kRError)
         logmsg(kLERROR,"Couldn't create "+d);
      if (rslt==kRSuccess)
         logmsg(kLDEBUG,"Created "+d);
      if (rslt==kRNoChange)
         logmsg(kLDEBUG,d+" exists. Unchanged.");

      if (chmod(d.c_str(), mode)!=0)
         logmsg(kLERROR, "Unable to change permissions on "+d);
   }

   void makesymlink(const std::string & file, const std::string & link)
   {
	if (!utils::fileexists(file))
		logmsg(kLERROR, "Can't link to " + file + " because it doesn't exist");
	if (utils::fileexists(link))
		if (remove(link.c_str()) != 0)
			logmsg(kLERROR, "Couldn't remove stale symlink at " + link);
	std::string cmd = "ln -s " + file + " " + link;
	std::string op;
	if (utils::bashcommand(cmd, op) != 0)
		logmsg(kLERROR, "Failed to create symbolic link for drunner. "+op);
   }

   void deltree(const std::string & s)
   {
      std::string op;
      if (fileexists(s))
      {
         if (bashcommand("rm -rf "+s+" 2>&1", op) != 0)
            logmsg(kLERROR, "Unable to remove existing directory at "+s+" - "+op);
         logmsg(kLDEBUG,"Recursively deleted "+s);
      }
      else
         logmsg(kLDEBUG,"Directory "+s+" does not exist (no need to delete).");
   }

   void delfile(const std::string & fullpath)
   {
      if (utils::fileexists(fullpath))
         {
         std::string op;
         if (bashcommand("rm -f "+fullpath+" 2>&1", op) != 0)
            logmsg(kLERROR, "Unable to remove "+fullpath + " - "+op);
         logmsg(kLDEBUG,"Deleted "+fullpath);
         }
   }

   std::string getHostIP()
   {
      std::string hostIP;
      if (utils::bashcommand("ip route get 1 | awk '{print $NF;exit}'", hostIP) != 0)
         return "";
      return hostIP;
   }

   std::string getTime()
   {
      std::time_t rtime = std::time(nullptr);
      return utils::trim_copy(std::asctime(std::localtime(&rtime)));
   }

   std::string getPWD()
   {
      char p[300];
      getcwd(p,300);
      return std::string(p);
   }

   bool dockerVolExists(const std::string & vol)
   { // this could be better - will match substrings rather than whole volume name. :/ 
      // We name with big unique names so unlikely to be a problem for us.
      std::string op;
      int rval = utils::bashcommand("docker volume ls | grep \"" + vol + "\"", op);
      return (rval == 0); 
   }

   std::string getenv(std::string envParam)
   {
      const char * cstr = std::getenv(envParam.c_str());
      std::string r;
      if (cstr != NULL)
         r = std::string(cstr);
      return r;
   }

   bool copyfile(std::string src, std::string dest)
   {
      // boost bug makes copy_file grumpy with c++11x.
      // also can't copy to another filesystem.
      // so we just use bash.

      std::string op;
      int r = bashcommand("cp -a " + src + " " + dest,op);
      return (r == 0);
   }

   void downloadexe(std::string url, std::string filepath)
   {
      std::string op;

      // only download if server has newer version.      
      int rval = utils::bashcommand("curl " + url + " -z " + filepath + " -o " + filepath + " --silent --location 2>&1 && chmod 0755 " + filepath, op);
      if (rval != 0)
         logmsg(kLERROR, "Unable to download " + url);
      if (!utils::fileexists(filepath))
         logmsg(kLERROR, "Failed to download " + url + ", curl return value is success but expected file "+ filepath +" does not exist.");
      logmsg(kLDEBUG, "Successfully downloaded " + filepath);
   }

   std::string alphanumericfilter(std::string s, bool whitespace)
   {
      std::string validchars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
      if (whitespace) validchars += " \n";
      size_t pos;
      while ((pos = s.find_first_not_of(validchars)) != std::string::npos)
         s.erase(pos, 1);
      return s;
   }


   tempfolder::tempfolder(std::string d) : mPath(d)
   {   // http://stackoverflow.com/a/10232761
      eResult rslt = utils::mkdirp(d);
      if (rslt == kRError)
         die("Couldn't create " + d);
      if (rslt == kRSuccess)
         logmsg(kLDEBUG, "Created " + d);
      if (rslt == kRNoChange)
         die(d+ " already exists. Can't use as temp folder. Aborting.");

      if (chmod(d.c_str(), S_777) != 0)
         die("Unable to change permissions on " + d);
   }

   tempfolder::~tempfolder() 
   {
      tidy();
   }

   const std::string & tempfolder::getpath() 
   { 
      return mPath; 
   }

   void tempfolder::die(std::string msg)
   {
      tidy();
      logmsg(kLERROR, msg); // throws. dtor won't be called since die is only called from ctor.
   }
   void tempfolder::tidy()
   {
      std::string op;
      if (bashcommand("rm -rf " + mPath + " 2>&1", op) != 0)
         std::cerr << "ERROR: failed to remove " + mPath << std::endl; // don't throw on dtor.
      else
         logmsg(kLDEBUG, "Recursively deleted " + mPath);
   }



   dockerrun::dockerrun(const std::string & cmd, const std::vector<std::string> & args, std::string dockername)
      : mDockerName(dockername)
   {
      int rval = utils::dServiceCmd(cmd, args);
      if (rval != 0)
      {
         std::ostringstream oss;
         for (auto entry : args)
            oss << entry << " ";
         logmsg(kLDEBUG, oss.str());
         tidy(); // throwing from ctor does not invoke dtor!
         std::ostringstream oss2;
         oss2 << "Docker command failed. Return code=" << rval;
         logmsg(kLERROR, oss2.str());
      }
   }
   dockerrun::~dockerrun()
   {
      tidy();
   }

   void dockerrun::tidy()
   {
      std::string op;
      int rval = utils::bashcommand("docker rm " + mDockerName, op);
      if (rval != 0)
         std::cerr << "failed to remove " + mDockerName << std::endl; // don't throw on dtor.
      else
         logmsg(kLDEBUG, "Deleted docker container " + mDockerName);
   }


} // namespace utils
