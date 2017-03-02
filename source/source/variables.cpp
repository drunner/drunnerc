#include <Poco/String.h>
#include <Poco/Environment.h>
#include <fstream>
#include <cereal/archives/json.hpp>

#include "variables.h"
#include "utils.h"
#include "globallogger.h"
#include "dassert.h"
#include "basen.h"


keyVals::keyVals(const keyVals & other1, const keyVals & other2)
{
   mKeyVals = other1.getAll();
   for (const auto & x : other2.getAll())
      mKeyVals[x.first] = x.second;
}

bool keyVals::hasKey(std::string key) const
{
   for (const auto & x : mKeyVals)
      if (0==Poco::icompare(key, x.first))
         return true;
   return false;
}

bool keyVals::isDefined(std::string key) const
{
   return getVal(key).length()>0;
}

std::string keyVals::getVal(std::string key) const
{
   for (const auto & x : mKeyVals)
      if (0==Poco::icompare(key, x.first))
         return x.second;
   return "";
}

bool keyVals::getBool(std::string key) const
{
   std::string s = getVal(key);
   if (s.length() == 0) return false;
   return (tolower(s[0]) == 'y' || tolower(s[0]) == 't');
}

void keyVals::setVal(std::string key, std::string val)
{
   mKeyVals[key] = val;
}

void keyVals::delKey(std::string key)
{
   mKeyVals.erase(key);
}


std::string keyVals::substitute(std::string s) const
{
   std::string os(s);
   for (auto it = mKeyVals.begin(); it != mKeyVals.end(); ++it)
   {
      Poco::replaceInPlace(os, "$" + it->first, it->second);
      Poco::replaceInPlace(os, "${" + it->first+"}", it->second);
   }
   return os;
}


// -----------------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------------


persistvariables::persistvariables(std::string name, Poco::Path path, const std::vector<envDef> config) :
   keyVals(), mName(name), mPath(path), mEnvDefs(config)
{
   // set default values
   for (const auto & x : mEnvDefs)
      setVal(x.name, x.defaultval);
}

cResult persistvariables::loadvariables()
{
   if (!exists())
      return cError("The settings file does not exist: " + mPath.toString());

   // read the settings.
   keyVals storedvars;
   std::ifstream is(mPath.toString());
   if (is.bad())
      return cError("Unable to open " + mPath.toString() + " for reading.");
   try
   {
      cereal::JSONInputArchive archive(is);
      archive(storedvars);
   }
   catch (const cereal::Exception & e)
   {
      return cError("Cereal exception on reading settings: " + std::string(e.what()));
   }

   // check all items are (1) listed in config, and (2) valid.
   for (auto x : storedvars.getAll())
      setVal(x.first, x.second);

   return kRSuccess;
}

cResult persistvariables::savevariables() const
{
   logdbg("Creating " + mPath.toString());
   std::ofstream os(mPath.toString());
   if (os.bad() || !os.is_open())
   {
      logmsg(kLDEBUG, "Unable to open " + mPath.toString() + " for writing.");
      return cError("Unable to open " + mPath.toString() + " for writing.");
   }

   // extract out all the variables that are to be persisted.
   keyVals vars;
   for (auto x : getAll())
      if (std::unique_ptr<envDef> def = getDef(x.first))
      {
         if (def->persists)
            vars.setVal(x.first, x.second);
      }
      else
         logmsg(kLWARN, "Defined variable " + x.first + " does not appear in environment definitions. Not persisted.");

   try
   {
      cereal::JSONOutputArchive archive(os);
      archive(vars);
   }
   catch (const cereal::Exception & e)
   {
      logmsg(kLDEBUG, "Cereal exception on writing settings: " + std::string(e.what()));
      return cError("Cereal exception on writing settings: " + std::string(e.what()));
   }

   if (!utils::fileexists(mPath))
      logmsg(kLWARN, "Failed to create blank settings at " + mPath.toString());

   return kRSuccess;
}

