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

#include <boost/filesystem.hpp>

class Resources {
  public:
  void LoadAllResources(const boost::filesystem::path resource_dir);

  // html content
  std::string vise_main_html_;
  std::string vise_help_html_;
  std::string vise_404_html_;
  std::string vise_css_;
  std::string vise_js_;
  boost::filesystem::path vise_favicon_fn_;

  private:
  boost::filesystem::path resource_dir_;

  // html content location
  boost::filesystem::path vise_css_fn_;
  boost::filesystem::path vise_js_fn_;
  boost::filesystem::path vise_main_html_fn_;
  boost::filesystem::path vise_help_html_fn_;
  boost::filesystem::path vise_404_html_fn_;

  // state complexity
  std::vector< std::vector<double> > state_complexity_model_;
  std::vector<double> total_complexity_model_;
  std::string state_complexity_info_;
  std::string complexity_model_assumption_;

  int LoadFile(const boost::filesystem::path filename, std::string &file_contents);
  void LoadStateComplexityModel();
};

#endif /* _VISE_RESOURCES_H */
