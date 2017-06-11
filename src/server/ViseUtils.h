/** @file   ViseUtils.h
 *  @brief  common utility methods
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   10 June 2017
 */

#ifndef _VISE_UTILS_H
#define _VISE_UTILS_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>

#include <boost/filesystem.hpp>  // to query/update filesystem

namespace ViseUtils {

  // file i/o
  int FileLoad(const std::string filename, std::string &file_contents);
  void FileWrite(std::string filename, std::string &file_contents);

  // string manipulation
  bool StringReplace(std::string &s, std::string old_str, std::string new_str);
  void StringSplit( const std::string s, char sep, std::vector<std::string> &tokens );
  bool StringStartsWith( const std::string &s, const std::string &prefix );
  void StringHttpUnescape( std::string &s );
  void StringParseKeyValue(std::string keyvalue_str, char separator, std::map< std::string, std::string >& keyvalue);

  // HTTP
  static const std::string crlf  = "\r\n";
  static const std::string crlf2 = "\r\n\r\n";

  bool HttpGetHeaderValue(const std::string& header, const std::string& name, std::string& value);
  std::string HttpGetFileContentType( const boost::filesystem::path& fn);
}

#endif /* _VISE_UTILS_H */