cResult persistvariables::setVal(std::string key, std::string val)
{
   for (const auto & x : mEnvDefs)
      if (Poco::icompare(x.name, key) == 0)
      {
         keyVals::setVal(x.name, val);
         return kRSuccess;
      }

   return cError("Configuration value '" + key + "' is not recognised.");
}

cResult persistvariables::_showconfiginfo() const
{ // show current variables.
   logmsg(kLINFO, "Current configuration:\n ");

   int maxkey = 0;
   for (const auto & y : getAll())
      maxkey = utils::_max(maxkey, y.first.length());

   int uservars = 0;
   for (const auto & y : getAll())
      if (std::unique_ptr<envDef> def = getDef(y.first))
      {
         if (def->usersettable)
         {
            logmsg(kLINFO, " " + utils::_pad(y.first, maxkey) + " = " + (def->secret ? "xxxxxx" : y.second));
            logmsg(kLINFO, " " + utils::_pad(" ", maxkey) + "   " + def->description + "\n");
            ++uservars;
         }
         else
         {
            logdbg("[" + utils::_pad(y.first, maxkey) + "]= " + (def->secret ? "xxxxxx" : y.second) + " (not user settable)");
            logdbg(" " + utils::_pad(" ", maxkey) + "   " + def->description + "\n");
         }
      }
      else
         logmsg(kLWARN, "Variable " + y.first + " is set, but does not have an environment definition.");

   if (uservars == 0)
      logmsg(kLINFO, "There are no user configurable variables.");
   else
   {
      logmsg(kLINFO, " ");
      logmsg(kLINFO, "Change configuration variables with:");
      logmsg(kLINFO, " " + mName + " configure VARIABLE         -- configure from environment var");
      logmsg(kLINFO, " " + mName + " configure VARIABLE=VALUE   -- configure with specified value");
   }
   return kRSuccess;
}

cResult persistvariables::handleConfigureCommand(CommandLine cl)
{
   if (cl.args.size() == 0)
      return _showconfiginfo();

   cResult rval;
   for (const auto & kv : cl.args)
   {
      std::string key, val;
      size_t epos = kv.find('=');
      if (epos == std::string::npos)
      { // env variable
         try {
            key = kv;
            val = Poco::Environment::get(key);
         }
         catch (const Poco::Exception &)
         {
            logmsg(kLWARN, "Couldn't find environment variable " + key);
            logmsg(kLERROR, "Configuration variables must be given in form key=val or represent an environment variable.");
         }
         logmsg(kLINFO, "Setting " + key + " to value from environment [not logged].");
      }
      else
      { // form key=val.
         if (epos == 0)
            logmsg(kLERROR, "Missing key.");

         key = kv.substr(0, epos);
         val = "";
         if (epos < kv.length() - 1)
            val = kv.substr(epos + 1);
      }

      if (std::unique_ptr<envDef> def = getDef(key))
      {
         if (!def->usersettable)
            return cError("You can't override " + key);

         if (val.length() == 0)
            logmsg(kLINFO, "Clearing " + key);
         else
            logmsg(kLINFO, "Setting " + key + " to " + val);
         rval += setVal(key, val);
      }
      else
      {
         logmsg(kLDEBUG, "Configuration variable " + key + " was not recognised");
         return cError("Configuration variable " + key + " was not recognised");
      }
   }
   if (!rval.noChange())
      rval += savevariables();
   return rval;
}

void persistvariables::_addConfig(const envDef & c)
{
   mEnvDefs.push_back(c);

   if (!isDefined(c.name))
      setVal(c.name, c.defaultval);
}

std::unique_ptr<envDef> persistvariables::getDef(std::string key) const
{
   for (const auto & x : mEnvDefs)
      if (Poco::icompare(x.name, key) == 0)
         return std::unique_ptr<envDef>(new envDef(x));
   return nullptr;
}

bool persistvariables::exists() const
{
   return utils::fileexists(mPath);
}
