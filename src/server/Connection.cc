#include "Connection.h"

const std::string Connection::crlf = "\r\n";
const std::string Connection::crlf2 = "\r\n\r\n";

const std::string Connection::http_100 = "HTTP/1.0 100 Continue\r\n";
const std::string Connection::http_200 = "HTTP/1.0 200 OK\r\n";
const std::string Connection::http_400 = "HTTP/1.0 400 Bad Request\r\n";
const std::string Connection::http_404 = "HTTP/1.0 404 Not Found\r\n";

Connection::Connection(boost::asio::io_service& io_service, SearchEngine* search_engine, Resources* resources, boost::filesystem::path vise_datadir, boost::filesystem::path vise_src_code_dir)
  : strand_( io_service ), socket_( io_service ), search_engine_( search_engine ), resources_( resources )

{
  vise_datadir_ = vise_datadir;
  vise_src_code_dir_ = vise_src_code_dir;
  request_header_seen_ = false;
  session_name_ = boost::filesystem::unique_path("s%%%%%%%%%%%%");
  response_code_ = -1; // unknown

  //std::cout << "\n[[[START: " << session_name_ << "]]]" << std::flush;
}

Connection::~Connection() {
  //std::cout << "\n{{{END  : " << session_name_ << "}}}" << std::flush;
}

boost::asio::ip::tcp::socket& Connection::Socket() {
  return socket_;
}

void Connection::ProcessConnection() {
  socket_.async_read_some(boost::asio::buffer( buffer_ ),
                          strand_.wrap( boost::bind(&Connection::OnRequestRead, shared_from_this(),
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred
                                                    )
                                        )
                          );
}

void Connection::OnRequestRead(const boost::system::error_code& e, std::size_t bytes_read) {
  // assumption: the first chunk that is read from the HTTP connection, contains the full HTTP header
  if ( request_header_seen_ ) {
    request_content_ << std::string( large_buffer_.data(), large_buffer_.data() + bytes_read );

    if ( request_content_.str().length() != request_content_len_ ) {
      // there are more data to be fetched
      socket_.async_read_some(boost::asio::buffer( large_buffer_ ),
                              strand_.wrap( boost::bind(&Connection::OnRequestRead, shared_from_this(),
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred
                                                        )
                                            )
                              );
    } else {
      ProcessRequestData();
    }
  } else {
    // the buffer should contain HTTP Header
    std::string header(buffer_.data(), buffer_.data() + bytes_read);

    crlf2_index_ = header.find(Connection::crlf2);
    if ( crlf2_index_ == std::string::npos ) {
      ResponseHttp400();
      return;
    }
    request_header_ = header.substr(0, crlf2_index_);
    request_header_seen_ = true;
    crlf2_index_ = crlf2_index_ + Connection::crlf2.length();

    if ( crlf2_index_ != bytes_read ) {
      request_content_ << header.substr(crlf2_index_);
    }

    bool found;
    std::string content_len_str;
    found = ViseUtils::HttpGetHeaderValue(request_header_, "Content-Length: ", content_len_str);
    if ( found ) {
      std::stringstream s ( content_len_str );
      s >> request_content_len_;

      if ( request_content_.str().length() != request_content_len_ ) {
        // there are more data to be fetched
        socket_.async_read_some(boost::asio::buffer( large_buffer_ ),
                                strand_.wrap( boost::bind(&Connection::OnRequestRead, shared_from_this(),
                                                          boost::asio::placeholders::error,
                                                          boost::asio::placeholders::bytes_transferred
                                                          )
                                              )
                                );
      } else {
        ProcessRequestData();
      }
    } else {
      // process the header (most probably a HTTP GET request)
      ProcessRequestData();
    }
  }
}

void Connection::OnResponseWrite(const boost::system::error_code& e) {
  if ( !e ) {
    // close connection on successful write operation
    boost::system::error_code ignored_err;
    socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ignored_err );
    //std::cout << "\nConnection::WriteResponse() : [" << session_name_.string() << "] sent response_buffer_ = " << std::flush;
  }
}

//
// HTTP Standard Responses
//

void Connection::ResponseHttp404() {
  std::ostream http_response( &response_buffer_ );
  http_response << Connection::http_404 << "Content-type: text/plain; charset=utf-8\r\n\r\n";
  WriteResponse();
  response_code_ = 404;
}

void Connection::ResponseHttp400() {
  std::ostream http_response( &response_buffer_ );
  http_response << Connection::http_400 << "Content-type: text/plain; charset=utf-8\r\n\r\n";
  WriteResponse();
  response_code_ = 400;
}

