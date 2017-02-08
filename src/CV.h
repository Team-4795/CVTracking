#ifndef CV_H_
#define CV_H_

#include <vector>
#include <opencv2/opencv.hpp>
#include <zmq.hpp>
#include "Settings.h"

using namespace cv;
using namespace std;
using namespace zmq;

class Arectangle
{
public:
  int cx, cy, area;
};

class Image_capsule
{
public:
  //image containers
  Mat frame;
  Mat hsv_image;
  Mat threshHold_image;
  Mat contour_image;
  Mat hull_image;
};

class HSV_capsule
{
public:
  //HSV min max contraints
  Scalar hsv_min;
  Scalar hsv_max;
};

void show_help(void);
void findBoundingBox(Image_capsule &images, vector< vector<Point> > &contours);
void getContours(Image_capsule &images, vector< vector<Point> > &contours, vector <Vec4i> &hierarchy);
void findSquares(Image_capsule &images, vector< vector<Point> > &contours);
void init(Image_capsule &images, HSV_capsule &HSVs, Settings &settings);
void findConvexHull(Image_capsule &images, vector< vector<Point> > &contours, vector<vector<Point> > &hull, double &angle);

#endif
