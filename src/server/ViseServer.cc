#include "ViseServer.h"

const int ViseServer::STATE_NOT_LOADED;
const int ViseServer::STATE_SETTING;
const int ViseServer::STATE_INFO;
const int ViseServer::STATE_PREPROCESS;
const int ViseServer::STATE_DESCRIPTOR;
const int ViseServer::STATE_CLUSTER;
const int ViseServer::STATE_ASSIGN;
const int ViseServer::STATE_HAMM;
const int ViseServer::STATE_INDEX;
const int ViseServer::STATE_QUERY;

ViseServer::ViseServer( boost::filesystem::path vise_datadir, boost::filesystem::path vise_src_code_dir, boost::filesystem::path user_home_dir ) {
  // set resource names
  vise_datadir_         = boost::filesystem::path(vise_datadir);
  vise_source_code_dir_ = boost::filesystem::path(vise_src_code_dir);
  user_home_dir_        = user_home_dir;
  vise_templatedir_     = vise_source_code_dir_ / "src/server/html_templates/";

  if ( ! boost::filesystem::exists( vise_datadir_ ) ) {
    std::cerr << "\nViseServer::ViseServer() : vise_datadir_ does not exist! : "
              << vise_datadir_.string() << std::flush;
    return;
  }
  if ( ! boost::filesystem::exists( vise_templatedir_ ) ) {
    std::cerr << "\nViseServer::ViseServer() : vise_templatedir_ does not exist! : "
              << vise_templatedir_.string() << std::flush;
    return;
  }

  vise_enginedir_    = vise_datadir_ / "search_engines";
  if ( ! boost::filesystem::exists( vise_enginedir_ ) ) {
    boost::filesystem::create_directory( vise_enginedir_ );
  }

  vise_logdir_       = vise_datadir_ / "log";
  if ( ! boost::filesystem::exists( vise_logdir_ ) ) {
    boost::filesystem::create_directory( vise_logdir_ );
  }

  vise_main_html_fn_ = vise_templatedir_ / "vise_main.html";
  vise_help_html_fn_ = vise_templatedir_ / "vise_help.html";
  vise_css_fn_       = vise_templatedir_ / "vise.css";
  vise_js_fn_        = vise_templatedir_ / "vise.js";
  vise_404_html_fn_  = vise_templatedir_ / "404.html";

  // load html resources
  LoadFile(vise_main_html_fn_.string(), vise_main_html_);
  LoadFile(vise_help_html_fn_.string(), vise_help_html_);
  LoadFile(vise_404_html_fn_.string() , vise_404_html_);
  LoadFile(vise_css_fn_.string(), vise_css_);
  LoadFile(vise_js_fn_.string() , vise_js_);

  state_id_list_.push_back( ViseServer::STATE_NOT_LOADED );
  state_name_list_.push_back( "NotLoaded" );
  state_html_fn_list_.push_back( "404.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_SETTING );
  state_name_list_.push_back( "Setting" );
  state_html_fn_list_.push_back( "Setting.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_INFO );
  state_name_list_.push_back( "Info" );
  state_html_fn_list_.push_back( "Info.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_PREPROCESS );
  state_name_list_.push_back( "Preprocess" );
  state_html_fn_list_.push_back( "Preprocess.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_DESCRIPTOR );
  state_name_list_.push_back( "Stage-1" );
  state_html_fn_list_.push_back( "Descriptor.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_CLUSTER );
  state_name_list_.push_back( "Stage-2" );
  state_html_fn_list_.push_back( "Cluster.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_ASSIGN );
  state_name_list_.push_back( "Stage-3" );
  state_html_fn_list_.push_back( "Assign.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_HAMM );
  state_name_list_.push_back( "Stage-4" );
  state_html_fn_list_.push_back( "Hamm.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_INDEX );
  state_name_list_.push_back( "Stage-5" );
  state_html_fn_list_.push_back( "Index.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( ViseServer::STATE_QUERY );
  state_name_list_.push_back( "Ready to Search" );
  state_html_fn_list_.push_back( "Query.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  total_complexity_model_ = std::vector<double>(4, 0.0);

  state_id_ = ViseServer::STATE_NOT_LOADED;

  // load all state html
  state_html_list_.resize( state_id_list_.size() );
  for (unsigned int i=0; i < state_id_list_.size(); i++) {
    state_html_list_.at(i) = "";
    boost::filesystem::path fn = vise_templatedir_ / state_html_fn_list_.at(i);
    LoadFile( fn.string(), state_html_list_.at(i) );
  }
  state_html_template_list_ = state_html_list_;

  vise_index_html_reload_ = true;

  // open a log file to save training statistics
  // for logging statistics
  vise_training_stat_fn_ = vise_logdir_ / "training_stat.csv";

  bool append_csv_header = false;
  if ( ! boost::filesystem::exists( vise_training_stat_fn_ ) ) {
    append_csv_header = true;
  }

  training_stat_f.open( vise_training_stat_fn_.string().c_str(), std::ofstream::app );
  if ( append_csv_header ) {
    training_stat_f << "date,time,dataset_name,img_count,state_name,time_sec,space_bytes";
  }
  std::cout << "\nvise_training_stat_fn_ = " << vise_training_stat_fn_ << std::flush;

  LoadStateComplexityModel();

  vise_shutdown_flag_ = false;
}

ViseServer::~ViseServer() {
  // for logging statistics
  training_stat_f.close();

  // cleanup
  if ( cons_queue_ ) {
    delete cons_queue_;
  }

  if ( hamming_emb_ != NULL ) {
    delete hamming_emb_;
    delete multi_query_max_;
  }
  if ( multi_query_ != NULL ) {
    delete multi_query_;
  }
  delete emb_factory_;

  if ( !useHamm ) {
    delete soft_assigner_;
  }

  delete nn_;
  delete clst_centres_;
  delete feat_getter_;

  delete fidx_;
  delete iidx_;

  delete tfidf_;

  delete dataset_;

  delete spatial_verif_v2_;

  delete dbFidx_;
  delete dbIidx_;
}

std::string ViseServer::GetStateComplexityInfo() {
  unsigned long n = search_engine_.GetImglistSize();

  std::ostringstream s;
  std::vector< double > m = total_complexity_model_;
  double time  = m[0] + m[1] * n; // in minutes
  double space = m[2] + m[3] * n; // in MB

  s << "<h3>Overview of Search Engine Training Requirements</h3>"
    << "<table id=\"engine_overview\">"
    << "<tr><td>Number of images</td><td>" << n << "</td></tr>"
    << "<tr><td>Estimated total training time*</td><td>" << ceil(time) << " min.</td></tr>"
    << "<tr><td>Estimated memory needed*</td><td>" << "4 GB</td></tr>"
    << "<tr><td>Estimated total disk space needed*</td><td>" << ceil(space) << " MB</td></tr>"
    << "<tr><td>&nbsp;</td><td>&nbsp;</td></tr>"
    << "<tr><td colspan=\"2\">* estimates are based on the following specifications : </td></tr>"
    << "<tr><td colspan=\"2\">  " << complexity_model_assumption_ << "</td></tr>"
    <<"</td></tr>"
    << "</table>";
  return s.str();
}

void ViseServer::UpdateStateInfoList() {
  unsigned long n = search_engine_.GetImglistSize();
  std::vector< int > state_id_list;
  state_id_list.push_back( ViseServer::STATE_PREPROCESS );
  state_id_list.push_back( ViseServer::STATE_DESCRIPTOR );
  state_id_list.push_back( ViseServer::STATE_CLUSTER );
  state_id_list.push_back( ViseServer::STATE_INDEX );

  for (unsigned int i=0; i<state_id_list.size(); i++) {
    int state_id = state_id_list.at(i);
    std::vector< double > m = state_complexity_model_.at(state_id);
    double time  = m[0] + m[1] * n; // in minutes
    double space = m[2] + m[3] * n; // in MB

    std::ostringstream sinfo;
    sinfo << "(" << ceil(time) << " min, " << ceil(space) << " MB)";
    state_info_list_.at( state_id ) = sinfo.str();
    //std::cout << "\nm=" << m[0] << "," << m[1] << "," << m[2] << "," << m[3] << std::flush;
    //std::cout << "\nstate = " << state_id << " : " << sinfo.str() << std::flush;
  }
}

void ViseServer::LoadStateComplexityModel() {
  /*
    NOTE: state_model_complexity_ is obtained as follows:
    regression coefficient source: docs/training_time/plot_training_time_model.R

    > source('plot.R')
    [1] "Model coefficients for time"
    state_name   (Intercept)    img_count
    1     Assign -0.0635593220 0.0008276836
    2    Cluster -1.5004237288 0.0364477401
    3 Descriptor  0.2545197740 0.0031129944
    4       Hamm -0.0004237288 0.0001144068
    5      Index -0.4600282486 0.0175409605
    6 Preprocess -0.0608757062 0.0011031073
    7      TOTAL -1.8307909605 0.0591468927
    [1] "Model coefficients for space"
    state_name   (Intercept)   img_count
    1     Assign  4.440892e-16 0.003814697
    2    Cluster  3.147125e-05 0.048828125
    3 Descriptor  4.768372e-06 0.122070312
    4       Hamm  3.126557e-02 0.024414063
    5      Index -1.438618e+00 0.072752569
    6 Preprocess  3.374722e+00 0.427843547
    7      TOTAL  1.967406e+00 0.699723314
  */

  complexity_model_assumption_ = "cpu name: Intel(R) Core(TM) i7-6700HQ CPU @ 2.60GHz; cpu MHz : 3099.992; RAM: 16GB; cores : 8";

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( ViseServer::STATE_PREPROCESS ).at(0) = -0.0608757062; // coef_0 (time)
  state_complexity_model_.at( ViseServer::STATE_PREPROCESS ).at(1) =  0.0011031073; // coef_1 (time)
  state_complexity_model_.at( ViseServer::STATE_PREPROCESS ).at(2) =  3.374722;     // coef_0 (space)
  state_complexity_model_.at( ViseServer::STATE_PREPROCESS ).at(3) =  0.427843547;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( ViseServer::STATE_DESCRIPTOR ).at(0) = 0.2545197740; // coef_0 (time)
  state_complexity_model_.at( ViseServer::STATE_DESCRIPTOR ).at(1) = 0.0031129944; // coef_1 (time)
  state_complexity_model_.at( ViseServer::STATE_DESCRIPTOR ).at(2) = 4.768372e-06; // coef_0 (space)
  state_complexity_model_.at( ViseServer::STATE_DESCRIPTOR ).at(3) = 0.122070312;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( ViseServer::STATE_CLUSTER ).at(0) = -1.5004237288; // coef_0 (time)
  state_complexity_model_.at( ViseServer::STATE_CLUSTER ).at(1) =  0.0364477401; // coef_1 (time)
  state_complexity_model_.at( ViseServer::STATE_CLUSTER ).at(2) =  3.147125e-05; // coef_0 (space)
  state_complexity_model_.at( ViseServer::STATE_CLUSTER ).at(3) =  0.048828125;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( ViseServer::STATE_INDEX ).at(0) = -0.4600282486; // coef_0 (time)
  state_complexity_model_.at( ViseServer::STATE_INDEX ).at(1) =  0.0175409605; // coef_1 (time)
  state_complexity_model_.at( ViseServer::STATE_INDEX ).at(2) = -1.438618;     // coef_0 (space)
  state_complexity_model_.at( ViseServer::STATE_INDEX ).at(3) =  0.072752569;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  total_complexity_model_.at(0) = -1.8307909605; // coef_0 (time)
  total_complexity_model_.at(1) =  0.0591468927; // coef_1 (time)
  total_complexity_model_.at(2) =  1.967406;     // coef_0 (space)
  total_complexity_model_.at(3) =  0.699723314;  // coef_1 (space)
}

void ViseServer::Start(unsigned int port) {
  hostname_ = "localhost";
  port_ = port;

  std::ostringstream url_builder;
  url_builder << "http://" << hostname_ << ":" << port_;
  url_prefix_ = url_builder.str();

  boost::asio::io_service io_service;
  boost::asio::ip::tcp::endpoint endpoint( tcp::v4(), port_ );

  //vise_acceptor_ = new boost::asio::ip::tcp::acceptor( io_service , endpoint );
  //vise_acceptor_->set_option( tcp::acceptor::reuse_address(true) );
  tcp::acceptor acceptor ( io_service , endpoint );
  acceptor.set_option( tcp::acceptor::reuse_address(true) );

  std::cout << "\nServer started on port " << port << " :-)" << std::flush;

  /*
  // DEBUG
  LoadSearchEngine( "ox5k" );
  QueryInit();
  //QueryTest();

  return;
  */

  /*  */
  try {
    while ( !vise_shutdown_flag_ ) {
      boost::shared_ptr<tcp::socket> p_socket( new tcp::socket(io_service) );
      //vise_acceptor_->accept( *p_socket );
      acceptor.accept( *p_socket );
      boost::thread t( boost::bind( &ViseServer::HandleConnection, this, p_socket ) );
    }
  } catch (std::exception &e) {
    std::cerr << "\nCannot listen for http request!\n" << e.what() << std::flush;
    return;
  }
}

bool ViseServer::Stop() {
  return true;
}

void ViseServer::HandleConnection(boost::shared_ptr<tcp::socket> p_socket) {
  size_t bytes_read = p_socket->read_some(boost::asio::buffer( buffer_ ), error_);
  if ( error_ ) {
    std::cerr << "ViseServer::HandleConnection() : error reading http request" << std::endl;
    std::cerr << error_.message() << std::endl;
    boost::system::error_code ignored_err;
    p_socket->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ignored_err );
    return;
  }
  std::string http_request(buffer_.data(), buffer_.data() + bytes_read);
  std::string http_method  = http_request.substr(0, 4);
  std::string http_method_uri;
  ExtractHttpResource(http_request, http_method_uri);

  // for debug
  std::cout << "\n" << http_method << " " << http_method_uri << " [http request = " << http_request.length() << "]" << std::flush;
  //std::cout << "\nRequest = " << http_request << std::flush;

  if ( http_method == "GET " ) {
    if ( http_method_uri == "" ) {
      p_socket->close();
      return;
    }
    if ( http_method_uri == "/" ) {
      // show help page when user enteres http://localhost:8080
      SendHttpResponse( vise_main_html_, p_socket);
      p_socket->close();

      /*
      // debug
      SendCommand("_progress show");
      for ( unsigned int i=0; i<101; i++ ) {
        SendProgress( "Main", i, 100 );
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
      }
      SendCommand("_progress hide");
      */
      return;
    }

    if ( http_method_uri == "/_vise_index.html" ) {
      vise_index_html_reload_ = true; // @todo
      if ( vise_index_html_reload_ ) {
        GenerateViseIndexHtml();
      }
      SendHttpResponse(vise_index_html_, p_socket);
      p_socket->close();
      return;
    }

    if ( http_method_uri == "/_state" ) {
      // @todo
      // json reply containing current state info.
      std::string state_json = GetStateJsonData();
      SendJsonResponse( state_json, p_socket );
      p_socket->close();
      return;
    }

    if ( http_method_uri == "/favicon.ico" ) {
      // @todo not implemented yet
      SendHttp404NotFound( p_socket );
      p_socket->close();
      return;
    }

    if ( http_method_uri == "/vise.css" ) {
      SendRawResponse( "text/css", vise_css_, p_socket );
      p_socket->close();
      return;
    }

    if ( http_method_uri == "/vise.js" ) {
      SendRawResponse( "application/javascript", vise_js_, p_socket );
      p_socket->close();
      return;
    }

    const std::string message_prefix = "/_message";
    if ( StringStartsWith(http_method_uri, message_prefix) ) {
      // since HTTP server always requires a request in order to send responses,
      // we always create this _message channel which keeps waiting for messages
      // to be pushed to vise_message_queue_, sends this message to the client
      // which in turn again creates another request for any future messages

      std::string msg = ViseMessageQueue::Instance()->BlockingPop();
      SendRawResponse( "text/plain", msg, p_socket );
      p_socket->close();
      return;
    }

    const std::string query_prefix = "/_query";
    if ( StringStartsWith(http_method_uri, query_prefix) ) {
      HandleQueryGetRequest( http_method_uri, p_socket );
      p_socket->close();
      return;
    }

    // if http_method_uri contains request for static resource in the form:
    //   GET /_static/ox5k/dir1/dir2/all_souls_000022.jpg?original
    // return the static resource as binary data in HTTP response
    const std::string static_resource_prefix = "/_static/";
    if ( StringStartsWith(http_method_uri, static_resource_prefix) ) {
      std::string resource_uri = http_method_uri.substr( static_resource_prefix.size(), std::string::npos);
      std::string resource_name;
      std::map< std::string, std::string > resource_args;
      ParseHttpMethodUri( resource_uri, resource_name, resource_args);
      ServeStaticResource( resource_name, resource_args, p_socket );
      p_socket->close();
      return;
    }

    if ( http_method_uri == "/_random_image" ) {
      std::cout << "\nsearch_engine_.IsImglistEmpty() = " << search_engine_.IsImglistEmpty() << std::flush;
      // respond with a random training image
      if ( !search_engine_.IsImglistEmpty() ) {
        int index = rand() % search_engine_.GetImglistSize();
        std::string im_fn = search_engine_.GetImglistFn(index);
        std::ostringstream s;
        s << "<div class=\"random_image\"><img src=\"/_static/"
          << search_engine_.GetName() << "/"
          << im_fn << "?variant=original\"/></div>";
        SendHttpResponse( s.str(), p_socket );

        std::cout << "\n\tim_fn = " << im_fn << std::flush;
        //SendStaticImageResponse(im_fn, p_socket);
      }
      p_socket->close();
      return;
    }

    const std::string dired_prefix = "/_dired?path=";
    if ( StringStartsWith(http_method_uri, dired_prefix) ) {
      HandleDiredGetRequest( http_method_uri, p_socket );
      p_socket->close();
      return;
    }

    // request for state html
    // Get /Cluster
    std::vector< std::string > tokens;
    SplitString( http_method_uri, '/', tokens);
    if ( tokens.at(0) == "" && tokens.size() == 2 ) {
      std::string resource_name;
      std::map< std::string, std::string > resource_args;

      // check if the GET request contains parameters of the form
      // /Query?cmd=show_img_list&collection=all
      std::size_t qmark_pos = http_method_uri.find('?');
      if ( qmark_pos != std::string::npos ) {
        ParseHttpMethodUri( http_method_uri, resource_name, resource_args );
      } else {
        resource_name = tokens.at(1);
      }
      HandleStateGetRequest( resource_name, resource_args, p_socket );
      p_socket->close();
      return;
    }

    // otherwise, say not found
    SendHttp404NotFound( p_socket );
    p_socket->close();
    return;
  }
  if ( http_method == "POST" ) {
    std::string http_post_data;
    ExtractHttpContent(http_request, http_post_data);

    if ( http_method_uri == "/" ) {
      // the http_post_data can be one of these:
      //  * create_search_engine  _NAME_OF_SEARCH_ENGINE_
      //  * load_search_engine    _NAME_OF_SEARCH_ENGINE_
      //  * stop_training_process _NAME_OF_SEARCH_ENGINE_
      std::vector< std::string > tokens;
      SplitString( http_post_data, ' ', tokens);

      if ( tokens.size() == 2 ) {
        std::string search_engine_name = tokens.at(1);
        if ( tokens.at(0) == "create_search_engine" ) {
          if ( SearchEngineExists( search_engine_name ) ) {
            SendMessage("Search engine by that name already exists!");
          } else {
            if ( search_engine_.IsEngineNameValid( search_engine_name ) ) {
              state_id_ = ViseServer::STATE_NOT_LOADED;
              search_engine_.Init( search_engine_name, vise_enginedir_ );

              if ( UpdateState() ) {
                // send control message : state updated
                SendCommand("_state update_now");
                SendHttpPostResponse( http_post_data, "OK", p_socket );
                SendCommand("_state update_now");
              } else {
                SendMessage("Cannot initialize search engine [" + search_engine_name + "]");
                SendHttpPostResponse( http_post_data, "ERR", p_socket );
              }
            } else {
              SendMessage("Search engine name cannot contains spaces, or special characters ( *,?,/ )");
              SendHttpPostResponse( http_post_data, "ERR", p_socket );
            }
          }
          p_socket->close();
          return;
          // send control message to set loaded engine name
        } else if ( tokens.at(0) == "load_search_engine" ) {
            if ( search_engine_.GetName() == search_engine_name ) {
              SendHttpPostResponse( http_post_data, "Search engine already loaded!", p_socket );
              SendCommand(""); // @todo: this is needed to wakeup the messaging system! But why?
              SendCommand("_state update_now");
            } else {
              if ( SearchEngineExists( search_engine_name ) ) {
                SendMessage("Loading search engine [" + search_engine_name + "] ...");
                LoadSearchEngine( search_engine_name );
                SendHttpPostResponse( http_post_data, "Loaded search engine", p_socket );
              } else {
                SendHttpPostResponse( http_post_data, "Search engine does not exist", p_socket );
                SendMessage("Search engine does not exists!");
              }
            }
            p_socket->close();
            return;
        } else if ( tokens.at(0) == "msg_to_training_process" ) {
          std::string msg = tokens.at(1);
          if ( msg == "stop" ) {
            vise_training_thread_->interrupt();
          } else if ( msg == "continue" ) {
            vise_training_thread_ = new boost::thread( boost::bind( &ViseServer::InitiateSearchEngineTraining, this ) );
          }
        } else if ( tokens.at(0) == "shutdown_vise" ) {
          std::string msg = tokens.at(1);
          if ( msg == "now" ) {
            std::cout << "\nShutdown feature not implemented yet!" << std::flush;
            p_socket->close();
            return;
          }
        } else {
          // unknown command
          SendHttp404NotFound( p_socket );
          p_socket->close();
          return;
        }
      } else {
        // unexpected POST data
        SendHttp404NotFound( p_socket );
        p_socket->close();
        return;
      }
    } else {
      std::string resource_name = http_method_uri.substr(1, std::string::npos); // remove prefix "/"
      int state_id = GetStateId( resource_name );
      if ( state_id != -1 ) {
        HandleStatePostData( state_id, http_post_data, p_socket );
        p_socket->close();
        return;
      }
    }
    // otherwise, say not found
    SendHttp404NotFound( p_socket );
    p_socket->close();
    return;
  }
}


//
// Communications with HTTP client
//
void ViseServer::SendErrorResponse(std::string message, std::string backtrace, boost::shared_ptr<tcp::socket> p_socket) {
  std::string html;
  html  = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
  html += "<title>VGG Image Search Engine</title></head>";
  html += "<body><h1>Error !</h1>";
  html += "<p>" + message + "</p>";
  html += "<p><pre>" + backtrace + "</pre></p>";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  html += "<p>&nbsp;</p>";
  html += "<p>Generated on " + std::string(date_str) + "</p>";
  html += "</body></html>";

  SendHttpResponse(html, p_socket);
}

void ViseServer::SendRawResponse(std::string content_type,
                                 std::string content,
                                 boost::shared_ptr<tcp::socket> p_socket) {
  std::stringstream http_response;
  http_response << "HTTP/1.1 200 OK\r\n";
  http_response << "Content-type: " << content_type << "; charset=utf-8\r\n";
  http_response << "Content-Length: " << content.length() << "\r\n";
  http_response << "\r\n";
  http_response << content;

  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );
  //std::cout << "\nSent response : [" << response << "]" << std::flush;
}

void ViseServer::SendJsonResponse(std::string json, boost::shared_ptr<tcp::socket> p_socket) {
  std::stringstream http_response;
  http_response << "HTTP/1.1 200 OK\r\n";
  http_response << "Content-type: application/json; charset=utf-8\r\n";
  http_response << "Content-Length: " << json.length() << "\r\n";
  http_response << "Connection: keep-alive\r\n";
  http_response << "\r\n";

  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );
  boost::asio::write( *p_socket, boost::asio::buffer(json) );
  //std::cout << "\nSent json response : [" << json << "]" << std::flush;
}

void ViseServer::SendHttpResponse(std::string response, boost::shared_ptr<tcp::socket> p_socket) {
  std::stringstream http_response;
  http_response << "HTTP/1.1 200 OK\r\n";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  http_response << "Date: " << date_str << "\r\n";
  http_response << "Content-Language: en\r\n";
  http_response << "Connection: close\r\n";
  http_response << "Cache-Control: no-cache\r\n";
  http_response << "Content-type: text/html; charset=utf-8\r\n";
  http_response << "Content-Encoding: utf-8\r\n";
  http_response << "Content-Length: " << response.length() << "\r\n";
  http_response << "\r\n";

  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );
  boost::asio::write( *p_socket, boost::asio::buffer(response) );

  //std::cout << "\nSent http html response of length : " << response.length() << std::flush;
}

void ViseServer::SendStaticImageResponse(boost::filesystem::path im_fn, boost::shared_ptr<tcp::socket> p_socket) {
  std::string content_type = GetHttpContentType(im_fn);
  std::stringstream http_response;
  http_response << "HTTP/1.1 200 OK\r\n";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  http_response << "Date: " << date_str << "\r\n";
  http_response << "Content-Language: en\r\n";
  http_response << "Connection: close\r\n";

  http_response << "Content-type: " << content_type << "\r\n";
  http_response << "Content-Encoding: utf-8\r\n";
  http_response << "Content-Length: " << boost::filesystem::file_size( im_fn ) << "\r\n";
  http_response << "\r\n";
  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );

  // write the image file contents to socket
  try {
    std::ifstream im( im_fn.string().c_str(), std::ios::binary );
    if ( im.is_open() ) {
      const unsigned int BUF_SIZE = 1024 * 1024; // 1024 KB
      char* im_buf = new char[BUF_SIZE];
      while ( !im.eof() ) {
        memset(im_buf, 0, BUF_SIZE);
        im.read(im_buf, BUF_SIZE);
        unsigned int bytes_read = im.gcount();
        boost::asio::write( *p_socket,
                            boost::asio::buffer(im_buf, bytes_read),
                            boost::asio::transfer_all());
      }
      im.close();
      delete( im_buf);
    }
  } catch( std::exception &e) {
    std::cerr << "\nViseServer::SendStaticImageResponse() : exception " << e.what() << std::flush;
  }
}

void ViseServer::SendImageResponse(Magick::Image &im,
                                   std::string content_type,
                                   boost::shared_ptr<tcp::socket> p_socket) {
  Magick::Blob im_blob;
  try {
    im.magick("JPEG");
    im.write( &im_blob );
  } catch( std::exception &e) {
    std::cerr << "\nViseServer::SendImageResponse() : exception " << e.what() << std::flush;
    SendHttp404NotFound( p_socket );
  }

  std::stringstream http_response;
  http_response << "HTTP/1.1 200 OK\r\n";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  http_response << "Date: " << date_str << "\r\n";
  http_response << "Content-Language: en\r\n";
  http_response << "Connection: close\r\n";

  http_response << "Content-type: " << content_type << "\r\n";
  http_response << "Content-Encoding: utf-8\r\n";
  http_response << "Content-Length: " << im_blob.length() << "\r\n";
  http_response << "\r\n";
  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );

  // write the image file contents to socket
  try {
    char* im_buf = (char *) im_blob.data();
    boost::asio::write( *p_socket,
                        boost::asio::buffer(im_buf, im_blob.length()),
                        boost::asio::transfer_all());
  } catch( std::exception &e) {
    std::cerr << "\nViseServer::SendImageResponse() : exception " << e.what() << std::flush;
    SendHttp404NotFound( p_socket );
  }
}

void ViseServer::SendMessage(std::string message) {
  SendPacket( "message", message);
}

void ViseServer::SendLog(std::string log) {
  SendPacket( "log", log );
}

void ViseServer::SendCommand(std::string command) {
  SendPacket("command", command );
}

void ViseServer::SendProgress(std::string state_name, unsigned long completed, unsigned long total) {
  std::ostringstream s;
  s << state_name << " progress " << completed << "/" << total;
  ViseMessageQueue::Instance()->Push( s.str() );
}

void ViseServer::SendPacket(std::string type, std::string message) {
  std::ostringstream s;
  s << GetCurrentStateName() << " " << type << " " << message;
  ViseMessageQueue::Instance()->Push( s.str() );
}

void ViseServer::SendHttpPostResponse(std::string http_post_data, std::string result, boost::shared_ptr<tcp::socket> p_socket) {
  std::string response = "{ \"id\": \"http_post_response\",";
  response += "\"http_post_data\": \"" + http_post_data + "\",";
  response += "\"result\": \"" + result + "\" }";

  std::ostringstream http_response;
  http_response << "HTTP/1.1 200 OK\r\n";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  http_response << "Date: " << date_str << "\r\n";
  http_response << "Connection: Closed\r\n";
  http_response << "Content-type: application/json\r\n";
  http_response << "Content-Length: " << response.length() << "\r\n";
  http_response << "\r\n";
  http_response << response;

  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );
}

void ViseServer::SendHttp404NotFound(boost::shared_ptr<tcp::socket> p_socket) {
  std::string html;
  html  = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
  html += "<title>VGG Image Search Engine</title></head>";
  html += "<body><h1>You seem to be lost.</h1>";
  html += "<p>Don't despair. Getting lost is a good sign -- it means that you are exploring.</p>";
  html += "</body></html>";

  std::stringstream http_response;

  http_response << "HTTP/1.1 404 Not Found\r\n";
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S %Z", std::gmtime(&t));
  http_response << "Date: " << date_str << "\r\n";
  http_response << "Content-type: text/html\r\n";
  http_response << "Content-Length: " << html.length() << "\r\n";
  http_response << "\r\n";
  http_response << html;
  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );
}

void ViseServer::SendHttp400BadRequest(boost::shared_ptr<tcp::socket> p_socket) {
  std::string http_response = "HTTP/1.1 400 Bad Request\r\n\r\n";
  boost::asio::write( *p_socket, boost::asio::buffer(http_response) );
}

void ViseServer::SendHttpRedirect( std::string redirect_uri,
                                   boost::shared_ptr<tcp::socket> p_socket)
{
  std::stringstream http_response;
  http_response << "HTTP/1.1 303 See Other\r\n";
  http_response << "Location: " << (url_prefix_ + redirect_uri) << "\r\n";
  std::cout << "\nRedirecting to : " << (url_prefix_ + redirect_uri) << std::flush;
  boost::asio::write( *p_socket, boost::asio::buffer(http_response.str()) );
}

//
// Handle HTTP Get requests (used to query search engine)
//
void ViseServer::HandleStateGetRequest( std::string resource_name,
                                        std::map< std::string, std::string> resource_args,
                                        boost::shared_ptr<tcp::socket> p_socket ) {
  //std::cout << "\nViseServer::HandleStateGetRequest() : resource_name = " << resource_name << std::flush;
  std::map< std::string, std::string >::iterator it;

  int state_id = GetStateId( resource_name );
  if ( state_id != -1 ) {
    if ( state_id == ViseServer::STATE_QUERY && GetCurrentStateId() == ViseServer::STATE_QUERY ) {
      return;
      /*
      if ( resource_args.empty() ) {
        std::cout << "\nViseServer::HandleStateGetRequest() : sending query" << std::flush;
        SendCommand("_state hide");
        QueryServeImgList( 0, 20, p_socket );
        return;
      }
      */
    } else {
      switch( state_id ) {
      case ViseServer::STATE_SETTING:
        state_html_list_.at(state_id) = state_html_template_list_.at(state_id);
        ReplaceString( state_html_list_.at(state_id), "__SEARCH_ENGINE_NAME__", search_engine_.GetName() );
        SendCommand("_state show");
        SendCommand("_dired fetch " + user_home_dir_.string() );
        break;

      case ViseServer::STATE_INFO:
        state_html_list_.at( ViseServer::STATE_INFO ) = GetStateComplexityInfo();
        SendCommand("_control_panel add <div class=\"action_button\" onclick=\"_vise_server_send_state_post_request('Info', 'proceed')\">Proceed</div>");
        break;
      }
      SendHttpResponse( state_html_list_.at(state_id), p_socket );
      return;
    }
  }
}

void ViseServer::HandleQueryGetRequest(std::string http_method_uri, boost::shared_ptr<tcp::socket> p_socket) {
  std::string resource_name;
  std::map < std::string, std::string> args;
  ParseHttpMethodUri( http_method_uri, resource_name, args );

  std::string cmd = args.find("cmd")->second;

  if ( cmd == "show_img_list" ) {
    std::string page = args.find( "page" )->second;
    std::string imcount = args.find( "imcount" )->second;
    unsigned int page_i, imcount_i;

    std::istringstream s(page);
    s >> page_i;
    s.clear();
    s.str("");
    s.str(imcount);
    s >> imcount_i;

    QueryServeImgList(page_i, imcount_i, p_socket);
    return;
  } else if ( cmd == "search_img_region") {
    std::string img_uri  = args.find( "img_fn" )->second;
    std::string x0y0x1y1_str = args.find( "x0y0x1y1" )->second;

    unsigned int x0,y0,x1,y1;
    char comma;
    std::istringstream csv( x0y0x1y1_str );
    csv >> x0 >> comma
        >> y0 >> comma
        >> x1 >> comma >> y1;

    std::string img_uri_prefix = "/_static/" + search_engine_.GetName() + "/";
    std::string img_fn = img_uri.substr(img_uri_prefix.length(), std::string::npos);
    QuerySearchImageRegion(img_fn, x0, y0, x1, y1, p_socket);

  } else if ( cmd == "image_compare") {
    std::string im1fn    = args.find( "im1fn" )->second;
    std::string im2fn    = args.find( "im2fn" )->second;
    std::string H_str    = args.find( "H" )->second;
    std::string x0y0x1y1_str = args.find( "x0y0x1y1" )->second;

    QueryCompareImage(im1fn, im2fn, x0y0x1y1_str, H_str, p_socket);
  }
  SendHttp404NotFound( p_socket );
}

void ViseServer::ServeStaticResource(const std::string resource_name,
                                     const std::map< std::string, std::string> &resource_args,
                                     boost::shared_ptr<tcp::socket> p_socket) {
  // resource uri format:
  // SEARCH_ENGINE_NAME/...path...
  //std::cout << "\nViseServer::ServeStaticResource() : " << resource_uri << ", " << resource_arg << std::flush;
  std::size_t first_slash_pos = resource_name.find('/', 0); // ignore the first /
  if ( first_slash_pos != std::string::npos ) {
    std::string search_engine_name = resource_name.substr(0, first_slash_pos);
    if ( search_engine_.GetName() == search_engine_name ) {
      std::string res_rel_path = resource_name.substr(first_slash_pos+1, std::string::npos);
      boost::filesystem::path res_fn = search_engine_.GetTransformedImageDir() / res_rel_path;
      if ( resource_args.empty() ) {
        if ( boost::filesystem::exists(res_fn) ) {
          SendStaticImageResponse( res_fn, p_socket );
        } else {
          SendHttp404NotFound( p_socket );
        }
        return;
      }

      if ( resource_args.count("variant") == 1 ) {
        if ( resource_args.find("variant")->second == "original" ) {
          res_fn = search_engine_.GetOriginalImageDir() / res_rel_path;
        }
      }

      if ( ! boost::filesystem::exists(res_fn) ) {
        SendHttp404NotFound( p_socket );
        return;
      }

      try {
        Magick::Image im;
        im.read( res_fn.string() );

        QueryTransformImage( im, resource_args );

        std::string content_type = GetHttpContentType(res_fn);
        SendImageResponse( im, content_type, p_socket );
      } catch ( std::exception &error ) {
        SendHttp404NotFound( p_socket );
      }
    }
  }
  SendHttp404NotFound( p_socket );
  return;
}

void ViseServer::QueryTransformImage(Magick::Image &im,
                                     const std::map< std::string, std::string > &resource_args) {
  bool crop = false;
  bool draw_region = false;
  bool scale = false;

  unsigned int x0, y0, x1, y1;
  char comma;
  unsigned int sw, sh;
  std::stringstream s;

  if ( resource_args.count("crop") == 1 ) {
    if ( resource_args.find("crop")->second == "true" ) {
      crop = true;
    }
  }

  if ( resource_args.count("scale") == 1 ) {
    if ( resource_args.find("scale")->second == "true" ) {
      scale = true;

      s.clear(); s.str("");
      s.str( resource_args.find("sw")->second );
      s >> sw;

      s.clear(); s.str("");
      s.str( resource_args.find("sh")->second );
      s >> sh;
    }
  }

  if ( resource_args.count("draw_region") == 1 ) {
    if ( resource_args.find("draw_region")->second == "true" ) {
      draw_region = true;
    }
  }

  if ( resource_args.count("x0y0x1y1") == 1 ) {
    std::string x0y0x1y1_str = resource_args.find("x0y0x1y1")->second;
    std::istringstream csv( x0y0x1y1_str );
    csv >> x0 >> comma
        >> y0 >> comma
        >> x1 >> comma >> y1;
    //std::cout << "\nViseServer::QueryTransformImage() : x0=" << x0 << ",y0=" << y0 << ",x1=" << x1 << ",y1=" << y1 << std::flush;
  }

  unsigned int tx0, ty0, tx1, ty1;
  std::string Hstr;
  double H[9];
  if ( resource_args.count("H") == 1 ) {
    Hstr = resource_args.find("H")->second;
    std::istringstream csv( Hstr );
    csv >> H[0] >> comma >> H[1] >> comma >> H[2] >> comma;
    csv >> H[3] >> comma >> H[4] >> comma >> H[5] >> comma;
    csv >> H[6] >> comma >> H[7] >> comma >> H[8];
    HomographyTransform( H,
                         x0,  y0,  x1,  y1,
                         tx0, ty0, tx1, ty1);
  }

  if ( crop == true ) {
    if ( Hstr == "" ) {
      im.chop( Magick::Geometry( x0    , y0    ) );
      im.crop( Magick::Geometry( x1-x0 , y1-y0 ) );

      if ( scale ) {
        Magick::Geometry s0 = im.size();
        double aspect_ratio = ((double) s0.width()) / ((double) s0.height());
        unsigned int new_height = sh;
        unsigned int new_width = ((unsigned int) (new_height * aspect_ratio));

        im.zoom( Magick::Geometry( new_width, new_height ) );
      }
      return;
    } else {
      im.chop( Magick::Geometry( tx0     , ty0    ) );
      im.crop( Magick::Geometry( tx1-tx0 , ty1-ty0 ) );

      if ( scale ) {
        Magick::Geometry s0 = im.size();
        double aspect_ratio = ((double) s0.width()) / ((double) s0.height());
        unsigned int new_height = sh;
        unsigned int new_width = ((unsigned int) (new_height * aspect_ratio));

        im.zoom( Magick::Geometry( new_width, new_height ) );
      }
      return;
    }
  }

  if ( draw_region ) {
    im.strokeColor("yellow");
    im.strokeAntiAlias(false);
    im.fillColor( "transparent" );
    im.strokeWidth( 2.0 );

    if ( Hstr != "" ) {
      im.draw( Magick::DrawableRectangle(tx0, ty0, tx1, ty1) );
    } else {
      im.draw( Magick::DrawableRectangle(x0, y0, x1, y1) );
    }
    /*
      std::cout << "\nViseServer::QueryTransformImage() : Drawn rect. at [(" << tx0 << "," << ty0 << ") : (" << tx1 << "," << ty1 << ")], H = " << std::flush;
      for ( unsigned int i=0; i<9; i++ ) {
      std::cout << H[i] << "," << std::flush;
      }
    */
  }
  //std::cout << "\nViseServer::QueryTransformImage() : s = " << s << std::flush;
  //std::cout << "\nViseServer::QueryTransformImage() : s_tx = " << s_tx << std::flush;
}

void ViseServer::HomographyTransform( double H[],
                                      double   x0, double   y0, double   x1, double   y1,
                                      unsigned int &tx0, unsigned int &ty0, unsigned int &tx1, unsigned int &ty1) {

  double x_H_tx[4] = {0.0, 0.0, 0.0, 0.0};
  double y_H_tx[4] = {0.0, 0.0, 0.0, 0.0};

  HomographyPointTransform( H, x0   , y0   , x_H_tx[0], y_H_tx[0] );
  HomographyPointTransform( H, x1   , y0   , x_H_tx[1], y_H_tx[1] );
  HomographyPointTransform( H, x1   , y1   , x_H_tx[2], y_H_tx[2] );
  HomographyPointTransform( H, x0   , y1   , x_H_tx[3], y_H_tx[3] );

  tx0 = (unsigned int) *std::min_element( x_H_tx, x_H_tx+3 );
  ty0 = (unsigned int) *std::min_element( y_H_tx, y_H_tx+3 );
  tx1 = (unsigned int) *std::max_element( x_H_tx, x_H_tx+3 );
  ty1 = (unsigned int) *std::max_element( y_H_tx, y_H_tx+3 );
  //std::cout << "\nViseServer::HomographyTransform() : tx0=" << tx0 << ", ty0=" << ty0 << ", tx1=" << tx1 << ", ty1=" << ty1 << std::flush;
}

void ViseServer::HomographyPointTransform( double H[], const double x, const double y, double &xt, double &yt ) {
  xt = H[0]*x + H[1]*y + H[2];
  yt = H[3]*x + H[4]*y + H[5];
  double h = H[6]*x + H[7]*y + H[8];
  xt = xt / h;
  yt = yt / h;
  //std::cout << "\n\tx=" << x << ", y=" << y << " : xt=" << xt << ", yt=" << yt << std::flush;
}

//
// Handler of HTTP POST data for States
//
void ViseServer::HandleStatePostData( int state_id, std::string http_post_data, boost::shared_ptr<tcp::socket> p_socket ) {
  if ( state_id == ViseServer::STATE_SETTING ) {
    search_engine_.SetEngineConfig(http_post_data);
    search_engine_.WriteConfigToFile();
    if ( search_engine_.GetImglistSize() == 0 ) {
      search_engine_.CreateFileList();
    }
    UpdateStateInfoList();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
    }
  } else if ( state_id == ViseServer::STATE_INFO ) {
    if ( http_post_data == "proceed" ) {
      if ( UpdateState() ) {
        // send control message : state updated
        SendCommand("_state update_now");
        SendHttpPostResponse( http_post_data, "OK", p_socket );

        // initiate the search engine training process
        vise_training_thread_ = new boost::thread( boost::bind( &ViseServer::InitiateSearchEngineTraining, this ) );
      } else {
        SendHttpPostResponse( http_post_data, "ERR", p_socket );
      }
    }
  } else {
    std::cerr << "\nViseServer::HandleStatePostData() : Do not know how to handle this HTTP POST data!" << std::flush;
  }
}

//
// Search engine training
//
void ViseServer::InitiateSearchEngineTraining() {
  //SendCommand("_log clear show");
  SendCommand("_log clear hide");
  SendCommand("_control_panel clear all");
  SendCommand("_control_panel add <div id=\"toggle_log\" class=\"action_button\" onclick=\"_vise_toggle_log()\">Log</div>");
  //SendCommand("_control_panel add <div id=\"Training_button_stop\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('stop')\">Stop</div>");

  // Pre-process
  if ( state_id_ == ViseServer::STATE_PREPROCESS ) {
    boost::timer::cpu_timer t_start;
    search_engine_.Preprocess();
    boost::timer::cpu_times elapsed = t_start.elapsed();

    AddTrainingStat(search_engine_.GetName(),
                    GetCurrentStateName(),
                    elapsed.wall / 1e9,
                    search_engine_.GetImglistTransformedSize());

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
    } else {
      SendLog("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }
  /*
    if ( vise_training_thread_->interruption_requested() ) {
    SendLog("\nStopped training process on user request");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"Training_button_continue\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('continue')\">Continue</div>");
    return;
    }
  */

  // Descriptor
  if ( state_id_ == ViseServer::STATE_DESCRIPTOR ) {
    boost::timer::cpu_timer t_start;
    search_engine_.Descriptor();
    boost::timer::cpu_times elapsed = t_start.elapsed();

    AddTrainingStat(search_engine_.GetName(),
                    GetCurrentStateName(),
                    elapsed.wall / 1e9,
                    search_engine_.DescFnSize());

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }
  /*
    if ( vise_training_thread_->interruption_requested() ) {
    SendLog("\nStopped training process on user request");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"Training_button_continue\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('continue')\">Continue</div>");
    return;
    }
  */
  // Cluster
  if ( state_id_ == ViseServer::STATE_CLUSTER ) {
    boost::timer::cpu_timer t_start;
    search_engine_.Cluster( vise_source_code_dir_ );
    boost::timer::cpu_times elapsed = t_start.elapsed();

    AddTrainingStat(search_engine_.GetName(),
                    GetCurrentStateName(),
                    elapsed.wall / 1e9,
                    search_engine_.ClstFnSize());

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }
  /*
    if ( vise_training_thread_->interruption_requested() ) {
    SendLog("\nStopped training process on user request");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"Training_button_continue\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('continue')\">Continue</div>");
    return;
    }
  */

  // Assign
  if ( state_id_ == ViseServer::STATE_ASSIGN ) {
    boost::timer::cpu_timer t_start;
    search_engine_.Assign();
    boost::timer::cpu_times elapsed = t_start.elapsed();

    AddTrainingStat(search_engine_.GetName(),
                    GetCurrentStateName(),
                    elapsed.wall / 1e9,
                    search_engine_.AssignFnSize());

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }
  if ( vise_training_thread_->interruption_requested() ) {
    SendLog("\nStopped training process on user request");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"Training_button_continue\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('continue')\">Continue</div>");
    return;
  }

  // Hamm
  if ( state_id_ == ViseServer::STATE_HAMM ) {
    boost::timer::cpu_timer t_start;
    search_engine_.Hamm();
    boost::timer::cpu_times elapsed = t_start.elapsed();

    AddTrainingStat(search_engine_.GetName(),
                    GetCurrentStateName(),
                    elapsed.wall / 1e9,
                    search_engine_.HammFnSize());

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }
  if ( vise_training_thread_->interruption_requested() ) {
    SendLog("\nStopped training process on user request");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"Training_button_continue\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('continue')\">Continue</div>");
    return;
  }

  // Index
  if ( state_id_ == ViseServer::STATE_INDEX ) {
    boost::timer::cpu_timer t_start;
    search_engine_.Index();
    boost::timer::cpu_times elapsed = t_start.elapsed();

    AddTrainingStat(search_engine_.GetName(),
                    GetCurrentStateName(),
                    elapsed.wall / 1e9,
                    search_engine_.IndexFnSize());

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("_state update_now");

    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }
  if ( vise_training_thread_->interruption_requested() ) {
    SendLog("\nStopped training process on user request");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"Training_button_continue\" class=\"action_button\" onclick=\"_vise_send_msg_to_training_process('continue')\">Continue</div>");
    return;
  }

  QueryInit();
  SendCommand("_go_to home");
}

