#include "SearchEngine.h"

// search engine possible states
const int SearchEngine::STATE_NOT_LOADED =  0;
const int SearchEngine::STATE_SETTING    =  1;
const int SearchEngine::STATE_INFO       =  2;
const int SearchEngine::STATE_PREPROCESS =  3;
const int SearchEngine::STATE_DESCRIPTOR =  4;
const int SearchEngine::STATE_CLUSTER    =  5;
const int SearchEngine::STATE_ASSIGN     =  6;
const int SearchEngine::STATE_HAMM       =  7;
const int SearchEngine::STATE_INDEX      =  8;
const int SearchEngine::STATE_QUERY      =  9;

SearchEngine::SearchEngine(Resources* resources, boost::filesystem::path engine_dir, boost::filesystem::path vise_src_code_dir) {
  resources_ = resources;
  basedir_   = engine_dir;
  vise_src_code_dir_ = vise_src_code_dir;

  state_id_ = SearchEngine::STATE_NOT_LOADED;
  engine_name_ = "";
  engine_config_.clear();

  acceptable_img_ext_.insert( ".jpg" );
  acceptable_img_ext_.insert( ".jpeg" );
  acceptable_img_ext_.insert( ".png" );
  acceptable_img_ext_.insert( ".pgm" );
  acceptable_img_ext_.insert( ".pnm" );
  acceptable_img_ext_.insert( ".ppm" );
}

SearchEngine::~SearchEngine() {
  std::cout << "\nStopping search engine " << GetName() << std::flush;
  StopTraining();
}

void SearchEngine::Init(std::string name) {
  engine_name_ = name;
  engine_config_.clear();
  state_id_ = SearchEngine::STATE_NOT_LOADED;

  boost::filesystem::path engine_name( engine_name_ );
  enginedir_ = basedir_ / engine_name;

  engine_config_fn_    = enginedir_ / "user_settings.txt";
  transformed_imgdir_  = enginedir_ / "img";
  training_datadir_    = enginedir_ / "training_data";
  tmp_datadir_         = enginedir_ / "tmp";

  imglist_fn_ = training_datadir_ / "imlist.txt";
  engine_config_fn_ = training_datadir_ / "vise_config.cfg";

  transformed_imgdir_ += boost::filesystem::path::preferred_separator;
  training_datadir_ += boost::filesystem::path::preferred_separator;
  tmp_datadir_ += boost::filesystem::path::preferred_separator;

  // create directory
  if ( ! boost::filesystem::exists( enginedir_ ) ) {
    boost::filesystem::create_directory( enginedir_ );
    boost::filesystem::create_directory( transformed_imgdir_ );
    boost::filesystem::create_directory( training_datadir_ );
    boost::filesystem::create_directory( tmp_datadir_ );
  } else {
    if ( ! boost::filesystem::exists( transformed_imgdir_ ) ) {
      boost::filesystem::create_directory( transformed_imgdir_ );
    }
    if ( ! boost::filesystem::exists( training_datadir_ ) ) {
      boost::filesystem::create_directory( training_datadir_ );
    }
    if ( ! boost::filesystem::exists( tmp_datadir_ ) ) {
      boost::filesystem::create_directory( tmp_datadir_ );
    }
  }

  // load state name, description, complexity model, html files, etc
  LoadStateResources();
  LoadStateComplexityModel();
}

//
// Workers for each state
//
void SearchEngine::Preprocess() {
  if ( imglist_.empty() ) {
    CreateFileList();
  }

  if ( ! boost::filesystem::exists( imglist_fn_ ) ) {
    SendLog("Preprocess", "\nPreprocessing started ...");
    SendCommand("Preprocess", "_progress reset show");

    std::string transformed_img_width = GetEngineConfigParam("transformed_img_width");
    if (transformed_img_width != "original") {
      // scale and copy image to transformed_imgdir_
      SendLog("Preprocess", "\nSaving transformed images to [" + transformed_imgdir_.string() + "] ");
    } else {
      SendLog("Preprocess", "\nCopying original images to [" + transformed_imgdir_.string() + "] ");
    }
    for ( unsigned int i=0; i<imglist_.size(); i++ ) {
      boost::filesystem::path img_rel_path = imglist_.at(i);
      boost::filesystem::path src_fn  = original_imgdir_ / img_rel_path;
      boost::filesystem::path dest_fn = transformed_imgdir_ / img_rel_path;

      //std::cout << "\n\t" << src_fn.string() << " => " << dest_fn.string();
      if ( !boost::filesystem::exists( dest_fn ) ) {
        try {
          // check if image path exists
          if ( ! boost::filesystem::is_directory( dest_fn.parent_path() ) ) {
            boost::filesystem::create_directories( dest_fn.parent_path() );
          }

          if (transformed_img_width != "original") {
            try {
              //std::cout << "\n  Processing image :  " << src_fn.string() << std::flush;
              Magick::Image im;
              im.read( src_fn.string() );
              Magick::Geometry size = im.size();

              double aspect_ratio =  ((double) size.height()) / ((double) size.width());

              std::stringstream s;
              s << transformed_img_width;
              unsigned int new_width;
              s >> new_width;
              unsigned int new_height = (unsigned int) (new_width * aspect_ratio);

              Magick::Geometry resize = Magick::Geometry(new_width, new_height);
              im.zoom( resize );

              im.write( dest_fn.string() );
              imglist_fn_transformed_size_.at(i) = boost::filesystem::file_size(dest_fn.string().c_str());

              // to avoid overflow of the message queue
              if ( (i % 5) == 0 ) {
                SendProgress( "Preprocess", i+1, imglist_.size() );
              }
            } catch( std::exception& e ) {
              SendLog("Preprocess", "\nfailed to transform image " + src_fn.string() + " : Error [" + e.what() + "]" );
              std::cout << "\nSearchEngine::Preprocess() : failed to transform image " << src_fn.string() << std::endl;
              std::cout << e.what() << std::flush;
              return;
            }
          } else {
            // just copy the files
            boost::filesystem::copy_file( src_fn, dest_fn );
            imglist_fn_transformed_size_.at(i) = imglist_fn_original_size_.at(i);

            // to avoid overflow of the message queue
            if ( (i % 50) == 0 ) {
              SendProgress( "Preprocess", i+1, imglist_.size() );
            }
          }
          if ( (i % 50) == 0 ) {
            SendLog("Preprocess", ".");
          }
        } catch (std::exception &error) {
          SendLog("Preprocess", "\n" + src_fn.string() + " : Error [" + error.what() + "]" );
        }
      }
    }
    if (transformed_img_width != "original") {
      SendProgress( "Preprocess", imglist_.size(), imglist_.size() );
    }

    SendLog("Preprocess", "[Done]");

    //if ( ! boost::filesystem::exists( imglist_fn_ ) ) {
    WriteImageListToFile( imglist_fn_.string(), imglist_ );
    SendLog("Preprocess", "\nWritten image list to : [" + imglist_fn_.string() + "]" );
  }
}