void Connection::ResponseHttp200() {
  std::ostream http_response( &response_buffer_ );
  http_response << Connection::http_200 << "Content-type: text/plain; charset=utf-8\r\n\r\n";
  WriteResponse();
  response_code_ = 200;
}

//
// HTTP Responses
//

void Connection::WriteResponse() {
  boost::asio::async_write(socket_, boost::asio::buffer( response_buffer_.data() ), boost::asio::transfer_all(), 
                           strand_.wrap(boost::bind(&Connection::OnResponseWrite,
                                                    shared_from_this(),
                                                    boost::asio::placeholders::error
                                                    )
                                        )
                           );
  //std::cout << "\nConnection::WriteResponse() : response_buffer_ = " << response_buffer_.size() << std::flush;
  //std::cout << "\nConnection::WriteResponse() : [" << session_name_.string() << "] sending response_buffer_ = " << response_buffer_.size() << std::flush;
}

void Connection::SendHttpResponse(const std::string content_type, const std::string& content) {
  std::ostream http_response( &response_buffer_ );
  http_response << "HTTP/1.1 200 OK\r\n";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  http_response << "Date: " << date_str << "\r\n";
  http_response << "Content-Language: en\r\n";
  http_response << "Connection: close\r\n";
  http_response << "Cache-Control: no-cache\r\n";
  http_response << "Content-type: " << content_type << "; charset=utf-8\r\n";
  http_response << "Content-Length: " << content.length() << "\r\n";
  http_response << "\r\n";
  http_response << content << std::flush;
  //std::cout << "\nConnection::SendHttpResponse() : [" << session_name_.string() << "] sending response_buffer_ = " << response_buffer_.size() << ", content=" << content << std::flush;

  WriteResponse();
  response_code_ = 200;
}

void Connection::SendJsonResponse(const std::string& json) {
  SendHttpResponse("application/json", json);
}

void Connection::SendHtmlResponse(const std::string& html) {
  SendHttpResponse("text/html", html);
}

void Connection::SendImageResponse(const boost::filesystem::path &im_fn) {
  try {
    Magick::Image im;
    im.read( im_fn.string().c_str() );

    std::string content_type = ViseUtils::HttpGetFileContentType(im_fn);
    SendImageResponse( im, content_type );
  } catch( std::exception& e ) {
    std::cerr << "\nViseServer::SendStaticImageResponse() : image cannot be read : " << im_fn.string() << std::endl;
    std::cerr << e.what() << std::flush;
    ResponseHttp404();
  }
}

void Connection::SendImageResponse(Magick::Image &im, std::string content_type) {
  // Connection::image_blob_ keeps the image data safe until the async_write()
  // operation completes. This is necessary.
  try {
    im.magick("JPEG");
    im.write( &image_blob_ );
  } catch( std::exception &e) {
    std::cerr << "\nViseServer::SendImageResponse() : exception " << e.what() << std::flush;
    ResponseHttp404();
    return;
  }

  // calling WriteResponse() is avoided here for performance reasons
  std::ostringstream http_header;
  http_header << http_200
              << "Content-Type: " << content_type << crlf
              << "Content-Length: " << image_blob_.length() << crlf2;

  boost::asio::write(socket_, 
                     boost::asio::buffer( http_header.str(), http_header.str().length() ),
                     boost::asio::transfer_all());

  char* image_buffer = (char *) image_blob_.data();
  boost::asio::async_write(socket_, boost::asio::buffer( image_buffer, image_blob_.length() ),
                           strand_.wrap(boost::bind(&Connection::OnResponseWrite,
                                                    shared_from_this(),
                                                    boost::asio::placeholders::error
                                                    )
                                        )
                           );
  response_code_ = 200;
}


void Connection::SendHttpPostResponse(std::string http_post_data, std::string result) {
  std::ostringstream json;
  json << "{\"id\":\"http_post_response\",\"http_post_data\":\"" << http_post_data 
       << "\",\"result\":\"" << result << "\"}";

  SendJsonResponse( json.str() );
}

//
// Process API Request
//

void Connection::ProcessRequestData() {
  // parse basic headers
  request_http_method_ = request_header_.substr(0, 4);

  std::size_t first_spc = request_header_.find(' ', 0);
  std::size_t second_spc = request_header_.find(' ', first_spc+1);
  if ( second_spc == std::string::npos ) {
    ResponseHttp400();
    return;
  }
  request_http_uri_ = request_header_.substr(first_spc+1, second_spc - first_spc - 1);

  std::cout << "\n  [" << session_name_ << "] " << request_http_method_ << " {" 
            << request_http_uri_ << "} [ request size (header,content) = (" 
            << request_header_.length() << "," << request_content_.str().length() << ") bytes]" << std::flush;

  if ( request_http_method_ == "GET " ) {
    // query the state of search engine
    HandleGetRequest();
  } else if ( request_http_method_ == "POST" ) {
    // change the state of search engine
    HandlePostRequest();
  } else {
    ResponseHttp400();
  }
    
}

