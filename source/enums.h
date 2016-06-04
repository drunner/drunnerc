#ifndef __ENUMS_H
#define __ENUMS_H

#include <sys/stat.h>


// some statics.

static const mode_t S_ALLREAD = S_IRUSR | S_IRGRP | S_IROTH;
static const mode_t S_755 = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
static const mode_t S_777 = S_IRWXU | S_IRWXG | S_IRWXO;
static const mode_t S_700  = S_IRWXU;
// enumerators used often.

enum eResult
{
   kRSuccess=0,
   kRError=1,
   kRNoChange=3,
   kRNotImplemented=127
};


enum eLogLevel
{
   kLDEBUG=0,
   kLINFO=1,
   kLWARN=2,
   kLERROR=3
};

enum edServiceOutput
{
   kOLogged,
   kORaw,
   kOSuppressed
};


enum eCommand {
   c_UNDEFINED,
   c_setup,
   c_clean,
   c_list,
   c_update,
   c_checkimage,
   c_backup,
   c_restore,
   c_install,
   c_recover,
   c_uninstall,
   c_obliterate,
   c_enter,
   c_status,
   c_build,
   c_unittest,
   c_servicecmd,
   c_saveenvironment,
   c_help,
};


#endif