void SearchEngine::Descriptor() {
  std::string const trainDescsFn  = GetEngineConfigParam("descFn");
  boost::filesystem::path train_desc_fn( trainDescsFn );

  if ( ! boost::filesystem::exists( train_desc_fn ) ) {
    SendLog("Descriptor", "\nComputing training descriptors ...");
    SendCommand("Descriptor", "_progress reset show");
    SendProgressMessage("Descriptor", "Starting to compute image descriptors");

    std::string const trainImagelistFn = GetEngineConfigParam("trainImagelistFn");
    std::string const trainDatabasePath = GetEngineConfigParam("trainDatabasePath");

    int32_t trainNumDescs;
    /*
    std::stringstream s;
    s << GetEngineConfigParam("trainNumDescs");
    s >> trainNumDescs;
    */

    // @todo : find a better place to define these constants
    unsigned int DESCRIPTORS_PER_IMAGE = 1000;
    double VOCABULARY_SIZE_FACTOR = 10; // voc. size = no. of descriptors / 10

    trainNumDescs = imglist_.size() * DESCRIPTORS_PER_IMAGE; // we use all the images

    std::ostringstream s;
    unsigned int vocSize = ((double)trainNumDescs) / VOCABULARY_SIZE_FACTOR;
    s << vocSize;
    SetEngineConfigParam( "vocSize", s.str());
    WriteConfigToFile();
    std::cout << "\nSearchEngine::Descriptor() : trainNumDescs = " << trainNumDescs << std::flush;
    std::cout << "\nSearchEngine::Descriptor() : vocSize = " << vocSize << std::flush;

    bool SIFTscale3 = false;
    if ( GetEngineConfigParam("SIFTscale3") == "on" ) {
      SIFTscale3 = true;
    }

    // source: src/v2/indexing/compute_index_v2.cpp
    featGetter_standard const featGetter_obj( (
                                               std::string("hesaff-") +
                                               "sift" +
                                               std::string(SIFTscale3 ? "-scale3" : "")
                                               ).c_str() );

    buildIndex::computeTrainDescs(trainImagelistFn, trainDatabasePath,
                                  trainDescsFn,
                                  trainNumDescs,
                                  featGetter_obj);
  }
  SendLog("Descriptor", "Completed computing descriptors");
  SendCommand("Cluster", "_progress reset hide");
}

// ensure that you install the dkmeans_relja as follows
// $ cd src/external/dkmeans_relja/
// $ python setup.py build
// $ sudo python setup.py install
void SearchEngine::Cluster() {
  if ( ! ClstFnExists() ) {
    SendLog("Cluster", "\nStarting clustering of descriptors ...");
    SendCommand("Cluster", "_progress reset show");
    SendProgressMessage("Descriptor", "Starting clustering of descriptors");

    //boost::thread t( boost::bind( &SearchEngine::RunClusterCommand, this ) );
    RunClusterCommand();
  } else {
    //SendLog("Cluster", "\nLoaded");
  }
}

void SearchEngine::RunClusterCommand() {
  // @todo: avoid relative path and discover the file "compute_clusters.py" automatically
  std::string exec_name = vise_src_code_dir_.string() + "/src/v2/indexing/compute_clusters.py";
  std::string cmd = "python";
  cmd += " " + exec_name;
  cmd += " " + engine_name_;
  cmd += " " + engine_config_fn_.string();

  FILE *pipe = popen( cmd.c_str(), "r");

  if ( pipe != NULL ) {
    SendLog("Cluster", "\nCommand executed: $" + cmd);

    char line[128];
    /*
    while ( fgets(line, 128, pipe) ) {
    */
    while ( !feof( pipe ) ) {
      if ( fgets(line, 128, pipe) != NULL ) {
        std::string status_txt(line);
        SendLog("Cluster", status_txt);

        std::cout << "\nSearchEngine::Cluster() : " << status_txt << std::flush;
        std::string itr_prefix = "Iteration ";
        if ( status_txt.substr(0, itr_prefix.length()) == itr_prefix) { // starts with
          unsigned int second_spc = status_txt.find(' ', itr_prefix.length() );
          unsigned int itr_str_len = second_spc - itr_prefix.length();
          std::string itr_str = status_txt.substr(itr_prefix.length(), itr_str_len);
          if ( itr_str.find('/') != std::string::npos ) {
            unsigned long completed, total;
            char slash;
            std::istringstream s ( itr_str );
            s >> completed >> slash >> total;
            SendProgress("Cluster", completed, total);
            //std::cout << "\nSending progress " << completed << " of " << total << std::flush;
          }
        }
      }
    }
    pclose( pipe );
    SendCommand("Cluster", "_progress reset hide");
  } else {
    //SendLog("Cluster", "\nFailed to execute python script for clustering: \n\t $" + cmd);
  }
}

