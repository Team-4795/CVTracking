#include <fstream>
#include <json/json.h>
#include "Settings.h"

const int Settings::def_cam_index = 0;
const int Settings::def_lowH = 70;
const int Settings::def_highH = 96;
const int Settings::def_lowS = 56;
const int Settings::def_highS = 255;
const int Settings::def_lowV = 142;
const int Settings::def_highV = 255;

const string Settings::cam_index_key = "camera-index";
const string Settings::lowH_key = "lowH";
const string Settings::highH_key = "highH";
const string Settings::lowS_key = "lowS";
const string Settings::highS_key = "highS";
const string Settings::lowV_key = "lowV";
const string Settings::highV_key = "highV";

void Settings::load_config(string filename)
{
  ifstream file(filename, fstream::binary);
  if (file.fail())
  {
    printf("Config file not found: %s\nUsing default config...\n", filename.c_str());
    camera_index = def_cam_index;
    lowH = def_lowH;
    highH = def_highH;
    lowS = def_lowS;
    highS = def_highS;
    lowV = def_lowV;
    highV = def_highV;
    return;
  }

  Json::Value root;
  file >> root;
  file.close();

  camera_index = root.get(cam_index_key, def_cam_index).asInt();
  lowH = root.get(lowH_key, def_lowH).asInt();
  highH = root.get(highH_key, def_highH).asInt();
  lowS = root.get(lowS_key, def_lowS).asInt();
  highS = root.get(highS_key, def_highS).asInt();
  lowV = root.get(lowV_key, def_lowV).asInt();
  highV = root.get(highV_key, def_highV).asInt();
}

void Settings::save_config(string filename)
{
  Json::Value root;

  root[cam_index_key] = camera_index;

  root[lowH_key] = lowH;
  root[highH_key] = highH;

  root[lowS_key] = lowS;
  root[highS_key] = highS;

  root[lowV_key] = lowV;
  root[highV_key] = highV;

  ofstream file(filename, fstream::binary | fstream::trunc);
  file << root;
  file.close();
}