// these HTTP GET requests are used to query the state of search engine
void Connection::HandleGetRequest() {
  if ( request_http_uri_ == "/" ) {
    // show main page http://localhost:8080
    SendHtmlResponse( resources_->GetFileContents(Resources::VISE_MAIN_HTML_FN) );
    return;
  }

  if ( request_http_uri_ == "/favicon.ico" ) {
    SendImageResponse( resources_->GetFileContents(Resources::VISE_FAVICON_FN) );
    return;
  }

  if ( request_http_uri_ == "/vise.css" ) {
    SendHttpResponse( "text/css", resources_->GetFileContents(Resources::VISE_CSS_FN) );
    return;
  }

  if ( request_http_uri_ == "/_state" ) {
    // json reply containing current state info.
    std::string state_json = search_engine_->GetStateJsonData();
    SendJsonResponse(state_json);
    return;
  }

  if ( request_http_uri_ == "/_vise_home.html" ) {
    std::string vise_home_template = resources_->GetFileContents(Resources::Resources::VISE_HOME_HTML_FN);
    std::vector< std::string > engine_list;
    search_engine_->GetSearchEngineList( engine_list );

    std::ostringstream engine_list_html;
    if ( engine_list.size() ) {
      for( unsigned int i=0; i<engine_list.size(); i++ ) {
        engine_list_html << "<a onclick=\"_vise_load_search_engine('" << engine_list.at(i) << "')\">"
                         << "<figure></figure><p>" << engine_list.at(i) << "</p></a>";
      }
    } else  {
      engine_list_html << "<p><i>All user created search engines will be shown here</i></p>";
    }
    std::string vise_home_html = vise_home_template;
    ViseUtils::StringReplace(vise_home_html, "__SEARCH_ENGINE_LIST__", engine_list_html.str());
    SendHtmlResponse(vise_home_html);
    return;
  }

  if ( request_http_uri_ == "/vise.js" ) {
    SendHttpResponse( "application/javascript", resources_->GetFileContents(Resources::VISE_JS_FN) );
    return;
  }

  const std::string message_prefix = "/_message";
  if ( ViseUtils::StringStartsWith(request_http_uri_, message_prefix) ) {
    // since HTTP server always requires a request in order to send responses,
    // we always create this _message channel which keeps waiting for messages
    // to be pushed to vise_message_queue_, sends this message to the client
    // which in turn again creates another request for any future messages

    // what if the peer closes connection?
    // socket_ is invalidated
    std::cout << "\n\tBlockingPop():: BLOCKED for session " << session_name_.string() << std::flush;
    std::string msg = ViseMessageQueue::Instance()->BlockingPop(); // blocks until a message is available
    std::cout << "\n\tBlockingPop():: UNBLOCKED for session " << session_name_.string() << std::flush;
    SendHttpResponse( "text/plain", msg);
    return;
  }

  // if http_method_uri contains request for static resource in the form:
  //   GET /_static/ox5k/dir1/dir2/all_souls_000022.jpg?original
  // return the static resource as binary data in HTTP response
  const std::string static_resource_prefix = "/_static/";
  if ( ViseUtils::StringStartsWith(request_http_uri_, static_resource_prefix) ) {
    std::string resource_uri = request_http_uri_.substr( static_resource_prefix.size(), std::string::npos);
    std::string resource_name;
    std::vector< std::string > args_key, args_value;
    std::map< std::string, std::string > resource_args;

    std::size_t qmark = resource_uri.find('?');
    resource_name = resource_uri.substr(0, qmark);
    std::string keyvalue_str = resource_uri.substr(qmark+1, std::string::npos); // remove ? prefix
    if ( qmark != std::string::npos ) {
      ViseUtils::StringParseKeyValue(keyvalue_str, '&', resource_args);
    }

    ServeStaticResource( resource_name, resource_args );
    return;
  }

  if ( request_http_uri_ == "/_random_image" ) {
    // respond with a random training image
    if ( !search_engine_->IsImglistEmpty() ) {
      int index = rand() % search_engine_->GetImglistSize();
      std::string im_fn = search_engine_->GetImglistFn(index);
      std::ostringstream s;
      s << "<div class=\"random_image\"><img src=\"/_static/"
        << search_engine_->GetName() << "/"
        << im_fn << "?variant=original\" /></div>";
      SendHtmlResponse( s.str() );
    } else {
      ResponseHttp404();
    }
    return;
  }

  // state html page
  // example: Get /Cluster
  std::vector< std::string > tokens;
  ViseUtils::StringSplit( request_http_uri_, '/', tokens);
  if ( tokens.at(0) == "" && tokens.size() == 2 ) {
    std::string resource_name = tokens.at(1);
    HandleStateGetRequest( resource_name );
    return;
  }
  ResponseHttp404();
}

