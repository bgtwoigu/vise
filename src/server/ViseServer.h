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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <cassert>                // for assert()
#include <math.h>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/timer/timer.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/filesystem.hpp>

#include <Magick++.h>            // to transform images

#include "SearchEngine.h"
#include "ViseMessageQueue.h"

// search engine query
#include <fastann.hpp>

#include "retriever.h"
#include "spatial_retriever.h"
#include "dataset_abs.h"
#include "query.h"
#include "multi_query.h"

#include "clst_centres.h"
#include "dataset_v2.h"
#include "feat_getter.h"
#include "feat_standard.h"
#include "hamming.h"
#include "hamming_embedder.h"
#include "index_entry.pb.h"
#include "macros.h"
#include "mq_filter_outliers.h"
#include "par_queue.h"
#include "proto_db.h"
#include "proto_db_file.h"
#include "proto_index.h"
#include "slow_construction.h"
#include "soft_assigner.h"
#include "spatial_verif_v2.h"
#include "tfidf_data.pb.h"
#include "tfidf_v2.h"
#include "util.h"

using boost::asio::ip::tcp;

class ViseServer {
 private:
  SearchEngine search_engine_;
  unsigned int port_;
  std::string hostname_;
  std::string url_prefix_;
  std::string msg_url_;

  int state_id_;
  std::vector< int > state_id_list_;
  std::vector< std::string > state_name_list_;
  std::vector< std::string > state_info_list_;
  std::vector< std::vector<double> > state_complexity_model_;
  std::vector< std::string > state_html_fn_list_;
  std::vector< std::string > state_html_list_;

  // to terminate the VISE server
  bool vise_shutdown_flag_;

  // threads
  boost::thread *vise_training_thread_;
  boost::thread *vise_load_search_index_thread_;

  // html content
  std::string vise_main_html_;
  std::string vise_help_html_;
  std::string vise_404_html_;
  std::string vise_css_;
  std::string vise_js_;

  std::string vise_index_html_;
  bool vise_index_html_reload_;

  // boost asio based server
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::acceptor *vise_acceptor_;

  // html content location
  boost::filesystem::path vise_css_fn_;
  boost::filesystem::path vise_js_fn_;
  boost::filesystem::path vise_main_html_fn_;
  boost::filesystem::path vise_help_html_fn_;
  boost::filesystem::path vise_404_html_fn_;

  // resource dir
  boost::filesystem::path vise_datadir_;
  boost::filesystem::path vise_source_code_dir_;
  boost::filesystem::path user_home_dir_;
  boost::filesystem::path vise_templatedir_;
  boost::filesystem::path vise_logdir_;
  boost::filesystem::path vise_enginedir_;

  // state complexity
  void LoadStateComplexityModel();
  std::string GetStateComplexityInfo();
  void UpdateStateInfoList();
  std::vector<double> total_complexity_model_;
  std::string state_complexity_info_;
  std::string complexity_model_assumption_;

  // search engine query
  spatialVerifV2 *spatial_verif_v2_;
  spatialRetriever *spatial_retriever_;
  multiQuery *multi_query_;
  multiQueryMax *multi_query_max_;
  datasetAbs *dataset_;
  sequentialConstructions *cons_queue_;
  hamming *hamming_emb_;
  embedderFactory *emb_factory_;
  clstCentres *clst_centres_;
  fastann::nn_obj<float> *nn_;
  featGetter *feat_getter_;
  softAssigner *soft_assigner_;
  retrieverFromIter *base_retriever_;
  protoDbFile *dbFidx_file_;
  protoDbFile *dbIidx_file_;
  protoDbInRamStartDisk *dbFidx_;
  protoDbInRamStartDisk *dbIidx_;
  protoIndex *fidx_;
  protoIndex *iidx_;
  tfidfV2 *tfidf_;
  bool useHamm;

  boost::system::error_code error_;

  // state maintainanace
  bool UpdateState();
  void ResetToInitialState();
  std::string GetStateJsonData();
  void GenerateViseIndexHtml();
  void ServeStaticResource(const std::string resource_name,
                           const std::map< std::string, std::string> &resource_args,
                           boost::shared_ptr<tcp::socket> p_socket);
  void LoadSearchEngine(std::string search_engine_name);

  // search engine training
  void InitiateSearchEngineTraining();

  // search engine query
  void HandleQueryGetRequest(std::string http_method_uri, boost::shared_ptr<tcp::socket> p_socket);
  void QueryServeImgList( unsigned int page_no,
                          unsigned int per_page_im_count,
                          boost::shared_ptr<tcp::socket> p_socket );
  void QuerySearchImageRegion(std::string query_img_fn,
                              unsigned int x0,
                              unsigned int y0,
                              unsigned int x1,
                              unsigned int y1,
                              boost::shared_ptr<tcp::socket> p_socket);
  void QueryCompareImage(std::string im1fn,
                         std::string im2fn,
                         std::string x0y0x1y1_str,
                         std::string H_str,
                         boost::shared_ptr<tcp::socket> p_socket);

