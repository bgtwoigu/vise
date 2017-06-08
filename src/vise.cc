/** @file   vise.cc
 *  @brief  main entry point for the VGG Image Search Enginer (VISE)
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   31 March 2017
 */

#include <iostream>
#include <string>

#include <boost/filesystem.hpp>

#include <Magick++.h>            // to transform images

#include "ViseServer.h"

int main(int argc, char** argv) {
  std::cout << "\nVGG Image Search Engine (VISE)";
  std::cout << "\n";
  if ( argc != 3 ) {
    std::cout << "\n  Usage: ./vise VISE_SOURCE_CODE_DIR VISE_DATA_DIR\n" << std::flush;
    return 0;
  }

  unsigned int port = 9971;

  Magick::InitializeMagick(*argv);

  boost::filesystem::path data_home( argv[2] );
  boost::filesystem::path vise_src_code_dir( argv[1] );

  boost::filesystem::path user_home_dir( getenv("HOME") );

  if (!boost::filesystem::exists(vise_src_code_dir) ) {
    std::cout << "\nVISE_SOURCE_CODE_DIR = " << vise_src_code_dir.string() << " does not exist!" << std::flush;
    std::cout << std::endl;
    return 0;
  }

  if ( !boost::filesystem::exists(data_home) ) {
    boost::filesystem::create_directories( data_home );
    std::cout << "\nCreated VISE_DATA_DIR=" << data_home.string() << std::endl;
  }

  ViseServer vise_server( data_home, vise_src_code_dir, user_home_dir );

  vise_server.Start(port);

  // server is stopped by sending the following HTTP POST request
  // POST /
  // shutdown_vise now (the POST data)
  return 0;
}
