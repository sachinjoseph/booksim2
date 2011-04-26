// $Id$
/*
Copyright (c) 2007-20[01][0-9], Trustees of The Leland Stanford Junior University
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this 
list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.
Neither the name of the Stanford University nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*config_utils.cpp
 *
 *The configuration object which contained the parsed data from the 
 *configuration file
 */

#include "booksim.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <cstdlib>

#include "config_utils.hpp"

Configuration *Configuration::theConfig = 0;

Configuration::Configuration()
{
  theConfig = this;
  _config_file = 0;
}

void Configuration::AddStrField(string const & field, string const & value)
{
  _str_map[field] = value;
}

void Configuration::Assign(string const & field, string const & value)
{
  map<string, string>::const_iterator match;
  
  match = _str_map.find(field);
  if(match != _str_map.end()) {
    _str_map[field] = value;
  } else {
    ParseError("Unknown string field: " + field);
  }
}

void Configuration::Assign(string const & field, int value)
{
  map<string, int>::const_iterator match;
  
  match = _int_map.find(field);
  if(match != _int_map.end()) {
    _int_map[field] = value;
  } else {
    ParseError("Unknown integer field: " + field);
  }
}

void Configuration::Assign(string const & field, double value)
{
  map<string, double>::const_iterator match;
  
  match = _float_map.find(field);
  if(match != _float_map.end()) {
    _float_map[field] = value;
  } else {
    ParseError("Unknown double field: " + field);
  }
}

string Configuration::GetStr(string const & field) const
{
  map<string, string>::const_iterator match;

  match = _str_map.find(field);
  if(match != _str_map.end()) {
    return match->second;
  } else {
    ParseError("Unknown string field: " + field);
    exit(-1);
  }
}

int Configuration::GetInt(string const & field) const
{
  map<string, int>::const_iterator match;

  match = _int_map.find(field);
  if(match != _int_map.end()) {
    return match->second;
  } else {
    ParseError("Unknown integer field: " + field);
    exit(-1);
  }
}

double Configuration::GetFloat(string const & field) const
{  
  map<string,double>::const_iterator match;

  match = _float_map.find(field);
  if(match != _float_map.end()) {
    return match->second;
  } else {
    ParseError("Unknown double field: " + field);
    exit(-1);
  }
}

vector<string> Configuration::GetStrArray(string const & field) const
{
  string const param_str = GetStr(field);
  return tokenize(param_str);
}

vector<int> Configuration::GetIntArray(string const & field) const
{
  vector<string> param_tokens = GetStrArray(field);
  vector<int> values;
  assert(values.empty());
  for(size_t i = 0; i < param_tokens.size(); ++i) {
    char const * const token = param_tokens[i].c_str();
    int const value = atoi(token);
    values.push_back(value);
    assert(values[i] == value);
  }
  assert(values.size() == param_tokens.size());
  return values;
}

vector<double> Configuration::GetFloatArray(string const & field) const
{
  vector<string> param_tokens = GetStrArray(field);
  vector<double> values;
  assert(values.empty());
  for(size_t i = 0; i < param_tokens.size(); ++i) {
    char const * const token = param_tokens[i].c_str();
    double const value = atof(token);
    values.push_back(value);
    assert(values[i] == value);
  }
  assert(values.size() == param_tokens.size());
  return values;
}

void Configuration::ParseFile(string const & filename)
{
  if((_config_file = fopen(filename.c_str(), "r")) == 0) {
    cerr << "Could not open configuration file " << filename << endl;
    exit(-1);
  }

  configparse();

  fclose(_config_file);
  _config_file = 0;
}

void Configuration::ParseString(string const & str)
{
  _config_string = str + ';';
  configparse();
  _config_string = "";
}

int Configuration::Input(char * line, int max_size)
{
  int length = 0;

  if(_config_file) {
    length = fread(line, 1, max_size, _config_file);
  } else {
    length = _config_string.length();
    _config_string.copy(line, max_size);
    _config_string.clear();
  }

  return length;
}

void Configuration::ParseError(string const & msg, unsigned int lineno) const
{
  if(lineno) {
    cerr << "Parse error on line " << lineno << " : " << msg << endl;
  } else {
    cerr << "Parse error : " << msg << endl;
  }


  exit( -1 );
}

Configuration * Configuration::GetTheConfig()
{
  return theConfig;
}

vector<string> Configuration::tokenize(string data)
{
  string const separator = "{,}";
  vector<string> values;
  size_t last_pos = data.find_first_not_of(separator);
  size_t pos = data.find_first_of(separator, last_pos);
  while(pos != string::npos || last_pos != string::npos) {
    string const value = data.substr(last_pos, pos - last_pos);
    values.push_back(value);
    last_pos = data.find_first_not_of(separator, pos);
    pos = data.find_first_of(separator, last_pos);
  }
  return values;
}

//============================================================

int config_input(char * line, int max_size)
{
  return Configuration::GetTheConfig()->Input(line, max_size);
}

bool ParseArgs(Configuration * cf, int argc, char * * argv)
{
  bool rc = false;

  //all dashed variables are ignored by the arg parser
  for(int i = 1; i < argc; ++i) {
    string arg(argv[i]);
    size_t pos = arg.find('=');
    bool dash = (argv[i][0] =='-');
    if(pos == string::npos && !dash) {
      // parse config file
      cf->ParseFile( argv[i] );
      ifstream in(argv[i]);
      cout << "BEGIN Configuration File: " << argv[i] << endl;
      while (!in.eof()) {
	char c;
	in.get(c);
	cout << c ;
      }
      cout << "END Configuration File: " << argv[i] << endl;
      rc = true;
    } else if(pos != string::npos)  {
      // override individual parameter
      cout << "OVERRIDE Parameter: " << arg << endl;
      cf->ParseString(argv[i]);
    }
  }

  return rc;
}


//helpful for the GUI, write out nearly all variables contained in a config file.
//However, it can't and won't write out  empty strings since the booksim yacc
//parser won't be abled to parse blank strings
void Configuration::WriteFile(string const & filename) {
  
  ostream *config_out= new ofstream(filename.c_str());
  
  
  for(map<string,string>::const_iterator i = _str_map.begin(); 
      i!=_str_map.end();
      i++){
    //the parser won't read empty strings
    if(i->second[0]!='\0'){
      *config_out<<i->first<<" = "<<i->second<<";"<<endl;
    }
  }
  
  for(map<string, int>::const_iterator i = _int_map.begin(); 
      i!=_int_map.end();
      i++){
    *config_out<<i->first<<" = "<<i->second<<";"<<endl;

  }

  for(map<string, double>::const_iterator i = _float_map.begin(); 
      i!=_float_map.end();
      i++){
    *config_out<<i->first<<" = "<<i->second<<";"<<endl;

  }
  config_out->flush();
  delete config_out;
 
}



void Configuration::WriteMatlabFile(ostream * config_out) const {

  
  
  for(map<string,string>::const_iterator i = _str_map.begin(); 
      i!=_str_map.end();
      i++){
    //the parser won't read blanks lolz
    if(i->second[0]!='\0'){
      *config_out<<"%"<<i->first<<" = \'"<<i->second<<"\';"<<endl;
    }
  }
  
  for(map<string, int>::const_iterator i = _int_map.begin(); 
      i!=_int_map.end();
      i++){
    *config_out<<"%"<<i->first<<" = "<<i->second<<";"<<endl;

  }

  for(map<string, double>::const_iterator i = _float_map.begin(); 
      i!=_float_map.end();
      i++){
    *config_out<<"%"<<i->first<<" = "<<i->second<<";"<<endl;

  }
  config_out->flush();

}
