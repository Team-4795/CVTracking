#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <string>
#include <fstream>
#include <json/json.h>

using namespace std;

class Settings
{
  // settings defaults
  static const int def_cam_index;
  static const int def_lowH;
  static const int def_highH;
  static const int def_lowS;
  static const int def_highS;
  static const int def_lowV;
  static const int def_highV;
  // json keys
  static const string cam_index_key;
  static const string lowH_key;
  static const string highH_key;
  static const string lowS_key;
  static const string highS_key;
  static const string lowV_key;
  static const string highV_key;
public:
  //whileloop condition
  bool running = true;
  bool GUI = false;
  bool debug = false;
  bool static_image = false;
  bool streamed_image = false;
  
  int camera_index;
  int lowH, highH;
  int lowS, highS;
  int lowV, highV;

  
  string static_path;
  Settings(void)
    : camera_index(def_cam_index),
      lowH(def_lowH), highH(def_highH),
      lowS(def_lowS), highS(def_highS),
      lowV(def_lowV), highV(def_highV) {}
  Settings(int camera_index,
           int lowH, int highH,
           int lowS, int highS,
           int lowV, int highV)
    : camera_index(camera_index),
      lowH(lowH), highH(highH),
      lowS(lowS), highS(highS),
      lowV(lowV), highV(highV) {}
  void load_config(string filename);
  void save_config(string filename);
};

#endif
