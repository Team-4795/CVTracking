#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <string>
#include <fstream>

using namespace std;

struct Settings
{
  bool running = true;
  bool GUI = false;
  bool debug = false;

  enum Mode {
    USB,
    STREAM,
    STATIC
  };
  Mode mode;

  int cam_index = 0;
  string static_path = "static_image.jpg";
  string stream_path = "http://axis-camera.local/mjpg/video.mjpg";

  int lowH = 53;
  int highH = 255;

  int lowS = 0;
  int highS = 255;

  int lowV = 150;
  int highV = 255;
};

#endif
