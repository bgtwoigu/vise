#include "ViseServer.h"


ViseServer::ViseServer( const std::string address, const std::string port, std::size_t thread_pool_size, boost::filesystem::path vise_src_code_dir, boost::filesystem::path vise_data_dir ) : acceptor_( io_service_ ), signals_(io_service_) {

  address_          = address;
  port_             = port;
  thread_pool_size_ = thread_pool_size;

  // set resource names
  vise_datadir_         = boost::filesystem::path(vise_data_dir);
  vise_src_code_dir_    = boost::filesystem::path(vise_src_code_dir);
  vise_resourcedir_     = vise_src_code_dir_ / "src/server/resources/";

  if ( ! boost::filesystem::exists( vise_datadir_ ) ) {
    std::cerr << "\nViseServer::ViseServer() : vise_datadir_ does not exist! : "
              << vise_datadir_.string() << std::flush;
    return;
  }
  if ( ! boost::filesystem::exists( vise_resourcedir_ ) ) {
    std::cerr << "\nViseServer::ViseServer() : vise_resourcedir_ does not exist! : "
              << vise_resourcedir_.string() << std::flush;
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

  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
  signals_.async_wait(boost::bind(&ViseServer::Stop, this));

  boost::asio::ip::tcp::resolver resolver( io_service_ );
  boost::asio::ip::tcp::resolver::query query( address_, port_ );
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve( query );

  acceptor_.open( endpoint.protocol() );
  acceptor_.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true) );
  acceptor_.bind( endpoint );
  acceptor_.listen();

  // search engine
  search_engine_ = new SearchEngine();

  // server resources
  resources_ = new Resources();
  resources_->LoadAllResources(vise_resourcedir_);

  server_connection_count_ = 0;
  AcceptNewConnection();
  std::cout << "\nserver waiting for connections at " << address_ << ":" << port_ << std::flush;
  std::cout << "\n[Press Ctrl + C to stop the server]\n" << std::flush;

}

ViseServer::~ViseServer() {
  std::cout << "\nServed " << server_connection_count_ << " HTTP requests using " 
            << thread_pool_size_ << " threads. It is now time to say bye :-)\n" << std::flush;
}

void ViseServer::Start() {
  std::vector< boost::shared_ptr<boost::thread> > threads;
  for ( std::size_t i = 0; i < thread_pool_size_; ++i ) {
    // these threads will block until async operations are complete
    // the first async operation is created by a call to AcceptNewConnection() in ImcompServer::ImcompServer()
    boost::shared_ptr<boost::thread> connection_thread( new boost::thread( boost::bind( &boost::asio::io_service::run, &io_service_) ) );
    threads.push_back( connection_thread );
  }

  for ( std::size_t i = 0; i < threads.size(); ++i ) {
    threads[i]->join();
  }
}

void ViseServer::AcceptNewConnection() {
  new_connection_.reset( new Connection(io_service_, search_engine_, resources_, vise_datadir_, vise_src_code_dir_) );
  acceptor_.async_accept( new_connection_->Socket(),
                          boost::bind(&ViseServer::HandleConnection, this, boost::asio::placeholders::error) );
}

void ViseServer::HandleConnection(const boost::system::error_code& e) {
  if ( !e ) {
    new_connection_->ProcessConnection();
    server_connection_count_++;
  }
  AcceptNewConnection();
}

void ViseServer::Stop() {
  std::cout << "\n\nServer shutting down ..." << std::flush;
  io_service_.stop();
}

