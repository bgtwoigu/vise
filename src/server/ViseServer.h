/** @file   ViseServer.h
 *  @brief  provides HTTP based user interface for VISE
 *
 *  Provides a HTTP based user interface to configure, train and query the
 *  VGG Image Search Enginer (VISE)
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   31 March 2017
 */

#ifndef _VISE_SERVER_H
#define _VISE_SERVER_H

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

#include "SearchEngine.h"
#include "Connection.h"
#include "Resources.h"
#include "ViseMessageQueue.h"

class ViseServer : private boost::noncopyable {
 public:
  ViseServer(const std::string address, const std::string port, std::size_t thread_pool_size, boost::filesystem::path vise_src_code_dir, boost::filesystem::path vise_data_dir);

  ~ViseServer();
  void Start();

 private:
  void AcceptNewConnection();
  void HandleConnection( const boost::system::error_code& e );
  void Stop();

  // server
  std::string address_;
  std::string port_;
  std::size_t thread_pool_size_;
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::shared_ptr<Connection> new_connection_;

  boost::asio::signal_set signals_;
  long server_connection_count_;

  // resource dir
  boost::filesystem::path vise_datadir_;
  boost::filesystem::path vise_src_code_dir_;
  boost::filesystem::path vise_resourcedir_;
  boost::filesystem::path vise_logdir_;
  boost::filesystem::path vise_enginedir_;

  // search engine
  SearchEngine *search_engine_;

  // server resources
  Resources* resources_;
};
#endif /* _VISE_SERVER_H */
