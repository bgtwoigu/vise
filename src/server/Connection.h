/** @file   Connection.h
 *  @brief  Denotes a connection corresponding to a user's request
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   22 May 2017
 */

#ifndef _IMCOMP_CONNECTION_H
#define _IMCOMP_CONNECTION_H

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/array.hpp>

#include "SearchEngine.h"

class Connection : public boost::enable_shared_from_this<Connection>, private boost::noncopyable
{
 public:
  Connection(boost::asio::io_service& io_service, SearchEngine* search_engine, boost::filesystem::path vise_datadir, boost::filesystem::path vise_src_code_dir);
  ~Connection();

  boost::asio::ip::tcp::socket& Socket();
  void ProcessConnection();

  static const std::string crlf;
  static const std::string crlf2;
  static const std::string http_100;
  static const std::string http_200;
  static const std::string http_400;
 private:
  // boost asio async handler functions
  void OnRequestRead(const boost::system::error_code& e, std::size_t bytes_read);
  void OnResponseWrite(const boost::system::error_code& e);

  // to ensure that only a single thread invokes a handler
  boost::asio::io_service::strand strand_;

  // tcp socket
  boost::asio::ip::tcp::socket socket_;

  // HTTP Request
  void ProcessRequestData();
  void CreateResponse();
  bool GetHttpHeaderValue(const std::string& header, const std::string& name, std::string& value);

  boost::array<char, 1024> buffer_;
  boost::array<char, 8192> large_buffer_;
  bool request_header_seen_;
  std::string request_header_;
  std::size_t request_content_len_;
  std::string request_content_type_;
  std::stringstream request_content_;
  std::string request_http_method_;
  std::string request_http_uri_;
  std::string request_host_;
  std::size_t crlf2_index_;

  // connection state variables
  boost::filesystem::path session_name_;
  boost::filesystem::path vise_datadir_;
  boost::filesystem::path vise_src_code_dir_;
  int connection_error_;

  // HTTP Response
  void ResponseForBadRequest(std::string message);
  void ResponseHttp200();

  boost::asio::streambuf response_buffer_;

  // Search Engine
  SearchEngine* search_engine_;
};

#endif
