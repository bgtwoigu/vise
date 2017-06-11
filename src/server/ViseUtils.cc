#include "ViseUtils.h"

namespace ViseUtils {

int FileLoad(const std::string filename, std::string &file_contents) {
  try {
    std::ifstream f;
    f.open(filename.c_str());
    f.seekg(0, std::ios::end);
    file_contents.reserve( f.tellg() );
    f.seekg(0, std::ios::beg);

    file_contents.assign( std::istreambuf_iterator<char>(f),
                          std::istreambuf_iterator<char>() );
    f.close();
    return 0;
  } catch (std::exception &e) {
    std::cerr << "\nResources::LoadFile() : failed to load file : " << filename << std::flush;
    file_contents = "";
    return 1;
  }
}


void FileWrite(std::string filename, std::string &file_contents) {
  try {
    std::ofstream f;
    f.open(filename.c_str());
    f << file_contents;
    f.close();
  } catch (std::exception &e) {
    std::cerr << "\nViseServer::WriteFile() : failed to save file : " << filename << std::flush;
  }

}

bool StringReplace(std::string &s, std::string old_str, std::string new_str) {
  std::string::size_type pos = s.find(old_str);
  if ( pos == std::string::npos ) {
    return false;
  } else {
    s.replace(pos, old_str.length(), new_str);
    return true;
  }
}

void StringSplit( const std::string s, char sep, std::vector<std::string> &tokens ) {
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

bool StringStartsWith( const std::string &s, const std::string &prefix ) {
  if ( s.substr(0, prefix.length()) == prefix ) {
    return true;
  } else {
    return false;
  }
}

void StringHttpUnescape( std::string &s ) {
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

// keyvalue_str = "key1=value1&key2=value2,..."
void StringParseKeyValue(std::string keyvalue_str, char separator, std::map< std::string, std::string >& keyvalue) {
  keyvalue.clear();
  if ( keyvalue_str == "" ) {
    return;
  }

  std::vector< std::string > tokens;
  ViseUtils::StringSplit( keyvalue_str, separator, tokens );

  for ( unsigned int i=0; i<tokens.size(); i++ ) {
    std::vector< std::string > keyvali;
    ViseUtils::StringSplit( tokens.at(i), '=', keyvali );

    if ( keyvali.size() == 2 ) {
      keyvalue.insert( std::pair<std::string, std::string>(keyvali.at(0), keyvali.at(1)) );
    } else {
      std::cerr << "\nViseUtils::StringParseKeyValue() : error parsing key/value data from [" << keyvalue_str << "]" << std::flush;
    }
  }
}


std::string HttpGetFileContentType( const boost::filesystem::path& fn) {
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

bool HttpGetHeaderValue(const std::string& header, const std::string& name, std::string& value) {
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

}