void SearchEngine::Assign() {
  if ( ! AssignFnExists() ) {
    SendLog("Assign", "\nStarting assignment ...");
    SendCommand("Cluster", "_progress reset hide");
    SendProgressMessage("Descriptor", "Starting assignment");

    bool useRootSIFT = false;
    if ( GetEngineConfigParam("RootSIFT") == "on" ) {
      useRootSIFT = true;
    }

    buildIndex::computeTrainAssigns( GetEngineConfigParam("clstFn"),
                                     useRootSIFT,
                                     GetEngineConfigParam("descFn"),
                                     GetEngineConfigParam("assignFn"));
  } else {
    //SendLog("Assign", "\nLoaded");
  }
}

void SearchEngine::Hamm() {
  if ( ! HammFnExists() ) {
    SendLog("Hamm", "\nComputing hamm ...");
    SendCommand("Cluster", "_progress reset hide");
    SendProgressMessage("Descriptor", "Starting to compute hamm embeddings");

    uint32_t hammEmbBits;
    std::string hamm_emb_bits = GetEngineConfigParam("hammEmbBits");
    std::istringstream s(hamm_emb_bits);
    s >> hammEmbBits;

    bool useRootSIFT = false;
    if ( GetEngineConfigParam("RootSIFT") == "on" ) {
      useRootSIFT = true;
    }

    buildIndex::computeHamming(GetEngineConfigParam("clstFn"),
                               useRootSIFT,
                               GetEngineConfigParam("descFn"),
                               GetEngineConfigParam("assignFn"),
                               GetEngineConfigParam("hammFn"),
                               hammEmbBits);
  } else {
    //SendLog("Hamm", "\nLoaded");
  }
}

void SearchEngine::Index() {
  if ( ! IndexFnExists() ) {
    SendLog("Index", "\nStarting indexing ...");
    SendCommand("Cluster", "_progress reset show");
    SendProgressMessage("Descriptor", "Starting image indexing");

    bool SIFTscale3 = false;
    if ( GetEngineConfigParam("SIFTscale3") == "on" ) {
      SIFTscale3 = true;
    }

    bool useRootSIFT = false;
    if ( GetEngineConfigParam("RootSIFT") == "on" ) {
      useRootSIFT = true;
    }

    // source: src/v2/indexing/compute_index_v2.cpp
    std::ostringstream feat_getter_config;
    feat_getter_config << "hesaff-";
    if (useRootSIFT) {
      feat_getter_config << "rootsift";
    } else {
      feat_getter_config << "sift";
    }
    if (SIFTscale3) {
      feat_getter_config << "-scale3";
    }
    featGetter_standard const featGetter_obj( feat_getter_config.str().c_str() );

    // embedder
    uint32_t hammEmbBits;
    std::string hamm_emb_bits = GetEngineConfigParam("hammEmbBits");
    std::istringstream s(hamm_emb_bits);
    s >> hammEmbBits;

    embedderFactory *embFactory= NULL;
    if ( EngineConfigParamExists("hammEmbBits") ) {
      embFactory= new hammingEmbedderFactory(GetEngineConfigParam("hammFn"), hammEmbBits);
    } else {
      embFactory= new noEmbedderFactory;
    }

    buildIndex::build(GetEngineConfigParam("imagelistFn"),
                      GetEngineConfigParam("databasePath"),
                      GetEngineConfigParam("dsetFn"),
                      GetEngineConfigParam("iidxFn"),
                      GetEngineConfigParam("fidxFn"),
                      GetEngineConfigParam("tmpDir"),
                      featGetter_obj,
                      GetEngineConfigParam("clstFn"),
                      embFactory);

    delete embFactory;
  } else {
    //SendLog("Index", "\nLoaded");
  }
}

//
// Helper functions
//
bool SearchEngine::EngineConfigExists() {
  if ( is_regular_file(engine_config_fn_) ) {
    return true;
  } else {
    return false;
  }
}

void SearchEngine::CreateFileList() {

  boost::filesystem::path dir(GetEngineConfigParam("imagePath"));

  //std::cout << "\nShowing directory contents of : " << dir.string() << std::endl;
  imglist_.clear();
  imglist_fn_original_size_.clear();
  imglist_fn_transformed_size_.clear();
  boost::filesystem::recursive_directory_iterator dir_it( dir ), end_it;

  std::string basedir = dir.string();
  std::locale locale;
  while ( dir_it != end_it ) {
    boost::filesystem::path p = dir_it->path();
    std::string fn_dir = p.parent_path().string();
    std::string rel_path = fn_dir.replace(0, basedir.length(), "");
    boost::filesystem::path rel_fn = boost::filesystem::path(rel_path) / p.filename();
    if ( boost::filesystem::is_regular_file(p) ) {
      // add only image files which can be read by VISE
      std::string fn_ext = p.extension().string();

      // convert fn_ext to lower case
      for ( unsigned int i=0; i<fn_ext.length(); i++) {
        std::tolower( fn_ext.at(i), locale );
      }

      if ( acceptable_img_ext_.count( fn_ext ) == 1 ) {
        imglist_.push_back( rel_fn.string() );
        imglist_fn_original_size_.push_back( boost::filesystem::file_size( p ) );
        imglist_fn_transformed_size_.push_back( 0 );
      }
    }
    ++dir_it;
  }
  original_imgdir_ = dir;
}