//
// Search engine query
//
void ViseServer::QueryServeImgList( unsigned int page_no,
                                    unsigned int per_page_im_count,
                                    boost::shared_ptr<tcp::socket> p_socket ) {
  std::ostringstream s;
  uint32_t start_doc_id = page_no * per_page_im_count;

  int32_t next_page_no = page_no + 1;
  int32_t prev_page_no = page_no - 1;
  if ( page_no == 0 ) {
    prev_page_no = -1;
  }

  if ( start_doc_id > dataset_->getNumDoc() ) {
    start_doc_id = dataset_->getNumDoc() - per_page_im_count;
    next_page_no = -1;
  }

  uint32_t end_doc_id   = start_doc_id + per_page_im_count;
  if ( end_doc_id > dataset_->getNumDoc() ) {
    end_doc_id = dataset_->getNumDoc();
  }

  s << "<ul class=\"img_list columns-4\">";
  s << "<input type=\"hidden\" name=\"page_no\" value=\"" << page_no << "\">";
  s << "<input type=\"hidden\" name=\"per_page_im_count\" value=\"" << per_page_im_count << "\">";
  s << "<input type=\"hidden\" name=\"next_page_no\" value=\"" << next_page_no << "\">";
  s << "<input type=\"hidden\" name=\"prev_page_no\" value=\"" << prev_page_no << "\">";

  for ( uint32_t doc_id=start_doc_id; doc_id < end_doc_id; doc_id++) {
    std::string im_fn  = dataset_->getInternalFn( doc_id );
    std::string im_uri = "/_static/" + search_engine_.GetName() + "/" + im_fn;
    std::pair<uint32_t, uint32_t> im_dim = dataset_->getWidthHeight( doc_id );

    s << "<li><img class=\"action_img\" "
      << "onclick=\"_vise_select_img_region('" << im_uri << "')\" "
      << "src=\"" << im_uri << "\" />"
      << "<h3>( " << (doc_id+1) << " of " << (dataset_->getNumDoc()+1) <<" ) " << im_fn << "</h3>"
      << "<p>" << im_dim.first << " x " << im_dim.second << " px</p></li>";
  }
  s << "</ul>";
  SendHttpResponse( s.str(), p_socket );
}

