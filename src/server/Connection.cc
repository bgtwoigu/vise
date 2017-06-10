#include "Connection.h"

const std::string Connection::crlf = "\r\n";
const std::string Connection::crlf2 = "\r\n\r\n";

const std::string Connection::http_100 = "HTTP/1.0 100 Continue\r\n";
const std::string Connection::http_200 = "HTTP/1.0 200 OK\r\n";
const std::string Connection::http_400 = "HTTP/1.0 400 Bad Request\r\n";

Connection::Connection(boost::asio::io_service& io_service, SearchEngine* search_engine, boost::filesystem::path vise_datadir, boost::filesystem::path vise_src_code_dir)
  : strand_( io_service ), socket_( io_service ), search_engine_( search_engine )

{
  vise_datadir_ = vise_datadir;
  vise_src_code_dir_ = vise_src_code_dir;
  request_header_seen_ = false;
  session_name_ = boost::filesystem::unique_path("s%%%%%%%%%%%%");
  connection_error_ = -1; // unknown
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
      ResponseForBadRequest("Malformed request");
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

void Connection::ResponseForBadRequest(std::string message) {
  std::ostream http_response( &response_buffer_ );
  http_response << Connection::http_400
        << "Content-Type: text/plain\r\n\r\n"
        << message;

  boost::asio::async_write(socket_, boost::asio::buffer( response_buffer_.data() ),
                           strand_.wrap(boost::bind(&Connection::OnResponseWrite,
                                                    shared_from_this(),
                                                    boost::asio::placeholders::error
                                                    )
                                        )
                           );
  connection_error_ = 1;
}


void Connection::ResponseHttp200() {
  std::ostream http_response( &response_buffer_ );
  http_response << Connection::http_200 << "\r\n";

  boost::asio::async_write(socket_, boost::asio::buffer( response_buffer_.data() ),
                           strand_.wrap(boost::bind(&Connection::OnResponseWrite,
                                                    shared_from_this(),
                                                    boost::asio::placeholders::error
                                                    )
                                        )
                           );
  connection_error_ = 0;
}

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

//
// Process API Request
//

void Connection::ProcessRequestData() {
  // parse basic headers
  request_http_method_ = request_header_.substr(0, 4);

  std::size_t first_spc = request_header_.find(' ', 0);
  std::size_t second_spc = request_header_.find(' ', first_spc+1);
  if ( second_spc == std::string::npos ) {
    ResponseForBadRequest("Malformed request");
    return;
  }
  request_http_uri_ = request_header_.substr(first_spc+1, second_spc - first_spc - 1);

  std::cout << "\n[" << session_name_ << "] " << request_http_method_ << " {" 
            << request_http_uri_ << "} [ request size (header,content) = (" 
            << request_header_.length() << "," << request_content_.str().length() << ") bytes]" << std::flush;

  ResponseHttp200();
}