// data_str =
// key1=value1
// key2=value2
// ...
void SearchEngine::SetEngineConfig(std::string engine_config) {
  std::istringstream d(engine_config);

  std::string keyval;
  while( std::getline(d, keyval, '\n') ) {
    std::size_t pos_eq = keyval.find('=');
    if ( pos_eq != std::string::npos ) {
      std::string key = keyval.substr(0, pos_eq);
      std::string val = keyval.substr(pos_eq + 1);
      engine_config_.insert( std::pair<std::string, std::string>(key, val) );
    }
  }

  // save full configuration file to training_datadir_
  SetEngineConfigParam("trainDatabasePath", transformed_imgdir_.string());
  SetEngineConfigParam("databasePath", transformed_imgdir_.string());
  SetEngineConfigParam("trainImagelistFn",  imglist_fn_.string());
  SetEngineConfigParam("imagelistFn",  imglist_fn_.string());
  boost::filesystem::path train_file_prefix = training_datadir_ / "train_";
  SetEngineConfigParam("trainFilesPrefix", train_file_prefix.string() );
  SetEngineConfigParam("pathManHide", transformed_imgdir_.string() );
  SetEngineConfigParam("descFn", train_file_prefix.string() + "descs.e3bin" );
  SetEngineConfigParam("assignFn", train_file_prefix.string() + "assign.bin" );
  SetEngineConfigParam("hammFn", train_file_prefix.string() + "hamm.v2bin" );
  SetEngineConfigParam("dsetFn", train_file_prefix.string() + "dset.v2bin" );
  SetEngineConfigParam("clstFn", train_file_prefix.string() + "clst.e3bin" );
  SetEngineConfigParam("iidxFn", train_file_prefix.string() + "iidx.e3bin" );
  SetEngineConfigParam("fidxFn", train_file_prefix.string() + "fidx.e3bin" );
  SetEngineConfigParam("wghtFn", train_file_prefix.string() + "wght.e3bin" );
  SetEngineConfigParam("tmpDir", tmp_datadir_.string());
}

std::string SearchEngine::GetEngineConfigParam(std::string key) {
  std::map<std::string, std::string>::iterator it;
  it = engine_config_.find( key );

  if ( it != engine_config_.end() ) {
    return it->second;
  }
  else {
    return "";
  }
}

bool SearchEngine::EngineConfigParamExists(std::string key) {
  std::map<std::string, std::string>::iterator it;
  it = engine_config_.find( key );

  if ( it != engine_config_.end() ) {
    return true;
  }
  else {
    return false;
  }
}

void SearchEngine::SetEngineConfigParam(std::string key, std::string value) {
  std::map<std::string, std::string>::iterator it;
  it = engine_config_.find( key );

  if ( it != engine_config_.end() ) {
    it->second = value;
  }
  else {
    engine_config_.insert( std::pair<std::string, std::string>(key, value) );
  }
}

boost::filesystem::path SearchEngine::GetEngineConfigFn() {
  return engine_config_fn_;
}
std::string SearchEngine::GetSearchEngineBaseUri() {
  return "/" + engine_name_ + "/";
}

void SearchEngine::WriteImageListToFile(const std::string fn,
                                        const std::vector< std::string > &imlist) {
  try {
    std::ofstream f( fn.c_str() );
    for ( unsigned int i=0; i < imlist.size(); i++) {
      f << imlist.at(i) << "\n";
    }
    f.close();
  } catch( std::exception &e) {
    std::cerr << "\nSearchEngine::WriteImageListToFile() : error writing image filename list to [" << fn << "]" << std::flush;
    std::cerr << e.what() << std::flush;
  }
}

void SearchEngine::LoadImglist() {
  try {
    std::ifstream f( imglist_fn_.string().c_str() );

    std::string imglist_i;
    while( std::getline(f, imglist_i, '\n') ) {
      imglist_.push_back( imglist_i );
    }
    f.close();
  } catch( std::exception &e) {
    std::cerr << "\nSearchEngine::LoadImglist() : error reading image filename list from [" << imglist_fn_.string() << "]" << std::flush;
    std::cerr << e.what() << std::flush;
  }
}

void SearchEngine::WriteConfigToFile() {
  try {
    std::map<std::string, std::string>::iterator it;
    std::ofstream f( engine_config_fn_.c_str() );

    f << "[" << engine_name_ << "]";
    for ( it = engine_config_.begin(); it != engine_config_.end(); ++it) {
      f << "\n" << it->first << "=" << it->second;
    }
    f.close();
  } catch( std::exception &e) {
    std::cerr << "\nSearchEngine::WriteConfigToFile() : error writing configuration to [" << engine_config_fn_ << "]" << std::flush;
    std::cerr << e.what() << std::flush;
  }
}

