#ifndef __COMMAND_DEV_H
#define __COMMAND_DEV_H

#include "params.h"
#include "sh_drunnercfg.h"

namespace command_dev
{
   void build(const std::string & thedir=""); // if thedir is not specified it uses the present working directory.
}

#endif