void ViseServer::QuerySearchImageRegion(std::string query_img_fn,
                                        unsigned int x0,
                                        unsigned int y0,
                                        unsigned int x1,
                                        unsigned int y1,
                                        boost::shared_ptr<tcp::socket> p_socket) {

  // ensure that search index loading is complete
  // see ViseServer::QueryInit()
  vise_load_search_index_thread_->join();

  SendCommand("_control_panel clear all");

  uint32_t doc_id = dataset_->getDocID( query_img_fn );
  query query_obj(doc_id, true, "",
                  x0, // xl
                  x1, // xu
                  y1, // yl
                  y0  // yu
                  );

  std::vector<indScorePair> queryRes;
  std::map<uint32_t,homography> Hs;

  spatial_retriever_->spatialQuery( query_obj, queryRes, Hs, 20 );

  std::ostringstream s;
  s << "<ul class=\"img_list columns-4\">";

  double H[9];
  for (uint32_t iRes= 0; (iRes < queryRes.size()) && (iRes < 20); ++iRes){
    uint32_t doc_id = queryRes[iRes].first;

    std::string im_fn  = dataset_->getInternalFn( doc_id );
    std::string im_uri = "/_static/" + search_engine_.GetName() + "/" + im_fn;
    std::pair<uint32_t, uint32_t> im_dim = dataset_->getWidthHeight( doc_id );
    Hs.find( doc_id )->second.exportToDoubleArray( H );

    std::ostringstream hcsv;
    hcsv << "H="
         << H[0] << "," << H[1] << "," << H[2] << ","
         << H[3] << "," << H[4] << "," << H[5] << ","
         << H[6] << "," << H[7] << "," << H[8];

    std::ostringstream region;
    region << "x0y0x1y1=" << x0 << "," << y0 << "," << x1 << "," << y1;

    std::ostringstream onclick;
    onclick << "imcomp("
            << "'" << query_img_fn << "',"
            << "'" << im_fn << "',"
            << "'" << region.str() << "',"
            << "'" << hcsv.str() << "')";

    s << "<li>"
      << "<img class=\"action_img\" onclick=\"" << onclick.str() << "\" "
      << "src=\"" << im_uri << "?crop=false&scale=false&draw_region=true&" << region.str() << "&" << hcsv.str() << "\" />"
      << "<h3>( " << iRes << " of " << queryRes.size() <<" ) " << im_fn << "</h3>"
      << "<p>Score = " << queryRes[iRes].second << "<br/>"
      <<    "Size  = " << im_dim.first << " x " << im_dim.second << " px</p></li>";
  }
  s << "</ul";
  SendHttpResponse( s.str(), p_socket );
}

