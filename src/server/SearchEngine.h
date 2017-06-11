/** @file   search_engine.h
 *  @brief  allow search of an image set
 *
 *  Given a set of images, this module lets initialize, configure and train a
 *  model that allows image region based queries on this image set
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   31 March 2017
 */

#ifndef _VISE_SEARCH_ENGINE_H
#define _VISE_SEARCH_ENGINE_H

#include <cstdio>                 // for popen()

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <map>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <locale>                // for std::tolower
#include <cassert>               // for assert()

#include <boost/filesystem.hpp>  // to query/update filesystem

#include <Magick++.h>            // to transform images

#include "ViseMessageQueue.h"
#include "ViseUtils.h"
#include "Resources.h"

#include "feat_standard.h"
#include "train_descs.h"
#include "train_assign.h"
#include "train_hamming.h"
#include "build_index.h"
#include "hamming_embedder.h"

class SearchEngine {
public:

  SearchEngine(Resources* resources, boost::filesystem::path engine_dir, boost::filesystem::path vise_src_code_dir);
  ~SearchEngine();
  void Init(std::string name);

  // search engine possible states
  static const int STATE_NOT_LOADED;
  static const int STATE_SETTING;
  static const int STATE_INFO;
  static const int STATE_PREPROCESS;
  static const int STATE_DESCRIPTOR;
  static const int STATE_CLUSTER;
  static const int STATE_ASSIGN;
  static const int STATE_HAMM ;
  static const int STATE_INDEX;
  static const int STATE_QUERY;

  // query search engine state
  std::string GetCurrentStateName() const;
  std::string GetCurrentStateInfo() const;
  std::string GetStateName( int state_id ) const;
  std::string GetStateInfo( int state_id ) const;
  std::string GetStateHtmlFn( int state_id ) const;
  int GetStateId( std::string ) const;
  int GetCurrentStateId() const;
  bool UpdateState();
  std::string& GetStateJsonData();
  std::string GetStateComplexityInfo();
  void UpdateStateInfoList();

  void Preprocess();
  void Descriptor();
  void Cluster();
  void Assign();
  void Hamm();
  void Index();
  void Query();

  // helper functions to query state of search engine
  std::string GetName();
  bool IsEngineConfigEmpty();
  bool IsImglistEmpty();
  bool EngineConfigFnExists();
  bool ImglistFnExists();
  bool DescFnExists();
  bool ClstFnExists();
  bool AssignFnExists();
  bool HammFnExists();
  bool IndexFnExists();
  std::string GetEngineOverview();
  unsigned long GetImglistOriginalSize();
  unsigned long GetImglistTransformedSize();
  unsigned long DescFnSize();
  unsigned long ClstFnSize();
  unsigned long AssignFnSize();
  unsigned long HammFnSize();
  unsigned long IndexFnSize();
  unsigned long GetImglistSize();

  boost::filesystem::path GetEngineConfigFn();
  std::string GetResourceUri(std::string resource_name);
  std::string GetSearchEngineBaseUri();
  boost::filesystem::path GetOriginalImageDir();
  boost::filesystem::path GetTransformedImageDir();

  void SetEngineConfig(std::string engine_config);
  void SetEngineConfigParam(std::string key, std::string value);
  std::string GetEngineConfigParam(std::string key);
  bool EngineConfigParamExists(std::string key);
  void PrintEngineConfig();

  void LoadImglist();
  void CreateFileList();
  void WriteConfigToFile();
  void ReadConfigFromFile();
  std::string GetImglistFn( unsigned int index );

  // static methods to help with search engine management
  static bool ValidateName(std::string engine_name);
  bool Exists( const std::string search_engine_name );
  void GetSearchEngineList( std::vector< std::string >& engine_list ) const;
  bool Delete( const std::string search_engine_name );

  // search engine training
  void StartTraining();
  void StopTraining();
  void QueryInit();
  void Load(std::string search_engine_name);

 private:
  std::string engine_name_;
  Resources* resources_;
  boost::system::error_code error_;

  // threads
  boost::thread* training_thread_;
  boost::thread* rr_thread_;

  // state variables
  int state_id_;
  std::vector< int > state_id_list_;
  std::vector< std::string > state_name_list_;
  std::vector< std::string > state_info_list_;
  std::vector< std::vector<double> > state_complexity_model_;
  std::vector< std::string > state_html_fn_list_;
  std::vector<double> total_complexity_model_;
  std::string state_complexity_info_;
  std::string complexity_model_assumption_;
  std::string state_json_;

  void LoadStateResources();
  void LoadStateComplexityModel();

  boost::filesystem::path basedir_;
  boost::filesystem::path enginedir_;
  boost::filesystem::path vise_src_code_dir_;

  boost::filesystem::path original_imgdir_;
  boost::filesystem::path transformed_imgdir_;
  boost::filesystem::path imglist_fn_;
  std::vector< std::string > imglist_;
  std::vector< unsigned int > imglist_fn_original_size_;
  std::vector< unsigned int > imglist_fn_transformed_size_;

  boost::filesystem::path training_datadir_;
  boost::filesystem::path tmp_datadir_;

  std::map< std::string, std::string > engine_config_;
  boost::filesystem::path engine_config_fn_;

  std::set< std::string > acceptable_img_ext_;

  void CreateEngine( std::string name );
  void LoadEngine( std::string name );
  bool EngineConfigExists();

  void SendProgress(std::string state_name, unsigned long completed, unsigned long total);
  void SendProgressMessage(std::string state_name, std::string msg);
  void SendCommand(std::string sender, std::string command);
  void SendLog(std::string sender, std::string log);
  void SendMessage(std::string message);
  void SendPacket(std::string sender, std::string type, std::string messsage);

  void WriteImageListToFile(const std::string fn,
                            const std::vector< std::string > &imlist);

  void Train();
  void InitEngineResources( std::string name );
  void RunClusterCommand();
  void InitReljaRetrivalBackend();
  void StartReljaRetrivalBackendFrontend();
  void StopReljaRetrivalBackendFrontend();
  void StopAllOperations();
};

#endif /* _VISE_SEARCH_ENGINE_H */