void SearchEngine::ReadConfigFromFile() {
  try {
    engine_config_.clear();
    std::ifstream f( engine_config_fn_.c_str() );
    char line[1024];
    f.getline(line, 1024, '\n');
    std::string search_engine_name(line);
    assert( engine_name_ == search_engine_name );

    while ( !f.eof() ) {
      f.getline(line, 1024, '\n');
      std::string param( line );
      std::size_t eq_pos = param.find('=');
      if ( eq_pos != std::string::npos ) {
        std::string key   = param.substr(0, eq_pos);
        std::string value = param.substr(eq_pos+1, std::string::npos);
        SetEngineConfigParam(key, value);
      }
    }
    f.close();
  } catch( std::exception &e) {
    std::cerr << "\nSearchEngine::ReadConfigFromFile() : error reading configuration from [" << engine_config_fn_ << "]" << std::flush;
    std::cerr << e.what() << std::flush;
  }
}

void SearchEngine::SendMessage(std::string msg) {
  SendPacket(GetCurrentStateName(), "message", msg);
}

void SearchEngine::SendCommand(std::string sender, std::string command) {
  SendPacket(sender, "command", command);
}

void SearchEngine::SendProgressMessage(std::string state_name, std::string msg) {
  std::ostringstream s;
  s << state_name << " progress " << msg;
  ViseMessageQueue::Instance()->Push( s.str() );
}

void SearchEngine::SendProgress(std::string state_name, unsigned long completed, unsigned long total) {
  std::ostringstream s;
  s << state_name << " progress " << completed << "/" << total;
  ViseMessageQueue::Instance()->Push( s.str() );
}

void SearchEngine::SendLog(std::string sender, std::string log) {
  SendPacket(sender, "log", log);
}

void SearchEngine::SendPacket(std::string sender, std::string type, std::string message) {
  std::ostringstream s;
  s << sender << " " << type << " " << message;
  ViseMessageQueue::Instance()->Push( s.str() );
}

//
// Helper methods
//
std::string SearchEngine::GetName() {
  return engine_name_;
}
bool SearchEngine::IsEngineConfigEmpty() {
  return engine_config_.empty();
}
bool SearchEngine::IsImglistEmpty() {
  return imglist_.empty();
}
bool SearchEngine::EngineConfigFnExists() {
  return boost::filesystem::exists( engine_config_fn_ );
}
bool SearchEngine::ImglistFnExists() {
  return boost::filesystem::exists( imglist_fn_ );
}
bool SearchEngine::DescFnExists() {
  boost::filesystem::path train_desc_fn( GetEngineConfigParam("descFn") );
  return boost::filesystem::exists( train_desc_fn );
}
bool SearchEngine::ClstFnExists() {
  boost::filesystem::path clst_fn( GetEngineConfigParam("clstFn") );
  return boost::filesystem::exists( clst_fn );
}
bool SearchEngine::AssignFnExists() {
  boost::filesystem::path assign_fn( GetEngineConfigParam("assignFn") );
  return boost::filesystem::exists( assign_fn );
}
bool SearchEngine::HammFnExists() {
  boost::filesystem::path hamm_fn( GetEngineConfigParam("hammFn") );
  return boost::filesystem::exists( hamm_fn );
}
bool SearchEngine::IndexFnExists() {
  boost::filesystem::path dset_fn( GetEngineConfigParam("dsetFn") );
  boost::filesystem::path fidx_fn( GetEngineConfigParam("fidxFn") );
  boost::filesystem::path iidx_fn( GetEngineConfigParam("iidxFn") );
  bool dset_exist = boost::filesystem::exists( dset_fn );
  bool fidx_exist = boost::filesystem::exists( fidx_fn );
  bool iidx_exist = boost::filesystem::exists( iidx_fn );
  return ( dset_exist && fidx_exist && iidx_exist );
}
unsigned long SearchEngine::DescFnSize() {
  boost::filesystem::path train_desc_fn( GetEngineConfigParam("descFn") );
  return boost::filesystem::file_size( train_desc_fn.string() );
}
unsigned long SearchEngine::ClstFnSize() {
  boost::filesystem::path clst_fn( GetEngineConfigParam("clstFn") );
  return boost::filesystem::file_size( clst_fn.string() );
}
unsigned long SearchEngine::AssignFnSize() {
  boost::filesystem::path assign_fn( GetEngineConfigParam("assignFn") );
  return boost::filesystem::file_size( assign_fn.string() );
}
unsigned long SearchEngine::HammFnSize() {
  boost::filesystem::path hamm_fn( GetEngineConfigParam("hammFn") );
  return boost::filesystem::file_size( hamm_fn.string() );
}
unsigned long SearchEngine::IndexFnSize() {
  boost::filesystem::path dset_fn( GetEngineConfigParam("dsetFn") );
  boost::filesystem::path fidx_fn( GetEngineConfigParam("fidxFn") );
  boost::filesystem::path iidx_fn( GetEngineConfigParam("iidxFn") );
  unsigned long dset_size = boost::filesystem::file_size( dset_fn.string() );
  unsigned long fidx_size = boost::filesystem::file_size( fidx_fn.string() );
  unsigned long iidx_size = boost::filesystem::file_size( iidx_fn.string() );
  return (dset_size + fidx_size + iidx_size);
}
unsigned long SearchEngine::GetImglistOriginalSize() {
  unsigned long total_size = 0;
  for ( unsigned int i=0; i<imglist_fn_original_size_.size(); i++) {
    total_size += imglist_fn_original_size_.at(i);
  }
  return total_size;
}
unsigned long SearchEngine::GetImglistTransformedSize() {
  unsigned long total_size = 0;
  for ( unsigned int i=0; i<imglist_fn_transformed_size_.size(); i++) {
    total_size += imglist_fn_transformed_size_.at(i);
  }
  return total_size;
}