void Connection::HandleStateGetRequest( std::string resource_name) {
  std::string state_name = resource_name;
  int state_id = search_engine_->GetStateId( resource_name );
  //std::cout << "\nstate_name = " << state_name << ", " << state_id << std::flush;
  if ( state_id != -1 ) {
    std::string state_html_fn = search_engine_->GetStateHtmlFn(state_id);
    std::string state_html = resources_->GetFileContents(state_html_fn);
    if ( state_id == SearchEngine::STATE_QUERY && search_engine_->GetCurrentStateId() == SearchEngine::STATE_QUERY ) {
      return;
    } else {
      if ( state_id == SearchEngine::STATE_SETTING ) {
        ViseUtils::StringReplace( state_html, "__SEARCH_ENGINE_NAME__", search_engine_->GetName() );
        ViseUtils::StringReplace( state_html,
                       "__DEFAULT_IMAGE_PATH__",
                       (std::string(getenv("HOME")) + "/vgg/mydata/images/") ); // @todo: replace with something more sensible (like dired)
        SendCommand("_state show");
        //SendCommand("_dired fetch " + user_home_dir_.string() );
      } else if ( state_id == SearchEngine::STATE_INFO ) {
        state_html = search_engine_->GetStateComplexityInfo();
        SendCommand("_control_panel add <div class=\"action_button\" onclick=\"_vise_server_send_state_post_request('Info', 'proceed')\">Proceed</div>");
      }
      //std::cout << "\nstate_html_fn = " << state_html_fn << std::flush;
      SendHtmlResponse( state_html );
      return;
    }
  }
  ResponseHttp404();
}

void Connection::ServeStaticResource(const std::string resource_name, const std::map< std::string, std::string> &resource_args) {
  // resource uri format:
  // SEARCH_ENGINE_NAME/...path...
  //std::cout << "\nViseServer::ServeStaticResource() : " << resource_name << ", " << resource_args.size() << std::flush;
  std::size_t first_slash_pos = resource_name.find('/', 0); // ignore the first /

  //std::map<std::string, std::string>::const_iterator it;
  //for ( it=resource_args.begin(); it!=resource_args.end(); ++it) {
  //  std::cout << "\n" << it->first << " => " << it->second << std::flush;
  //}


  if ( first_slash_pos != std::string::npos ) {
    std::string search_engine_name = resource_name.substr(0, first_slash_pos);
    if ( search_engine_->GetName() == search_engine_name ) {
      std::string res_rel_path = resource_name.substr(first_slash_pos+1, std::string::npos);
      boost::filesystem::path res_fn = search_engine_->GetTransformedImageDir() / res_rel_path;
      if ( resource_args.empty() ) {
        if ( boost::filesystem::exists(res_fn) ) {
          std::cout << "\nSending image : " << res_fn.string() << std::flush;
          SendImageResponse(res_fn);
        } else {
          ResponseHttp404();
        }
        return;
      }

      if ( resource_args.count("variant") == 1 ) {
        if ( resource_args.find("variant")->second == "original" ) {
          res_fn = search_engine_->GetOriginalImageDir() / res_rel_path;
        }
      }

      if ( ! boost::filesystem::exists(res_fn) ) {
        ResponseHttp404();
        return;
      }

      try {
        Magick::Image im;
        im.read( res_fn.string() );

        // @todo: transform image according to resource_args

        std::string content_type = ViseUtils::HttpGetFileContentType(res_fn);
        SendImageResponse( im, content_type );
        return;
      } catch ( std::exception &error ) {
        ResponseHttp404();
      }
      return;
    }
  }
  ResponseHttp404();
}

