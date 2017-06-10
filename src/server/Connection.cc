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
}

Connection::~Connection() {
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
    found = GetHttpHeaderValue(request_header_, "Content-Length: ", content_len_str);
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
  }
}

//
// Helper functions
//

bool Connection::GetHttpHeaderValue(const std::string& header, const std::string& name, std::string& value) {
  std::size_t start_index = header.find( name );
  //std::cout << "\n\tstart=" << start_index << std::flush;
  if ( start_index == std::string::npos ) {
    return false;
  } else {
    std::size_t end_index = header.find( crlf, start_index + name.length() );
    //std::cout << ", end=" << end_index << std::flush;
    if ( end_index == std::string::npos ) {
      return false;
    } else {
      value = header.substr( start_index + name.length(), end_index - start_index - name.length() );
      //std::cout << "[" << name << "=" << value << "]" << std::flush;
      return true;
    }
  }
}

std::string Connection::GetFileContentType( const boost::filesystem::path& fn) {
  std::string ext = fn.extension().string();
  std::string http_content_type = "unknown";
  if ( ext == ".jpg" || ext == ".JPG" ) {
    http_content_type = "image/jpeg";
  } else if ( ext == ".png" ) {
    http_content_type = "image/png";
  } else if ( ext == ".txt" ) {
    http_content_type = "text/plain";
  } else if ( ext == ".html" ) {
    http_content_type = "text/html";
  } else if ( ext == ".json" ) {
    http_content_type = "application/json";
  } else if ( ext == ".js" ) {
    http_content_type = "application/javascript";
  }

  return http_content_type;
}

bool Connection::StringStartsWith( const std::string &s, const std::string &prefix ) {
  if ( s.substr(0, prefix.length()) == prefix ) {
    return true;
  } else {
    return false;
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
  boost::asio::async_write(socket_, boost::asio::buffer( response_buffer_.data() ),
                           strand_.wrap(boost::bind(&Connection::OnResponseWrite,
                                                    shared_from_this(),
                                                    boost::asio::placeholders::error
                                                    )
                                        )
                           );
}

void Connection::SendHttpResponse(const std::string content_type, const std::string &content) {
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
  http_response << content;

  WriteResponse();
  response_code_ = 200;
}

void Connection::SendJsonResponse(const std::string &json) {
  SendHttpResponse("application/json", json);
}

void Connection::SendHtmlResponse(const std::string &html) {
  SendHttpResponse("text/html", html);
}

void Connection::SendImageResponse(const boost::filesystem::path &im_fn) {
  try {
    Magick::Image im;
    im.read( im_fn.string().c_str() );

    std::string content_type = GetFileContentType(im_fn);
    SendImageResponse( im, content_type );
  } catch( std::exception& e ) {
    std::cerr << "\nViseServer::SendStaticImageResponse() : image cannot be read : " << im_fn.string() << std::endl;
    std::cerr << e.what() << std::flush;
    ResponseHttp404();
  }
}

void Connection::SendImageResponse(Magick::Image &im, std::string content_type) {
  Magick::Blob im_blob;
  try {
    im.magick("JPEG");
    im.write( &im_blob );
  } catch( std::exception &e) {
    std::cerr << "\nViseServer::SendImageResponse() : exception " << e.what() << std::flush;
    ResponseHttp404();
    return;
  }

  Connection::SendHttpResponse(content_type, (char *) im_blob.data() );
  WriteResponse();
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

  std::cout << "\n[" << session_name_ << "] " << request_http_method_ << " {" 
            << request_http_uri_ << "} [ request size (header,content) = (" 
            << request_header_.length() << "," << request_content_.str().length() << ") bytes]" << std::flush;

  if ( request_http_method_ == "GET " ) {
    HandleGetRequest();
  } else if ( request_http_method_ == "POST" ) {
    HandlePostRequest();
  } else {
    ResponseHttp400();
  }
    
}

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

  if ( request_http_uri_ == "/_vise_home.html" ) {
    std::string vise_home_template = resources_->GetFileContents(Resources::Resources::VISE_HOME_HTML_FN);
    std::vector< std::string > engine_list;
    search_engine_->GetSearchEngineList( engine_list );

    std::ostringstream engine_list_html;
    for( unsigned int i=0; i<engine_list.size(); i++ ) {
      engine_list_html << "<a onclick=\"_vise_load_search_engine(\"" << engine_list.at(i) << "\")\">"
                       << "<figure></figure><p>" << engine_list.at(i) << "</p></a>";
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
  if ( StringStartsWith(request_http_uri_, message_prefix) ) {
    // since HTTP server always requires a request in order to send responses,
    // we always create this _message channel which keeps waiting for messages
    // to be pushed to vise_message_queue_, sends this message to the client
    // which in turn again creates another request for any future messages
    std::string msg = ViseMessageQueue::Instance()->BlockingPop(); // blocks until a message is available
    SendHttpResponse( "text/plain", msg);
    return;
  }

  ResponseHttp404();
  return;
}

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
        } else {
          if ( SearchEngine::ValidateName( search_engine_name ) ) {
            search_engine_->Init( search_engine_name );

            if ( search_engine_->UpdateState() ) {
              // send control message : state updated
              SendCommand("_state update_now");
              SendHttpPostResponse( http_post_data, "OK" );
              SendCommand("_state update_now");
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
              search_engine_->Init( search_engine_name );
              SendHttpPostResponse( http_post_data, "Loaded search engine" );
            } else {
              SendHttpPostResponse( http_post_data, "Search engine does not exist" );
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
    // state POST data
  }

  // default handler: unknown POST data
  ResponseHttp404();
  return;
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

