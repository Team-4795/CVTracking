#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <string>
#include <fstream>
#include <unordered_map>
#include <json/json.h>

using namespace std;

class Settings : public Json::Value
{
  struct FieldSpec {
    Json::Value value;
    bool save;
  };
  static const unordered_map<string,FieldSpec> defaults;
public:
  Settings(void);

  enum Mode {
    USB,
    STATIC,
    STREAM
  };
  
  void load_defaults(void);
  void load_config(const string &filename);
  void save_config(const string &filename) const;
};

#endif
