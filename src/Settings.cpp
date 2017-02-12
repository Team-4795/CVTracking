#include <fstream>
#include <json/json.h>
#include "Settings.h"

const unordered_map<string,Settings::FieldSpec> Settings::defaults = {
  // not saved in config
  {"running", {true,  false}},
  {"GUI",     {false, false}},
  {"debug",   {false, false}},

  // saved in config
  {"mode",  {Settings::Mode::USB, true}},
  {"camera-index", {0, true}},
  {"static-path", {"static_image.jpg", true}},
  {"stream-path", {"http://axis-camera.local/mjpg/video.mjpg", true}},
  
  {"lowH",  {70,  true}},
  {"lowS",  {56,  true}},
  {"lowV",  {142, true}},
  {"highH", {96,  true}},
  {"highS", {255, true}},
  {"highV", {255, true}}
};

Settings::Settings(void)
{
  ;
}

void Settings::load_defaults(void)
{
  this->clear();
  for(const auto &pair : defaults) {
    (*this)[pair.first] = pair.second.value;
  }
}

void Settings::load_config(const string &filename)
{
  load_defaults();
  
  ifstream file(filename, fstream::binary);
  if (file.fail())
  {
    fprintf(stderr, "Config file not found: %s\nUsing default config...\n", filename.c_str());
    return;
  }

  Json::Value root;
  file >> root;
  file.close();

  for(const auto &key : root.getMemberNames()) {
    (*this)[key] = root[key];
  }
}

void Settings::save_config(const string &filename) const
{
  Json::Value root;
  for(const auto &key : this->getMemberNames()) {
    try {
      if(defaults.at(key).save) {
	root[key] = (*this)[key];
      }
    } catch(out_of_range &exception) {
      ;
    }
  }
  
  ofstream file(filename, fstream::binary | fstream::trunc);
  if(file.fail())
  {
    fprintf(stderr, "Failed to open config file for writing: %s\nAborting save...\n", filename.c_str());
    return;
  }
  
  file << root;
  file.close();
}