void ViseServer::QueryCompareImage(std::string im1fn,
                                   std::string im2fn,
                                   std::string x0y0x1y1_str,
                                   std::string H_str,
                                   boost::shared_ptr<tcp::socket> p_socket) {

  std::ostringstream s;
  s << "<ul class=\"img_list columns-3\">";

  // image 1: left
  std::ostringstream im1_uri;
  im1_uri << "/_static/" << search_engine_.GetName() << "/" << im1fn;
  im1_uri << "?crop=false"
          << "&scale=false"
          << "&draw_region=true"
          << "&x0y0x1y1=" << x0y0x1y1_str;

  s << "<li>"
    << "<h3>Query Image</h3>"
    << "<img "
    << "src=\"" << im1_uri.str() << "\" />"
    << "<p>" << im1fn << "</p></li>";

  // image 1: crop
  std::ostringstream crop_im1_uri;
  crop_im1_uri << "/_static/" << search_engine_.GetName() << "/" << im1fn;
  crop_im1_uri << "?crop=true"
               << "&scale=false"
               << "&draw_region=false"
               << "&x0y0x1y1=" << x0y0x1y1_str;
  s << "<li>"
    << "<h3>Cropped query image</h3>"
    << "<img "
    << "src=\"" << crop_im1_uri.str() << "\" />"
    << "<p>Hover to show difference</p></li>";

  // image 2: crop
  unsigned int x0,y0,x1,y1;
  char comma;
  std::istringstream csv( x0y0x1y1_str );
  csv >> x0 >> comma
      >> y0 >> comma
      >> x1 >> comma >> y1;

  std::ostringstream crop_im2_uri;
  crop_im2_uri << "/_static/" << search_engine_.GetName() << "/" << im2fn;
  crop_im2_uri << "?crop=true"
               << "&scale=true"
               << "&draw_region=false"
               << "&x0y0x1y1=" << x0y0x1y1_str
               << "&H=" << H_str
               << "&sw=" << (x1-x0)
               << "&sh=" << (y1-y0);
  s << "<li>"
    << "<h3>Cropped search result</h3>"
    << "<img "
    << "src=\"" << crop_im2_uri.str() << "\" />"
    << "<p>Hover to show difference</p></li>";


  // image 2: right
  std::ostringstream im2_uri;
  im2_uri << "/_static/" << search_engine_.GetName() << "/" << im2fn;
  im2_uri << "?crop=false"
          << "&scale=false"
          << "&draw_region=true"
          << "&x0y0x1y1=" << x0y0x1y1_str
          << "&H=" << H_str;
  s << "<li>"
    << "<h3>Search result</h3>"
    << "<img "
    << "src=\"" << im2_uri.str() << "\" />"
    << "<p>" << im2fn << "</p></li>";

  s << "</ul>";
  SendHttpResponse( s.str(), p_socket );
}

