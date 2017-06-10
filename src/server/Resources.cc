#include "Resources.h"

const std::string Resources::VISE_MAIN_HTML_FN = "vise_main.html";
const std::string Resources::VISE_HOME_HTML_FN = "vise_home.html";
const std::string Resources::VISE_HELP_HTML_FN = "vise_help.html";
const std::string Resources::VISE_404_HTML_FN  = "vise_404.html";
const std::string Resources::VISE_CSS_FN       = "vise.css";
const std::string Resources::VISE_JS_FN        = "vise.js";
const std::string Resources::VISE_FAVICON_FN   = "favicon.ico";

void Resources::LoadAllResources(const boost::filesystem::path resource_dir) {
  resource_dir_ = resource_dir;
  file_contents_cache_.push_back("");
  filename_cache_.push_back("");
}

std::string& Resources::GetFileContents(const std::string filename) {
  // check if this file exists in cache
  for ( unsigned int i = 0; i < filename_cache_.size(); i++ ) {
    if ( filename_cache_.at(i) == filename ) {
      // send file contents from cache
      return file_contents_cache_.at(i);
    }
  }

  // control reaching here implies that the file is not in cache
  // so we load it into the cache
  boost::filesystem::path full_filename = resource_dir_ / filename;
  if ( boost::filesystem::exists( full_filename ) ) {
    std::string file_contents;
    ViseUtils::FileLoad( full_filename.string(), file_contents );

    filename_cache_.push_back( filename );
    file_contents_cache_.push_back(file_contents);
    return file_contents_cache_.at( file_contents_cache_.size() - 1 ); 
  } else {
    std::cerr << "\nResources::GetFileContents() : resource filename does not exist: " << full_filename << std::flush;
    return file_contents_cache_.at(0);
  }
}

