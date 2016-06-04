#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <sstream>

#include "settingsbash.h"
#include "utils.h"
#include "globallogger.h"

std::string dequote(const std::string & s, char c)
{
   std::string ss(s);
   if (ss.length()==0) return ss;
   if (ss[0]==c) ss.erase(0,1);
   if (ss.length()==0) return ss;
   if (ss[ss.length()-1]==c) ss.erase(ss.length()-1,std::string::npos);
   return ss;
}

// ----------------------------------------------------------------------------------------------------------------------

bashline::bashline()
{
}

void bashline::setline(const std::string & bashline_str)
{
   std::size_t end=0;
   if ((end=bashline_str.find("=",0)) != std::string::npos)
      {
         key=utils::trim_copy(bashline_str.substr(0,end));
         value=utils::trim_copy(bashline_str.substr(end+1)); //If this is equal to the string length, substr returns an empty string.
         value = dequote(utils::trim(value), '\"');
         if (key.length()>0 && key[0]=='#')
            key.clear();
   }
}

void bashline::setkeyvalue(const std::string &k, const std::string & v)
{
   key = k;
   value = v;
}

// ----------------------------------------------------------------------------------------------------------------------

bool settingsbash::readSettings(const std::string & settingspath)
{
   if (settingspath.length() == 0)
      fatal("settingsbash::readSettings - empty string passed as path to read from.");

   if (! utils::fileexists(settingspath))
      return false;

   std::string line,left,right;
   std::ifstream configfile(settingspath.c_str() );
   while (std::getline(configfile, line))
   {
      bashline bl;
      bl.setline(line);
      if (bl.valid())
         setSetting(bl, mCreateOnRead);
   }
   configfile.close();

   return true;
}

void settingsbash::setSetting(bashline bl, bool createOK)
{
   for (unsigned int i=0;i<mElements.size();++i)
      if (utils::stringisame(mElements[i].getkey(),bl.getkey()))
      {
         mElements[i] = bl;
         return;
      }
   if (!createOK)
      fatal("Trying to create setting " + bl.getkey() + " but not permitted.");
   mElements.push_back(bl);
}


bool settingsbash::writeSettings(const std::string & settingspath) const
{
   if (settingspath.length() == 0)
      fatal("settingsbash::writeSettings - empty string passed as path to write to.");

   if (settingspath.find(".sh")==std::string::npos)
      fatal("All bash style settings files should have .sh file extension. Offender: "+ settingspath);

   if (utils::fileexists(settingspath))
      std::remove(settingspath.c_str());

   std::ofstream ofile(settingspath.c_str() );
   if (!ofile.is_open()) return false; // can't open the file.

   ofile << "# "+ settingspath << std::endl;
   ofile << "# dRunner bash configuration file." << std::endl;

  // iterate through map. C++11 style.
  for (auto const &entry : mElements) {
     ofile << entry.getline() << std::endl;
  }
  ofile.close();
  return true;
}

bool settingsbash::getBool(const std::string & key) const
{ // remove quotes and whitespace.
   return istrue(getString(key));
}

void settingsbash::setBool(const std::string & key, bool b)
{
   bashline bl;
   bl.setkeyvalue(key, b ? "yes" : "no");
   setSetting(bl, true);
}

std::string settingsbash::getString(const std::string & key) const
{
   // remove quotes and whitespace.
   std::string v = getElement(key).getvalue();
   utils::trim(v);
   return dequote(v, '\"');
}

void settingsbash::setString(const std::string & key, const std::string & s)
{
   bashline bl;
   bl.setkeyvalue(key, "\"" + s + "\"");
   setSetting(bl, true);
}

void settingsbash::getVec(const std::string & key, std::vector<std::string> & vec) const
{
   vec.clear();

   std::string astr = getElement(key).getvalue();
   astr = dequote(dequote(astr, '('), ')');
   std::stringstream ss(astr);
   while (ss.good())
   {
      ss >> astr;
      utils::trim(astr);
      std::string element = dequote(astr, '\"');
      if (element.length() > 0)
         vec.push_back(element);
   }
}

void settingsbash::setVec(const std::string & key, const std::vector<std::string> & v)
{
   std::stringstream ss;
   ss << "(";
   for (unsigned int i = 0; i < v.size(); ++i)
   {
      if (i > 0) ss << " ";
      ss << "\"" << v[i] << "\"";
   }
   ss << ")";
   bashline bl;
   bl.setkeyvalue(key, ss.str());
   setSetting(bl, true);
}

//
bool settingsbash::istrue(const std::string & s) const
{
   if (s.length()==0) return false;
   return (tolower(s[0])=='y' || tolower(s[0])=='t');
}

bashline settingsbash::getElement(const std::string & key) const
{
	for (auto const &entry : mElements)
		if (utils::stringisame(entry.getkey(), key))
			return entry;

   fatal("Couldn't find key "+key);
   return bashline();
}

//-------------------------------------------------------------------------------------------------