void ViseServer::QueryInit() {
  /*
  // construct dataset
  dataset_ = new datasetV2( search_engine_.GetEngineConfigParam("dsetFn"),
  search_engine_.GetEngineConfigParam("databasePath"),
  search_engine_.GetEngineConfigParam("docMapFindPath") );

  // load the search index in separate thread
  // while the user browses image list and prepares search area
  vise_load_search_index_thread_ = new boost::thread( boost::bind( &ViseServer::QueryLoadSearchIndex, this ) );
  */
  boost::thread t( boost::bind( &ViseServer::InitReljaRetrival, this ) );
}

void ViseServer::QueryLoadSearchIndex() {
  // needed to setup forward and inverted index
  cons_queue_ = new sequentialConstructions();

  // setup forward index
  dbFidx_file_ = new protoDbFile( search_engine_.GetEngineConfigParam("fidxFn") );
  boost::function<protoDb*()> fidxInRamConstructor= boost::lambda::bind(boost::lambda::new_ptr<protoDbInRam>(),
                                                                        boost::cref(*dbFidx_file_) );
  dbFidx_ = new protoDbInRamStartDisk( *dbFidx_file_, fidxInRamConstructor, true, cons_queue_ );
  fidx_ = new protoIndex(*dbFidx_, false);

  // setup inverted index
  dbIidx_file_ = new protoDbFile( search_engine_.GetEngineConfigParam("iidxFn") );
  boost::function<protoDb*()> iidxInRamConstructor= boost::lambda::bind(boost::lambda::new_ptr<protoDbInRam>(),
                                                                        boost::cref(*dbIidx_file_) );
  dbIidx_ = new protoDbInRamStartDisk( *dbIidx_file_, iidxInRamConstructor, true, cons_queue_ );
  iidx_ = new protoIndex(*dbIidx_, false);
  cons_queue_->start(); // start the construction of in-RAM stuff

  // feature getter and assigner
  bool SIFTscale3  = false;
  if ( search_engine_.GetEngineConfigParam("SIFTscale3") == "true" ) {
    SIFTscale3 = true;
  }

  bool useRootSIFT = false;
  if ( search_engine_.GetEngineConfigParam("useRootSIFT") == "true" ) {
    useRootSIFT = true;
  }

  feat_getter_ = new featGetter_standard( (
                                           std::string("hesaff-") +
                                           std::string((useRootSIFT ? "rootsift" : "sift")) +
                                           std::string(SIFTscale3 ? "-scale3" : "")
                                           ).c_str() );

  // clusters
  clst_centres_ = new clstCentres( search_engine_.GetEngineConfigParam("clstFn").c_str(), true );

  nn_ = fastann::nn_obj_build_kdtree(clst_centres_->clstC_flat,
                                     clst_centres_->numClst,
                                     clst_centres_->numDims, 8, 1024);
  // soft assigner
  useHamm = false;
  uint32_t hammEmbBits;
  std::string hamm_emb_bits = search_engine_.GetEngineConfigParam("hammEmbBits");
  std::istringstream s(hamm_emb_bits);
  s >> hammEmbBits;
  if ( hamm_emb_bits != "" ) {
    useHamm = true;
  }

  if ( !useHamm ) {
    if ( useRootSIFT ) {
      soft_assigner_ = new SA_exp( 0.02 );
    } else {
      soft_assigner_ = new SA_exp( 6250 );
    }
  }
  if (useHamm){
    emb_factory_ = new hammingEmbedderFactory(search_engine_.GetEngineConfigParam("hammFn"), hammEmbBits);
  }
  else {
    emb_factory_ = new noEmbedderFactory;
  }
  // create retriever
  tfidf_ = new tfidfV2(iidx_,
                       fidx_,
                       search_engine_.GetEngineConfigParam("wghtFn"),
                       feat_getter_,
                       nn_,
                       soft_assigner_);

  if (useHamm) {
    hamming_emb_ = new hamming(*tfidf_,
                               iidx_,
                               *dynamic_cast<hammingEmbedderFactory const *>(emb_factory_),
                               fidx_,
                               feat_getter_, nn_, clst_centres_);
    base_retriever_ = hamming_emb_;
  } else {
    base_retriever_ = tfidf_;
  }

  // spatial verifier
  spatial_verif_v2_ = new spatialVerifV2(*base_retriever_, iidx_, fidx_, true, feat_getter_, nn_, clst_centres_);
  spatial_retriever_ = spatial_verif_v2_;

  // multiple queries
  multi_query_max_ = new multiQueryMax( *spatial_verif_v2_ );
  if (hamming_emb_ != NULL){
    multi_query_= new mqFilterOutliers(*multi_query_max_,
                                       *spatial_verif_v2_,
                                       *dynamic_cast<hammingEmbedderFactory const *>(emb_factory_) );
  } else {
    multi_query_ = multi_query_max_;
  }
}

