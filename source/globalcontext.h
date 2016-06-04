#ifndef __GLOBALCONTEXT_H
#define __GLOBALCONTEXT_H

#include <string>
#include <memory>

#include "enums.h"
#include "params.h"
#include "sh_drunnercfg.h"


class GlobalContext
{
public:
   static std::shared_ptr<const params> getParams();
   static std::shared_ptr<const sh_drunnercfg> getSettings();

   static bool hasParams();
   static bool hasSettings();

   static void init(int argc, char **argv);
   GlobalContext();

   static std::shared_ptr<const params> s_params;
   static std::shared_ptr<const sh_drunnercfg> s_settings;
};

#endif
