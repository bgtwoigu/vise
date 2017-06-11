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
#include <map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/array.hpp>

#include <Magick++.h>            // to transform images

#include "SearchEngine.h"
#include "Resources.h"
#include "ViseMessageQueue.h"
#include "ViseUtils.h"

class Connection : public boost::enable_shared_from_this<Connection>, private boost::noncopyable
{
 public:
  Connection(boost::asio::io_service& io_service, 
            SearchEngine* search_engine, 
            Resources* resources_, 
            boost::filesystem::path vise_datadir, 
            boost::filesystem::path vise_src_code_dir);
  ~Connection();

  boost::asio::ip::tcp::socket& Socket();
  void ProcessConnection();

  static const std::string crlf;
  static const std::string crlf2;
  static const std::string http_100;
  static const std::string http_200;
  static const std::string http_400;
  static const std::string http_404;
 private:
  // boost asio async handler functions
  void OnRequestRead(const boost::system::error_code& e, std::size_t bytes_read);
  void OnResponseWrite(const boost::system::error_code& e);

  // to ensure that only a single thread invokes a handler
  boost::asio::io_service::strand strand_;

  // tcp socket
  boost::asio::ip::tcp::socket socket_;

  // Request handlers
  void ProcessRequestData();
  void HandleGetRequest();
  void HandlePostRequest();
  void HandleStateGetRequest( std::string resource_name);
  void HandleStatePostData(int state_id);
  void ServeStaticResource(const std::string resource_name, const std::map< std::string, std::string> &resource_args);

  // HTTP Request
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

  // http response
  boost::asio::streambuf response_buffer_;
  unsigned int response_code_;

  // Search Engine
  SearchEngine* search_engine_;

  // Resources
  Resources* resources_;

  // HTTP Standard Responses
  void ResponseHttp404();
  void ResponseHttp400();
  void ResponseHttp200();

  // HTTP Responses
  void WriteResponse();
  void SendHttpResponse(const std::string content_type, const std::string &content);
  void SendJsonResponse(const std::string &json);
  void SendHtmlResponse(const std::string &html);
  void SendImageResponse(const boost::filesystem::path &im_fn);
  void SendImageResponse(Magick::Image &im, std::string content_type);
  void SendHttpPostResponse(std::string http_post_data, std::string result);

  // ViseMessageQueue
  void SendMessage(std::string message);
  void SendCommand(std::string command);
  void SendPacket(std::string type, std::string message);

  // for HTTP response containing image
  Magick::Blob image_blob_;
};

#endif