void ViseServer::QueryTest() {
  // http://localhost:9669/dosearch?docID=0&xl=410&yl=100&xu=640&yu=460
  // 0    = 573.002600
  // 844  = 38.000000
  // 3458 = 37.000100
  // 503  = 7.000100
  // 2838 = 36.000100
  // 4976 = 34.000100
  // 2439 = 33.000100
  uint32_t docID = 0;
  query query_obj(docID,
                  true,
                  "",
                  410,
                  640,
                  100,
                  460);
  std::cout << "\nG" << std::flush;

  std::vector<indScorePair> queryRes;
  std::map<uint32_t,homography> Hs;


  spatial_retriever_->spatialQuery( query_obj, queryRes, Hs, 5 );
  std::cout << "\nH" << std::flush;

  std::cout << "\nNumber of docs = " << dataset_->getNumDoc();
  std::cout << "\nQuery result: queryRes.size() = " << queryRes.size() << std::flush;

  double h[9];
  for (uint32_t iRes= 0; (iRes < queryRes.size()) && (iRes < 5); ++iRes){
    uint32_t doc_id = queryRes[iRes].first;

    std::ostringstream s;
    if ( !Hs.empty() && Hs.count( doc_id ) ) {
      /*      */
      Hs.find( doc_id )->second.exportToDoubleArray( h );
      s << h[0] << "," << h[1] << "," << h[2] << ","
        << h[3] << "," << h[4] << "," << h[5] << ","
        << h[6] << "," << h[7] << "," << h[8];

    }
    std::cout << "\n\trank=" << iRes << ", docId=" << doc_id << ", score=" << queryRes[iRes].second << ", fn=" << dataset_->getInternalFn(doc_id) << ", H=" << s.str() << std::flush;
  }

  std::cout << "\nDONE" << std::flush;
}

