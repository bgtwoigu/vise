#include "Resources.h"

void Resources::LoadAllResources(const boost::filesystem::path resource_dir) {
  resource_dir_      = resource_dir;

  vise_css_fn_       = resource_dir_ / "vise.css";
  vise_js_fn_        = resource_dir_ / "vise.js";
  vise_main_html_fn_ = resource_dir_ / "vise_main.html";
  vise_help_html_fn_ = resource_dir_ / "help.html";
  vise_404_html_fn_  = resource_dir_ / "404.html";
  vise_favicon_fn_   = resource_dir_ / "favicon.ico";

  LoadFile(vise_main_html_fn_, vise_main_html_);
  LoadFile(vise_help_html_fn_, vise_help_html_);
  LoadFile(vise_404_html_fn_, vise_404_html_);
  LoadFile(vise_css_fn_, vise_css_);
  LoadFile(vise_js_fn_, vise_js_);

  LoadStateComplexityModel();
}

void Resources::LoadStateComplexityModel() {
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
  */

}

int Resources::LoadFile(const boost::filesystem::path filename, std::string &file_contents) {
  try {
    std::ifstream f;
    f.open(filename.string().c_str());
    f.seekg(0, std::ios::end);
    file_contents.reserve( f.tellg() );
    f.seekg(0, std::ios::beg);

    file_contents.assign( std::istreambuf_iterator<char>(f),
                          std::istreambuf_iterator<char>() );
    f.close();
    return 1;
  } catch (std::exception &e) {
    std::cerr << "\nResources::LoadFile() : failed to load file : " << filename.string() << std::flush;
    file_contents = "";
    return 0;
  }
}
