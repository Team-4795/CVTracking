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

  int lowH = 70;
  int highH = 56;

  int lowS = 89;
  int highS = 255;

  int lowV = 255;
  int highV = 58;
};

#endif