//
// State based model
//
std::string ViseServer::GetCurrentStateName() {
  return GetStateName( state_id_ );
}

std::string ViseServer::GetCurrentStateInfo() {
  return GetStateInfo( state_id_ );
}

int ViseServer::GetStateId( std::string state_name ) {
  for ( unsigned int i=0; i < state_id_list_.size(); i++) {
    if ( state_name_list_.at(i) == state_name ) {
      return state_id_list_.at(i);
    }
  }
  return -1; // not found
}

std::string ViseServer::GetStateName(int state_id) {
  return state_name_list_.at( state_id );
}

std::string ViseServer::GetStateInfo(int state_id) {
  return state_info_list_.at( state_id );
}

int ViseServer::GetCurrentStateId() {
  return state_id_;
}

void ViseServer::ResetToInitialState() {
  state_id_ = ViseServer::STATE_NOT_LOADED;
}

bool ViseServer::UpdateState() {
  if ( state_id_ == ViseServer::STATE_NOT_LOADED ) {
    // check if search engine was initialized properly
    if ( search_engine_.GetName() == "" ) {
      return false;
    } else {
      state_id_ = ViseServer::STATE_SETTING;
      return true;
    }
  } else if ( state_id_ == ViseServer::STATE_SETTING ) {
    if ( search_engine_.IsEngineConfigEmpty() ) {
      return false;
    } else {
      state_id_ = ViseServer::STATE_INFO;
      return true;
    }
  } else if ( state_id_ == ViseServer::STATE_INFO ) {
    state_id_ = ViseServer::STATE_PREPROCESS;
    return true;
  } else if ( state_id_ == ViseServer::STATE_PREPROCESS ) {
    if ( search_engine_.EngineConfigFnExists() &&
         search_engine_.ImglistFnExists() ) {
      state_id_ = ViseServer::STATE_DESCRIPTOR;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == ViseServer::STATE_DESCRIPTOR ) {
    if ( search_engine_.DescFnExists() ) {
      state_id_ = ViseServer::STATE_CLUSTER;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == ViseServer::STATE_CLUSTER ) {
    if ( search_engine_.ClstFnExists() ) {
      state_id_ = ViseServer::STATE_ASSIGN;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == ViseServer::STATE_ASSIGN ) {
    if ( search_engine_.AssignFnExists() ) {
      state_id_ = ViseServer::STATE_HAMM;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == ViseServer::STATE_HAMM ) {
    if ( search_engine_.HammFnExists() ) {
      state_id_ = ViseServer::STATE_INDEX;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == ViseServer::STATE_INDEX ) {
    if ( search_engine_.IndexFnExists() ) {
      state_id_ = ViseServer::STATE_QUERY;
      return true;
    } else {
      return false;
    }
  }
  return false;
}

void ViseServer::LoadSearchEngine( std::string search_engine_name ) {
  ResetToInitialState();

  /*
    SendMessage("Loading search engine " + search_engine_name + " ...");

    SendCommand("_log clear show");
    SendCommand("_control_panel clear all");
    SendCommand("_control_panel add <div id=\"LoadSearchEngine_button_continue\" class=\"action_button\" onclick=\"_vise_server_send_state_post_request('Info', 'proceed')\">Continue</div>");
  */
  search_engine_.Init( search_engine_name, vise_enginedir_ );
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_SETTING );
  //SendMessage("[" + search_engine_name + "] Loading settings ...");

  std::string engine_config;
  LoadFile( search_engine_.GetEngineConfigFn().string(), engine_config );
  search_engine_.SetEngineConfig( engine_config );
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_INFO );

  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_PREPROCESS );
  //SendMessage("[" + search_engine_name + "] Loading pre-processed data ...");

  //search_engine_.Preprocess();
  search_engine_.LoadImglist();
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_DESCRIPTOR );
  //SendMessage("[" + search_engine_name + "] Loading descriptors ...");

  //search_engine_.Descriptor();
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_CLUSTER );
  //SendMessage("[" + search_engine_name + "] Loading clusters ...");

  //search_engine_.Cluster();
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_ASSIGN );
  //SendMessage("[" + search_engine_name + "] Loading assign ...");

  //search_engine_.Assign();
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_HAMM );
  //SendMessage("[" + search_engine_name + "] Loading hamm ...");

  //search_engine_.Hamm();
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_INDEX );
  //SendMessage("[" + search_engine_name + "] Loading index ...");

  //search_engine_.Index();
  if ( !UpdateState() ) {
    return;
  }
  assert( GetCurrentStateId() == ViseServer::STATE_QUERY );
  //SendMessage("[" + search_engine_name + "] Loading complete :-)");
  QueryInit();

  //SendCommand("_log clear hide");
  //SendCommand("_control_panel clear all");

  //SendCommand("_state update_now");
}

//
// methods that prepare UI data to be sent to the HTTP client
//
void ViseServer::GenerateViseIndexHtml() {
  std::ostringstream s;
  s << "<div id=\"create_engine_panel\">";
  s << "<input id=\"vise_search_engine_name\" name=\"vise_search_engine_name\" value=\"engine_name\" onclick=\"document.getElementById('vise_search_engine_name').value=''\" size=\"20\" autocomplete=\"off\">";
  s << "<div class=\"action_button\" onclick=\"_vise_create_search_engine()\">&nbsp;&nbsp;Create</div>";
  s << "</div>";
  s << "<div id=\"load_engine_panel\">";

  // iterate through all directories in vise_enginedir_
  boost::filesystem::directory_iterator dir_it( vise_enginedir_ ), end_it;
  while ( dir_it != end_it ) {
    boost::filesystem::path p = dir_it->path();
    if ( boost::filesystem::is_directory( p ) ) {
      std::string name = p.filename().string();
      s << "<a onclick=\"_vise_load_search_engine('" << name << "')\" title=\"" << name << "\">"
        << "<figure></figure>"
        << "<p>" << name << "</p>"
        << "</a>";
    }
    ++dir_it;
  }
  s << "</div>";
  vise_index_html_ = s.str();
  vise_index_html_reload_ = false;
}

std::string ViseServer::GetStateJsonData() {
  std::ostringstream json;
  json << "{ \"id\" : \"search_engine_state\",";
  json << "\"state_id_list\" : [ 0";
  for ( unsigned int i=1 ; i < state_id_list_.size(); i++ ) {
    json << "," << i;
  }
  json << "], \"state_name_list\" : [ \"" << state_name_list_.at(0) << "\"";
  for ( unsigned int i=1 ; i < state_id_list_.size(); i++ ) {
    json << ", \"" << state_name_list_.at(i) << "\"";
  }
  json << "], \"state_info_list\" : [ \"" << state_info_list_.at(0) << "\"";
  for ( unsigned int i=1 ; i < state_id_list_.size(); i++ ) {
    json << ", \"" << state_info_list_.at(i) << "\"";
  }
  json << "], \"current_state_id\" : " << state_id_ << ", ";
  json << "\"search_engine_name\" : \"" << search_engine_.GetName() << "\" }";

  return json.str();
}

