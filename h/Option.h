/* Option.h
 * This parses an option string, much like getopt()
 * getopt() takes the argc and argv given to your program by the operating system,
 * and I need this to work for an arbitrary string, or I would use getopt.
 */

#ifndef DIRT_OPTION_H
#define DIRT_OPTION_H

#include "dirt.h"
#include "config.h"
#include <limits.h>
#if HAVE_HASH_MAP && !HAVE_EXT_HASH_MAP
#include <hash_map>
#else
#include <ext/hash_map>
#endif
#include <string>
#include <vector>

using namespace std;

class OptionParser
{
public:
    OptionParser(const string& _s, const string& _options);  // does the dirty work.
    string& operator [](char c)  { return options[c]; }       // returns the argument to flag c.
                                                             //   note if flag c was not passed,
                                                             //   this will ADD AN ELEMENT TO THE
                                                             //   HASH AND RETURN IT.
    bool   flag(char c)         { return gotOpt(c) && (options[c] == "1"); }// or "1" if a boolean flag.
    string getOption(char c)    { return options[c]; }       // get an argument.
    bool   gotOpt(char c)       { return !(options.find(c) == options.end()); }
    int    argc()               { return nonOptions.size(); }
    string arg(unsigned int i);                              // get non-option argument i (returns "" 
                                                             // if the option doesn't exist.
    bool   valid()              { return isvalid; }          // indicates whether the string conforms to the option specification
    vector<string> rest()       { return nonOptions; }       // Get the non-options.
    string restStr();                                        // get non-options as a string, EXCLUDING arg(0).
    string str()                { return s; }
private:
    string s;
    string optionStr;
    string s_nonopt;
    bool   isvalid;
    vector<string> nonOptions;
    hash_map<char, string> options;
};

#endif
