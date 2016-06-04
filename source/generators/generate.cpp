#include "generate.h"
#include "globallogger.h"
#include "utils.h"

void generate(
   const std::string & fullpath,
   const mode_t mode,
   const std::string & content
)
{
   // for now we force the generation. In future should check
   // whether any changes have been made.
   utils::delfile(fullpath);

   std::string op;
   std::ofstream ofs;
   ofs.open(fullpath);
   if (!ofs.is_open())
      logmsg(kLERROR,"Couldn't write to "+fullpath);
   ofs << content;
   ofs.close();

   if (chmod(fullpath.c_str(), mode)!=0)
//   if (utils::bashcommand("chmod a+r "+fullpath,op) != 0)
      logmsg(kLERROR, "Unable to change permissions on "+fullpath);
   logmsg(kLDEBUG,"Created "+fullpath);
}