//
// helper functions
//

// extract parts of the HTTP get resource name uri
// for example:
//   GET /Query?docId=526&x0=40&y0=20&x1=300&y1=400&results=20
//
void ViseServer::ParseHttpMethodUri(const std::string http_method_uri,
                                    std::string &resource_name,
                                    std::map< std::string, std::string > &resource_args) {
  if ( http_method_uri == "" ) {
    return;
  }

  std::vector< std::string > tokens;
  SplitString( http_method_uri, '?', tokens );
  // assert( tokens.size() == 2 );

  resource_name = tokens.at(0);
  if ( resource_name.at(0) == '/' ) {
    resource_name = resource_name.substr( 1, std::string::npos ); // remove prefix "/"
  }

  if ( tokens.size() == 1 ) {
    return;
  }

  std::string args = tokens.at(1);
  std::vector< std::string > args_tokens;
  SplitString( args, '&', args_tokens );

  for ( unsigned int i=0; i<args_tokens.size(); i++ ) {
    std::vector< std::string > argi;
    SplitString( args_tokens.at(i), '=', argi );
    // assert( argi.size() == 2 );

    std::string key   = argi.at(0);
    std::string value = argi.at(1);
    resource_args.insert( std::pair<std::string, std::string>(key, value) );
  }
}

void ViseServer::ExtractHttpResource(std::string http_request, std::string &http_resource) {
  unsigned int first_space  = http_request.find(' ', 0);
  unsigned int second_space = http_request.find(' ', first_space+1);

  unsigned int length = (second_space - first_space);
  http_resource = http_request.substr(first_space+1, length-1);
}

void ViseServer::ExtractHttpContent(std::string http_request, std::string &http_content) {
  std::string http_content_start_flag = "\r\n\r\n";
  unsigned int start = http_request.rfind(http_content_start_flag);
  http_content = http_request.substr( start + http_content_start_flag.length() );
}

int ViseServer::LoadFile(const std::string filename, std::string &file_contents) {
  try {
    std::ifstream f;
    f.open(filename.c_str());
    f.seekg(0, std::ios::end);
    file_contents.reserve( f.tellg() );
    f.seekg(0, std::ios::beg);

    file_contents.assign( std::istreambuf_iterator<char>(f),
                          std::istreambuf_iterator<char>() );
    f.close();
    return 1;
  } catch (std::exception &e) {
    std::cerr << "\nViseServer::LoadFile() : failed to load file : " << filename << std::flush;
    file_contents = "";
    return 0;
  }
}

void ViseServer::WriteFile(std::string filename, std::string &file_contents) {
  try {
    std::ofstream f;
    f.open(filename.c_str());
    f << file_contents;
    f.close();
  } catch (std::exception &e) {
    std::cerr << "\nViseServer::WriteFile() : failed to save file : " << filename << std::flush;
  }

}

bool ViseServer::ReplaceString(std::string &s, std::string old_str, std::string new_str) {
  std::string::size_type pos = s.find(old_str);
  if ( pos == std::string::npos ) {
    return false;
  } else {
    s.replace(pos, old_str.length(), new_str);
    return true;
  }
}

void ViseServer::SplitString( const std::string s, char sep, std::vector<std::string> &tokens ) {
  if ( s.length() == 0 ) {
    return;
  }

  unsigned int start = 0;
  for ( unsigned int i=0; i<s.length(); ++i) {
    if ( s.at(i) == sep ) {
      tokens.push_back( s.substr(start, (i-start)) );
      start = i + 1;
    }
  }
  tokens.push_back( s.substr(start, s.length()) );
}

bool ViseServer::StringStartsWith( const std::string &s, const std::string &prefix ) {
  if ( s.substr(0, prefix.length()) == prefix ) {
    return true;
  } else {
    return false;
  }
}

void ViseServer::UnescapeHttpString( std::string &s ) {
  std::string::size_type pos = 0;
  std::string::size_type old_pos = 0;
  const std::string utf_space = "%20";
  while ( pos < s.length() ) {
    pos = s.find(utf_space, old_pos);
    if ( pos == std::string::npos ) {
      return;
    } else {
      s.replace(pos, utf_space.length(), " ");
      old_pos = pos;
    }
  }
}

bool ViseServer::SearchEngineExists( std::string search_engine_name ) {
  // iterate through all directories in vise_enginedir_
  boost::filesystem::directory_iterator dir_it( vise_enginedir_ ), end_it;
  while ( dir_it != end_it ) {
    boost::filesystem::path p = dir_it->path();
    if ( boost::filesystem::is_directory( p ) ) {
      if ( search_engine_name == p.filename().string() ) {
        return true;
      }
    }
    ++dir_it;
  }
  return false;
}

std::string ViseServer::GetHttpContentType( boost::filesystem::path fn) {
  std::string ext = fn.extension().string();
  std::string http_content_type = "unknown";
  if ( ext == ".jpg" ) {
    http_content_type = "image/jpeg";
  } else if ( ext == ".png" ) {
    http_content_type = "image/png";
  }
  return http_content_type;
}

//
// dired : send the names of folders present in a path
//
void ViseServer::HandleDiredGetRequest(const std::string& dired_uri, boost::shared_ptr<tcp::socket> p_socket) {
  const std::string dired_prefix = "/_dired?path=";
  const std::string folder_prefix = "&folder=";

  std::size_t folder_pos = dired_uri.find( folder_prefix, dired_prefix.length() );
  //std::cout << "\nuri = " << dired_uri << " : " << folder_pos << std::flush;
  if ( folder_pos == std::string::npos ) {
    SendHttp400BadRequest(p_socket);
    return;
  }
  std::string path_str = dired_uri.substr( dired_prefix.length(), folder_pos - dired_prefix.length() );
  std::string folder_str = dired_uri.substr( folder_pos + folder_prefix.length() );

  if ( path_str.find("%") != std::string::npos ) {
    UnescapeHttpString(path_str);
  }
  if ( folder_str.find("%") != std::string::npos ) {
    UnescapeHttpString(folder_str);
  }

  boost::filesystem::path p( path_str );
  boost::filesystem::path fp = p / boost::filesystem::path( folder_str );

  //std::cout << "\npath_str = " << path_str << ", folder_str = " << folder_str << std::flush;
  if ( boost::filesystem::exists( p ) && boost::filesystem::exists( fp ) ) {
    try {
      boost::filesystem::path cp = boost::filesystem::canonical( fp );
      if ( !boost::filesystem::exists( cp ) ) {
        SendHttp400BadRequest(p_socket);
        return;
      }
      boost::filesystem::directory_iterator dir_it( cp ), end_it;

      //std::cout << "\n\tp = " << p.string() << ", fp = " << fp.string() << ", cp = " << cp.string() <<std::flush;
      std::ostringstream json;
      json << "{ \"id\":\"dired\","
           << "\"path\":\"" << cp.string() << "\","
           << "\"folders\":[\"..\"";

      std::locale locale;
      while ( dir_it != end_it ) {
        boost::filesystem::path dir = dir_it->path();
        boost::filesystem::file_status s = boost::filesystem::status( dir );
        unsigned int perm = s.permissions();
        unsigned int user_perm = perm >> 6;
        //unsigned int other_perm = perm & 7;

        ++dir_it;
        // user_perm of 4,5,6,7 correponds to read access for owner
        if ( boost::filesystem::is_directory( dir ) && user_perm > 3) {
          std::string folder_name = dir.filename().string();
          //std::cout << "\n\tfolder = " << dir.filename().string() << " : " << s.permissions() << " : " << user_perm << " : " << other_perm << std::flush;
          if ( folder_name.substr(0,1) != "." ) {
            json << ",\"" << dir.filename().string() << "\"";
          }
        }
      }

      json << "]}";
      SendJsonResponse( json.str(), p_socket );
    } catch( std::exception &e ) {
      std::cerr << "\nViseServer::HandleDiredGetRequest() : failed to retrive contents of [" << fp.string() << "]";
      std::cerr << "\n" << e.what() << std::flush;
      SendHttp400BadRequest(p_socket);
      return;
    }
  } else {
    SendHttp400BadRequest(p_socket);
  }
}


// logging statistics
//
void ViseServer::AddTrainingStat(std::string dataset_name, std::string state_name, unsigned long time_sec, unsigned long space_bytes) {
  std::time_t t = std::time(NULL);
  char date_str[100];
  std::strftime(date_str, sizeof(date_str), "%F,%T", std::gmtime(&t));

  training_stat_f << "\n" << date_str  << "," << dataset_name << "," << search_engine_.GetImglistSize() << "," << state_name << "," << time_sec << "," << space_bytes << std::flush;
}

// TEMPORARY CODE -- WILL BE REMOVE IN FUTURE
// setup relja_retrival backend and frontend (temporary, until JS based frontend is ready)
extern void api_v2(std::vector< std::string > argv);
void ViseServer::InitReljaRetrival() {
  boost::thread backend( boost::bind( &ViseServer::InitReljaRetrivalBackend, this ) );
  //boost::thread frontend( boost::bind( &ViseServer::InitReljaRetrivalFrontend, this ) );

  backend.join();
  //frontend.join();
}

// Note: frontend is invoked by src/api/abs_api.cpp::InitReljaRetrivalFrontend()
void ViseServer::InitReljaRetrivalBackend() {
  // start relja_retrival backend
  std::vector< std::string > param;
  param.push_back("api_v2");
  param.push_back("65521");
  param.push_back( search_engine_.GetName() );
  param.push_back( search_engine_.GetEngineConfigFn().string() );
  param.push_back( vise_source_code_dir_.string() );
  api_v2( param );
}