// these HTTP POST requests are used to change the state of search engine
void Connection::HandlePostRequest() {
  std::string http_post_data = request_content_.str();

  if ( request_http_uri_ == "/" ) {
    // the http_post_data can be one of these:
    //  * create_search_engine  _NAME_OF_SEARCH_ENGINE_
    //  * load_search_engine    _NAME_OF_SEARCH_ENGINE_
    //  * stop_training_process _NAME_OF_SEARCH_ENGINE_
    std::vector< std::string > tokens;
    ViseUtils::StringSplit( http_post_data, ' ', tokens);

    if ( tokens.size() == 2 ) {
      std::string search_engine_name = tokens.at(1);
      if ( tokens.at(0) == "create_search_engine" ) {
        if ( search_engine_->Exists( search_engine_name ) ) {
          SendMessage("Search engine by that name already exists!");
          SendHttpPostResponse( http_post_data, "ERR" );
        } else {
          if ( SearchEngine::ValidateName( search_engine_name ) ) {
            search_engine_->Init( search_engine_name );
            if ( search_engine_->UpdateState() ) {
              // send control message : state updated
              SendCommand("_state update_now");
              SendHttpPostResponse( http_post_data, "OK" );
            } else {
              SendMessage("Cannot initialize search engine [" + search_engine_name + "]");
              SendHttpPostResponse( http_post_data, "ERR" );
            }
          } else {
            SendMessage("Search engine name cannot contains spaces, or special characters ( .,*,?,/,\\ )");
            SendHttpPostResponse( http_post_data, "ERR" );
          }
        }
        return;
        // send control message to set loaded engine name
      } else if ( tokens.at(0) == "load_search_engine" ) {
          if ( search_engine_->GetName() == search_engine_name ) {
            SendHttpPostResponse( http_post_data, "Search engine already loaded!" );
            SendCommand("_state update_now");
          } else {
            if ( search_engine_->Exists( search_engine_name ) ) {
              SendMessage("Loading search engine [" + search_engine_name + "] ...");
              search_engine_->Load( search_engine_name );
              //SendCommand("_state update_now");
              SendHttpPostResponse( http_post_data, "OK" );
            } else {
              SendHttpPostResponse( http_post_data, "ERR" );
              SendMessage("Search engine does not exists!");
            }
          }
          return;
      } else if ( tokens.at(0) == "delete_search_engine" ) {
        if ( search_engine_->Exists( search_engine_name ) ) {
          if ( search_engine_->Delete( search_engine_name ) ) {
            SendMessage("Deleted search engine [" + search_engine_name + "]");
            SendCommand("_go_home");
          } else {
            SendMessage("Failed to delete search engine [" + search_engine_name + "]");
          }
          return;
        } else {
          SendHttpPostResponse( http_post_data, "ERR" );
          SendMessage("Search engine [" + search_engine_name + "] does not exists!");
        }
      } else {

      }
    } else {
      // unexpected POST data
      SendHttpPostResponse(http_post_data, "ERR");
      return;
    }
  } else {
    // state POST data is stored by request_content_
    // For example: POST /Settings [post data: engine_description=...
    std::string resource_name = request_http_uri_.substr(1, std::string::npos); // remove prefix "/"
    int state_id = search_engine_->GetStateId( resource_name );
    if ( state_id != -1 ) {
      HandleStatePostData(state_id);
      return;
    }
  }

  // default handler: unknown POST data
  ResponseHttp404();
  return;
}

void Connection::HandleStatePostData( int state_id ) {
  if ( state_id == SearchEngine::STATE_SETTING ) {
    search_engine_->SetEngineConfig(request_content_.str());
    search_engine_->WriteConfigToFile();
    if ( search_engine_->GetImglistSize() == 0 ) {
      search_engine_->CreateFileList();
    }
    search_engine_->UpdateStateInfoList();

    if ( search_engine_->UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
      SendCommand("_content clear");
      SendHttpPostResponse( "http post data of vise configuration", "OK" );
    } else {
      SendHttpPostResponse( "http post data of vise configuration", "ERR" );
    }
  } else if ( state_id == SearchEngine::STATE_INFO ) {
    if ( request_content_.str() == "proceed" ) {
      if ( search_engine_->UpdateState() ) {
        // send control message : state updated
        SendCommand("_state update_now");
        SendHttpPostResponse( request_content_.str(), "OK" );

        // initiate the search engine training process
        search_engine_->StartTraining();
      } else {
        SendHttpPostResponse( request_content_.str(), "ERR" );
      }
    }
  } else {
    std::cerr << "\nViseServer::HandleStatePostData() : Do not know how to handle this HTTP POST data!" << std::flush;
  }
}

//
// ViseMessageQueue
//

void Connection::SendMessage(std::string message) {
  SendPacket( "message", message);
}

void Connection::SendCommand(std::string command) {
  SendPacket("command", command );
}

void Connection::SendPacket(std::string type, std::string message) {
  std::ostringstream s;
  s << "Connection " << type << " " << message;
  ViseMessageQueue::Instance()->Push( s.str() );
}