  void QueryTransformImage(Magick::Image &im, const std::map< std::string, std::string > &resource_args);
  void SendStaticImageResponse(boost::filesystem::path im_fn, boost::shared_ptr<tcp::socket> p_socket);
  void SendImageResponse(Magick::Image &im, std::string content_type, boost::shared_ptr<tcp::socket> p_socket);

  void QueryInit();
  void QueryLoadSearchIndex();

  void QueryTest();
  void HomographyTransform( double H[],
                            double   x0, double   y0, double   x1, double   y1,
                            unsigned int &tx0, unsigned int &ty0, unsigned int &tx1, unsigned int &ty1 );
  void HomographyPointTransform( double H[], const double x, const double y, double &xt, double &yt );

  // HTTP connection handler
  void HandleConnection(boost::shared_ptr<tcp::socket> p_socket);
  void HandleStatePostData( int state_id, std::string http_post_data, boost::shared_ptr<tcp::socket> p_socket );
  void HandleStateGetRequest( std::string resource_name,
                              std::map< std::string, std::string> resource_args,
                              boost::shared_ptr<tcp::socket> p_socket);
  void HandleDiredGetRequest(const std::string& dired_uri, boost::shared_ptr<tcp::socket> p_socket);

  void SendHttpResponse(std::string html, boost::shared_ptr<tcp::socket> p_socket);
  void SendHttpPostResponse(std::string http_post_data, std::string result, boost::shared_ptr<tcp::socket> p_socket);
  void SendHttp404NotFound(boost::shared_ptr<tcp::socket> p_socket);
  void SendHttp400BadRequest(boost::shared_ptr<tcp::socket> p_socket);
  void SendHttpRedirect(std::string redirect_uri, boost::shared_ptr<tcp::socket> p_socket);
  void SendErrorResponse(std::string message, std::string backtrace, boost::shared_ptr<tcp::socket> p_socket);
  void SendRawResponse(std::string content_type, std::string content, boost::shared_ptr<tcp::socket> p_socket);
  void SendJsonResponse(std::string json, boost::shared_ptr<tcp::socket> p_socket);

  void SendMessage(std::string message);
  void SendLog(std::string log);
  void SendCommand(std::string command);
  void SendProgress(std::string state_name, unsigned long completed, unsigned long total);
  void SendPacket(std::string type, std::string message);

  // helper functions
  void ExtractHttpResource(std::string http_request, std::string &http_resource);
  void ExtractHttpContent(std::string http_request, std::string &http_content);
  void ParseHttpMethodUri(const std::string http_method_uri,
                          std::string &resource_name,
                          std::map< std::string, std::string > &resource_args);
  int  LoadFile(const std::string filename, std::string &file_contents);
  void WriteFile(std::string filename, std::string &file_contents);
  void SplitString(const std::string s, char sep, std::vector<std::string> &tokens);
  bool ReplaceString(std::string &s, std::string old_str, std::string new_str);
  bool StringStartsWith( const std::string &s, const std::string &prefix );
  std::string GetHttpContentType(boost::filesystem::path fn);
  void UnescapeHttpString( std::string &s );

  // TEMPORARY -- WILL BE REMOVED IN FUTURE
  // setup relja_retrival backend and frontend (temporary, until JS based frontend is ready)
  void InitReljaRetrival();
  void InitReljaRetrivalBackend();

  // @todo: move to ViseServer.cc ( do not know how! )
  template<typename T> void ParseCsvString( const std::string csv, std::vector< T > &d ) {
    d.clear();

    std::istringstream s( csv );
    char comma;
    T value;
    while ( s >> value ) {
      d.push_back( value );
      s >> comma;
    }
  }

  // for logging statistics
  boost::filesystem::path vise_training_stat_fn_;

  std::ofstream training_stat_f;
  void AddTrainingStat(std::string dataset_name, std::string state_name, unsigned long time_sec, unsigned long space_bytes);

 public:
  static const int STATE_NOT_LOADED =  0;
  static const int STATE_SETTING    =  1;
  static const int STATE_INFO       =  2;
  static const int STATE_PREPROCESS =  3;
  static const int STATE_DESCRIPTOR =  4;
  static const int STATE_CLUSTER    =  5;
  static const int STATE_ASSIGN     =  6;
  static const int STATE_HAMM       =  7;
  static const int STATE_INDEX      =  8;
  static const int STATE_QUERY      =  9;

  ViseServer( boost::filesystem::path vise_datadir, boost::filesystem::path vise_src_code_dir, boost::filesystem::path user_home_dir );
  ~ViseServer();

  std::string GetCurrentStateName();
  std::string GetCurrentStateInfo();
  std::string GetStateName( int state_id );
  std::string GetStateInfo( int state_id );
  int GetStateId( std::string );
  int GetCurrentStateId();

  //void InitResources( std::string vise_datadir, std::string vise_templatedir );
  void Start(unsigned int port);
  bool Stop();
  bool Restart();

  bool SearchEngineExists( std::string search_engine_name );
};

#endif /* _VISE_SERVER_H */
