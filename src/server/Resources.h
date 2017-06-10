/** @file   Resources.h
 *  @brief  provides access to statis resources of VISE server
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   10 June 2017
 */

#ifndef _VISE_RESOURCES_H
#define _VISE_RESOURCES_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include <boost/filesystem.hpp>

#include "ViseUtils.h"

class Resources {
  public:
  void LoadAllResources(const boost::filesystem::path resource_dir);
  std::string& GetFileContents(const std::string filename);

  // html content
  static const std::string VISE_MAIN_HTML_FN;
  static const std::string VISE_HOME_HTML_FN;
  static const std::string VISE_HELP_HTML_FN;
  static const std::string VISE_404_HTML_FN;
  static const std::string VISE_CSS_FN;
  static const std::string VISE_JS_FN;
  static const std::string VISE_FAVICON_FN;

  private:
  std::vector< std::string > file_contents_cache_;
  std::vector< std::string > filename_cache_;

  boost::filesystem::path resource_dir_;
};

#endif /* _VISE_RESOURCES_H */
