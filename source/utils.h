#ifndef __UTILS_H
#define __UTILS_H

#include <vector>
#include <string>
#include <sys/stat.h>

#include "enums.h"

typedef std::vector<std::string> tVecStr;

// Shell utilities
namespace utils
{
   const std::string kCODE_S="\e[32m";
   const std::string kCODE_E="\e[0m";

   std::string getabsolutepath(std::string path);
   std::string getcanonicalpath(std::string path);

   eResult mkdirp(std::string path);
   void makedirectory(const std::string & d, mode_t mode);
   void makesymlink(const std::string & file, const std::string & link);
   void deltree(const std::string & s);
   void delfile(const std::string & fullpath);

   bool fileexists (const std::string& name);
   bool commandexists(std::string command);

   std::string getUSER();

   bool canrundocker(std::string username);
   bool isindockergroup(std::string username);

   std::string replacestring(std::string subject, const std::string& search, const std::string& replace);

   int execv(std::string command, tVecStr & args);
   int bashcommand(std::string command, std::string & output);
//   int bashcommand(std::string command, const std::vector<std::string> & args, bool printstdout, bool printstderr);
   int dServiceCmd(std::string command, const std::vector<std::string> & args, bool isServiceCmd=false);

   std::string trim_copy(std::string s, const char* t = " \t\n\r\f\v");
   std::string& trim(std::string& s, const char* t = " \t\n\r\f\v");
   std::string doquote(std::string s);

   // case insensitive comparison.
   bool stringisame(const std::string & s1, const std::string &s2 );

   std::string get_exepath();
   std::string get_exefullpath();
   std::string get_exename();
   std::string get_usersbindir();   // dies if fails.

   bool imageisbranch(const std::string & imagename);
   eResult pullimage(const std::string & imagename);

   bool getFolders(const std::string & parent, std::vector<std::string> & services);

   bool isInstalled();

   bool findStringIC(const std::string & strHaystack, const std::string & strNeedle);

   std::string getHostIP();

   std::string getTime();
   std::string getPWD();
   bool dockerVolExists(const std::string & vol);

   std::string getenv(std::string envParam);

   bool copyfile(std::string src, std::string dest);

   void downloadexe(std::string url, std::string filepath);

   std::string alphanumericfilter(std::string s, bool whitespace);
   
   class tempfolder
   {
   public:
      tempfolder(std::string d);
      ~tempfolder();
      const std::string & getpath();
      
   private:
      void die(std::string msg);
      void tidy();
      std::string mPath;
   };

   class dockerrun
   {
   public:
      dockerrun(const std::string & cmd, const std::vector<std::string> & args, std::string dockername);
      ~dockerrun();

   private:
      void tidy();

      std::string mDockerName;
   };

} // namespace


#endif
