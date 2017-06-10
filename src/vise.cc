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
  Magick::InitializeMagick(*argv);

  std::cout << "\nVGG Image Search Engine (VISE)";
  std::cout << "\nAuthor: Abhishek Dutta <adutta@robots.ox.ac.uk>, May 2017\n";
  std::cout << "\nVISE builds on the \"relja_retrival\" (Sep. 2014) C++ codebase \nauthored by Relja Arandjelovic <relja@robots.ox.ac.uk> during \nhis DPhil / Postdoc at the Visual Geometry Group in the \nDepartment of Engineering Science, University of Oxford." << std::endl;

  if ( argc != 5 ) {
    std::cerr << "\nUsage: vise <port> <threads> <vise_source_code_dir/> <vise_data_dir/>" << std::endl;
    return 0;
  }

  try {
    // To avoid potential security risks, this server should only be run with address : localhost (127.0.0.1)
    std::string address = "127.0.0.1";

    std::string port( argv[1] );

    std::size_t num_threads;
    std::istringstream s( argv[2] );
    s >> num_threads;

    boost::filesystem::path vise_src_code_dir( argv[3] );
    boost::filesystem::path vise_data_dir( argv[4] );

    if ( !boost::filesystem::exists(vise_src_code_dir) ) {
      std::cout << "\nvise_source_code_dir = " << vise_src_code_dir.string() << " does not exist!" << std::endl;
      return 0;
    }

    if ( !boost::filesystem::exists(vise_data_dir) ) {
      boost::filesystem::create_directories( vise_data_dir );
      std::cout << "\nCreated vise_data_dir=" << vise_data_dir.string() << std::endl;
    }

    ViseServer vise_server( address, port, num_threads, vise_src_code_dir, vise_data_dir );
    vise_server.Start();
  } catch( std::exception& e ) {
    std::cerr << "\nFailed to start server due to exception!" << std::endl;
    std::cerr << e.what() << std::flush;
  }

  return 0;
}