boost::filesystem::path SearchEngine::GetOriginalImageDir() {
  return original_imgdir_;
}
boost::filesystem::path SearchEngine::GetTransformedImageDir() {
  return transformed_imgdir_;
}

unsigned long SearchEngine::GetImglistSize() {
  return imglist_.size();
}

std::string SearchEngine::GetImglistFn( unsigned int index ) {
  return imglist_.at(index);
}

//
// Search Engine State
//
bool SearchEngine::UpdateState() {
  if ( state_id_ == SearchEngine::STATE_NOT_LOADED ) {
    // check if search engine was initialized properly
    if ( GetName() == "" ) {
      return false;
    } else {
      state_id_ = SearchEngine::STATE_SETTING;
      return true;
    }
  } else if ( state_id_ == SearchEngine::STATE_SETTING ) {
    if ( IsEngineConfigEmpty() ) {
      return false;
    } else {
      state_id_ = SearchEngine::STATE_INFO;
      return true;
    }
  } else if ( state_id_ == SearchEngine::STATE_INFO ) {
    state_id_ = SearchEngine::STATE_PREPROCESS;
    return true;
  } else if ( state_id_ == SearchEngine::STATE_PREPROCESS ) {
    if ( EngineConfigFnExists() && ImglistFnExists() ) {
      state_id_ = SearchEngine::STATE_DESCRIPTOR;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == SearchEngine::STATE_DESCRIPTOR ) {
    if ( DescFnExists() ) {
      state_id_ = SearchEngine::STATE_CLUSTER;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == SearchEngine::STATE_CLUSTER ) {
    if ( ClstFnExists() ) {
      state_id_ = SearchEngine::STATE_ASSIGN;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == SearchEngine::STATE_ASSIGN ) {
    if ( AssignFnExists() ) {
      state_id_ = SearchEngine::STATE_HAMM;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == SearchEngine::STATE_HAMM ) {
    if ( HammFnExists() ) {
      state_id_ = SearchEngine::STATE_INDEX;
      return true;
    } else {
      return false;
    }
  } else if ( state_id_ == SearchEngine::STATE_INDEX ) {
    if ( IndexFnExists() ) {
      state_id_ = SearchEngine::STATE_QUERY;
      return true;
    } else {
      return false;
    }
  }
  return false;
}

std::string& SearchEngine::GetStateJsonData() {
  std::ostringstream json;
  json << "{ \"id\":\"search_engine_state\",";
  json << "\"state_id_list\":[0";
  for ( unsigned int i=1 ; i < state_id_list_.size(); i++ ) {
    json << "," << i;
  }
  json << "],\"state_name_list\":[\"" << state_name_list_.at(0) << "\"";
  for ( unsigned int i=1 ; i < state_id_list_.size(); i++ ) {
    json << ",\"" << state_name_list_.at(i) << "\"";
  }
  json << "],\"state_info_list\":[\"" << state_info_list_.at(0) << "\"";
  for ( unsigned int i=1 ; i < state_id_list_.size(); i++ ) {
    json << ",\"" << state_info_list_.at(i) << "\"";
  }
  json << "],\"current_state_id\":" << state_id_ << ",";
  json << "\"search_engine_name\":\"" << engine_name_ << "\" }";

  state_json_ = json.str();
  return state_json_;
}

void SearchEngine::LoadStateResources() {
  state_id_list_.clear();
  state_name_list_.clear();
  state_html_fn_list_.clear();
  state_info_list_.clear();
  state_complexity_model_.clear();

  state_id_list_.push_back( SearchEngine::STATE_NOT_LOADED );
  state_name_list_.push_back( "NotLoaded" );
  state_html_fn_list_.push_back( "vise_404.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_SETTING );
  state_name_list_.push_back( "Setting" );
  state_html_fn_list_.push_back( "Setting.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_INFO );
  state_name_list_.push_back( "Info" );
  state_html_fn_list_.push_back( "Info.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_PREPROCESS );
  state_name_list_.push_back( "Preprocess" );
  state_html_fn_list_.push_back( "Preprocess.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_DESCRIPTOR );
  state_name_list_.push_back( "Stage-1" );
  state_html_fn_list_.push_back( "Descriptor.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_CLUSTER );
  state_name_list_.push_back( "Stage-2" );
  state_html_fn_list_.push_back( "Cluster.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_ASSIGN );
  state_name_list_.push_back( "Stage-3" );
  state_html_fn_list_.push_back( "Assign.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_HAMM );
  state_name_list_.push_back( "Stage-4" );
  state_html_fn_list_.push_back( "Hamm.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_INDEX );
  state_name_list_.push_back( "Stage-5" );
  state_html_fn_list_.push_back( "Index.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  state_id_list_.push_back( SearchEngine::STATE_QUERY );
  state_name_list_.push_back( "Ready to Search" );
  state_html_fn_list_.push_back( "Query.html" );
  state_info_list_.push_back( "" );
  state_complexity_model_.push_back( std::vector<double>(4, 0.0) );

  total_complexity_model_ = std::vector<double>(4, 0.0);

  state_id_ = SearchEngine::STATE_NOT_LOADED;
}

void SearchEngine::LoadStateComplexityModel() {
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
  state_complexity_model_.at( SearchEngine::STATE_PREPROCESS ).at(0) = -0.0608757062; // coef_0 (time)
  state_complexity_model_.at( SearchEngine::STATE_PREPROCESS ).at(1) =  0.0011031073; // coef_1 (time)
  state_complexity_model_.at( SearchEngine::STATE_PREPROCESS ).at(2) =  3.374722;     // coef_0 (space)
  state_complexity_model_.at( SearchEngine::STATE_PREPROCESS ).at(3) =  0.427843547;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( SearchEngine::STATE_DESCRIPTOR ).at(0) = 0.2545197740; // coef_0 (time)
  state_complexity_model_.at( SearchEngine::STATE_DESCRIPTOR ).at(1) = 0.0031129944; // coef_1 (time)
  state_complexity_model_.at( SearchEngine::STATE_DESCRIPTOR ).at(2) = 4.768372e-06; // coef_0 (space)
  state_complexity_model_.at( SearchEngine::STATE_DESCRIPTOR ).at(3) = 0.122070312;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( SearchEngine::STATE_CLUSTER ).at(0) = -1.5004237288; // coef_0 (time)
  state_complexity_model_.at( SearchEngine::STATE_CLUSTER ).at(1) =  0.0364477401; // coef_1 (time)
  state_complexity_model_.at( SearchEngine::STATE_CLUSTER ).at(2) =  3.147125e-05; // coef_0 (space)
  state_complexity_model_.at( SearchEngine::STATE_CLUSTER ).at(3) =  0.048828125;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  state_complexity_model_.at( SearchEngine::STATE_INDEX ).at(0) = -0.4600282486; // coef_0 (time)
  state_complexity_model_.at( SearchEngine::STATE_INDEX ).at(1) =  0.0175409605; // coef_1 (time)
  state_complexity_model_.at( SearchEngine::STATE_INDEX ).at(2) = -1.438618;     // coef_0 (space)
  state_complexity_model_.at( SearchEngine::STATE_INDEX ).at(3) =  0.072752569;  // coef_1 (space)

  // time_min = coef_0 + coef_1 * img_count
  total_complexity_model_.at(0) = -1.8307909605; // coef_0 (time)
  total_complexity_model_.at(1) =  0.0591468927; // coef_1 (time)
  total_complexity_model_.at(2) =  1.967406;     // coef_0 (space)
  total_complexity_model_.at(3) =  0.699723314;  // coef_1 (space)
}


std::string SearchEngine::GetStateComplexityInfo() {
  unsigned long n = GetImglistSize();

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

void SearchEngine::UpdateStateInfoList() {
  unsigned long n = GetImglistSize();
  std::vector< int > state_id_list;
  state_id_list.push_back( SearchEngine::STATE_PREPROCESS );
  state_id_list.push_back( SearchEngine::STATE_DESCRIPTOR );
  state_id_list.push_back( SearchEngine::STATE_CLUSTER );
  state_id_list.push_back( SearchEngine::STATE_INDEX );

  for (unsigned int i=0; i<state_id_list.size(); i++) {
    int state_id = state_id_list.at(i);
    std::vector< double > m = state_complexity_model_.at(state_id);
    double time  = m[0] + m[1] * n; // in minutes
    double space = m[2] + m[3] * n; // in MB

    if ( time < 0.0 ) {
      time = 0.0;
    }
    if ( space < 0.0 ) {
      space = 0.0;
    }

    std::ostringstream sinfo;
    sinfo << "(" << ceil(time) << " min, " << ceil(space) << " MB)";
    state_info_list_.at( state_id ) = sinfo.str();
    //std::cout << "\nm=" << m[0] << "," << m[1] << "," << m[2] << "," << m[3] << std::flush;
    //std::cout << "\nstate = " << state_id << " : " << sinfo.str() << std::flush;
  }
}

std::string SearchEngine::GetCurrentStateName() const {
  return GetStateName( state_id_ );
}

std::string SearchEngine::GetCurrentStateInfo() const {
  return GetStateInfo( state_id_ );
}

std::string SearchEngine::GetStateHtmlFn(int state_id) const {
  return state_html_fn_list_.at(state_id);
}

int SearchEngine::GetStateId( std::string state_name ) const {
  for ( unsigned int i=0; i < state_id_list_.size(); i++) {
    if ( state_name_list_.at(i) == state_name ) {
      return state_id_list_.at(i);
    }
  }
  return -1; // not found
}

std::string SearchEngine::GetStateName(int state_id) const {
  return state_name_list_.at( state_id );
}

std::string SearchEngine::GetStateInfo(int state_id) const {
  return state_info_list_.at( state_id );
}

int SearchEngine::GetCurrentStateId() const {
  return state_id_;
}

//
// static methods to help with search engine management
//
bool SearchEngine::Exists( std::string search_engine_name ) {
  // iterate through all directories in basedir_
  boost::filesystem::directory_iterator dir_it( basedir_ ), end_it;
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

bool SearchEngine::ValidateName(std::string engine_name) {
  // search engine name cannot contain:
  //  - spaces
  //  - special characters (*, ?, /)
  std::size_t space = engine_name.find(' ');
  std::size_t asterix = engine_name.find('*');
  std::size_t qmark = engine_name.find('?');
  std::size_t fslash = engine_name.find('/');
  std::size_t bslash = engine_name.find('\\');
  std::size_t dot = engine_name.find('.');

  if ( space  == std::string::npos ||
       asterix== std::string::npos ||
       qmark  == std::string::npos ||
       bslash == std::string::npos ||
       dot    == std::string::npos ||
       fslash == std::string::npos ) {
    return true;
  } else {
    return false;
  }
}

void SearchEngine::GetSearchEngineList( std::vector< std::string >& engine_list ) const {
  engine_list.clear();

  // iterate through all directories in engine_basedir
  boost::filesystem::directory_iterator dir_it( basedir_ ), end_it;
  while ( dir_it != end_it ) {
    boost::filesystem::path p = dir_it->path();
    if ( boost::filesystem::is_directory( p ) ) {
      engine_list.push_back( p.filename().string() );
    }
    ++dir_it;
  }
}

bool SearchEngine::Delete( std::string search_engine_name ) {
  // major security risk: validate that search_engine_name is really a engine name
  // and not something like name/../../../../etc/passwd
  // docker running as root is capable of deleting everything. So be careful!
  // @todo: revisit this and ensure safety of user data
  if ( SearchEngine::ValidateName(search_engine_name) ) {
    boost::filesystem::path engine_path = basedir_ / search_engine_name;
    boost::filesystem::remove_all( engine_path, error_ );
    if ( !error_ ) {
      return true;
    }
  }
  return false;
}

//
// Search engine training
//
void SearchEngine::StartTraining() {
  training_thread_ = new boost::thread( boost::bind( &SearchEngine::Train, this ) );
}

void SearchEngine::StopTraining() {
  try {
    //std::cout << "\nAttempting to stop the training thread ... " << std::flush;
    training_thread_->interrupt();
  } catch( std::exception& e ) {
    std::cerr << "\nFailed to stop training_thread" << std::endl;
    std::cerr << e.what() << std::flush;    
  }
}

void SearchEngine::Train() {
  //std::cout << "\nTrain() thread started ... " << boost::this_thread::get_id() << std::flush;
  SendCommand("SearchEngine", "_log clear hide");
  SendCommand("SearchEngine", "_control_panel clear all");
  SendCommand("SearchEngine", "_control_panel add <div id=\"toggle_log\" class=\"action_button\" onclick=\"_vise_toggle_log()\">Log</div>");

  // Pre-process
  if ( state_id_ == SearchEngine::STATE_PREPROCESS ) {
    Preprocess();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("SearchEngine", "_state update_now");
    } else {
      SendLog("SearchEngine", "\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }

  if ( training_thread_->interruption_requested() ) {
    SendLog("SearchEngine", "\nStopped training process on user request");
    return;
  }

  // Descriptor
  if ( state_id_ == SearchEngine::STATE_DESCRIPTOR ) {
    Descriptor();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("SearchEngine", "_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }

  if ( training_thread_->interruption_requested() ) {
    SendLog("SearchEngine", "\nStopped training process on user request");
    return;
  }

  // Cluster
  if ( state_id_ == SearchEngine::STATE_CLUSTER ) {
    Cluster();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("SearchEngine", "_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }

  if ( training_thread_->interruption_requested() ) {
    SendLog("SearchEngine", "\nStopped training process on user request");
    return;
  }

  // Assign
  if ( state_id_ == SearchEngine::STATE_ASSIGN ) {
    Assign();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("SearchEngine", "_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }

  if ( training_thread_->interruption_requested() ) {
    SendLog("SearchEngine", "\nStopped training process on user request");
    return;
  }

  // Hamm
  if ( state_id_ == SearchEngine::STATE_HAMM ) {
    Hamm();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("SearchEngine", "_state update_now");
    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }

  if ( training_thread_->interruption_requested() ) {
    SendLog("SearchEngine", "\nStopped training process on user request");
    return;
  }

  // Index
  if ( state_id_ == SearchEngine::STATE_INDEX ) {
    Index();

    if ( UpdateState() ) {
      // send control message : state updated
      SendCommand("SearchEngine", "_state update_now");

    } else {
      SendMessage("\n" + GetCurrentStateName() + " : failed to change to next state");
      return;
    }
  }

  QueryInit();
  SendCommand("SearchEngine", "_go_to home");
}

void SearchEngine::QueryInit() {

  /*
  // construct dataset
  dataset_ = new datasetV2( search_engine_.GetEngineConfigParam("dsetFn"),
  search_engine_.GetEngineConfigParam("databasePath"),
  search_engine_.GetEngineConfigParam("docMapFindPath") );

  // load the search index in separate thread
  // while the user browses image list and prepares search area
  vise_load_search_index_thread_ = new boost::thread( boost::bind( &ViseServer::QueryLoadSearchIndex, this ) );
  */
  boost::thread t( boost::bind( &SearchEngine::InitReljaRetrival, this ) );
}

// TEMPORARY CODE -- WILL BE REMOVED IN FUTURE WHEN NEW FRONTEND IS RELEASED
// setup relja_retrival backend and frontend (temporary, until JS based frontend is ready)
extern void api_v2(std::vector< std::string > argv);
void SearchEngine::InitReljaRetrival() {
  boost::thread backend( boost::bind( &SearchEngine::InitReljaRetrivalBackend, this ) );
  //boost::thread frontend( boost::bind( &ViseServer::InitReljaRetrivalFrontend, this ) );

  backend.join();
  //frontend.join();
}

// Note: frontend is invoked by src/api/abs_api.cpp::InitReljaRetrivalFrontend()
void SearchEngine::InitReljaRetrivalBackend() {
  // start relja_retrival backend
  std::vector< std::string > param;
  param.push_back("api_v2");
  param.push_back("65521");
  param.push_back( GetName() );
  param.push_back( GetEngineConfigFn().string() );
  param.push_back( vise_src_code_dir_.string() );
  api_v2( param );
}

//
// for debug
//
void SearchEngine::PrintEngineConfig() {
  std::cout << "\nShowing configurations for [" << engine_name_ << "] :" << std::endl;
  std::map<std::string, std::string>::iterator it;
  for ( it = engine_config_.begin(); it != engine_config_.end(); ++it) {
    std::cout << it->first << " = " << it->second << std::endl;
  }
  std::cout << std::flush;
}
