#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include "zhelpers.hpp"
#include "CV.h"

Settings settings;

static void handle_signal(int signal)
{
  settings.save_config("config.json");
  switch (signal)
  {
  case 0:
  case SIGINT:
  case SIGPIPE:
  case SIGTERM:
    exit(0);
  case SIGSEGV:
  default:
    fprintf(stderr, "Termination due to abnormal signal: %d\n", signal);
  case -1:
    exit(1);
  }
}

void show_help(void)
{
  printf("CVTracking [-hud] [-c <camera index>] [-s <image path>] [-i <stream url>]\n"
	 "  -h  Show this help menu\n"
	 "  -u  User mode (camera view only)\n"
	 "  -d  Debug mode (camera, threshold, control views, settings sliders)\n"
	 "  -c  Set the camera index to use (starts at zero)\n"
	 "  -s  Use a static image instead of a connected camera\n"
	 "  -i  Use an mjpg stream instead of a connected camera\n");
}

VideoCapture capture;

int main(int argc, char **argv)
{
  settings.load_config("config.json");

  // setup signal handling to save settings always upon exit
  struct sigaction action;
  action.sa_flags = SA_RESTART;
  action.sa_handler = SIG_IGN;
  sigaction(SIGHUP, &action, nullptr);
  sigaction(SIGQUIT, &action, nullptr);
  sigaction(SIGUSR1, &action, nullptr);
  sigaction(SIGUSR2, &action, nullptr);
  action.sa_handler = handle_signal;
  sigaction(SIGINT, &action, nullptr);
  sigaction(SIGSEGV, &action, nullptr);
  sigaction(SIGPIPE, &action, nullptr);
  sigaction(SIGTERM, &action, nullptr);

  // parse command line arguments
  int arg;
  while ((arg = getopt(argc, argv, "hudc:s:i:")) != -1)
    switch (arg)
    {
    case 'h':
    default:
      show_help();
      return (optopt == 'h' ? 0 : 1);
    case 'u':
      settings["GUI"] = true;
      break;
    case 'd':
      settings["GUI"] = true;
      settings["debug"] = true;
      break;
    case 'c':
      settings["mode"] = Settings::Mode::USB;
      settings["camera-index"] = (int) strtol(optarg, nullptr, 10);
      break;
    case 's':
      settings["mode"] = Settings::Mode::STATIC;
      settings["static-path"] = optarg;
      break;
    case 'i':
      settings["mode"] = Settings::Mode::STREAM;
      settings["stream-path"] = optarg;
      break;
    }

  if (optind != argc)
  {
    int index;
    for (index = optind; index < argc; index++)
      fprintf(stderr, "Invalid argument: %s\n", argv[index]);

    show_help();
    return 1;
  }

  Image_capsule images;
  HSV_capsule HSVs; 
  context_t context(1);
  socket_t socket(context, ZMQ_PUB);

  socket.bind("tcp://*:5808");

  init(images, settings);

  //main loop
  while (settings["running"].asBool())
  {
    //make sure the scalars are updated with the new HSV values
    HSVs.hsv_min = Scalar(settings["lowH"].asInt(), settings["lowS"].asInt(), settings["lowV"].asInt());
    HSVs.hsv_max = Scalar(settings["highH"].asInt(), settings["highS"].asInt(), settings["highV"].asInt());
    
    if(settings["mode"].asInt() == Settings::Mode::STATIC)
    {
      images.frame = imread(settings["static-path"].asString(), CV_LOAD_IMAGE_COLOR);
      if(images.frame.data == nullptr) {
	fprintf(stderr, "Error, static image not found.\n");
	handle_signal(-1);
      }
    }
    else
    {
      //get a fresh image from the camera
      capture >> images.frame;
      if (!capture.isOpened())
      {
	fprintf(stderr, "Error, image source not found.\n");
	handle_signal(-1);
      }
    }
    
    //filter to HSV and then the color picker filter
    cvtColor(images.frame, images.hsv_image, CV_BGR2HSV);
    inRange(images.hsv_image, HSVs.hsv_min, HSVs.hsv_max, images.threshHold_image);
    
    //find contours in the image
    vector< vector<Point> > contours;
    vector <Vec4i> hierarchy;
    getContours(images, contours, hierarchy);

    vector< vector<Point> > hull(contours.size());
    contourData contour_data[contours.size()];
    contourData tape[contours.size()] ;
    contourData goal[contours.size()];
    findConvexHull(images, contours, hull, contour_data);
    findTape(contour_data,tape);
    findGoal(tape,goal);
    if (settings["GUI"].asBool())
    {
      //show the raw image and the filtered images
      imshow("RGB", images.frame);
      if (settings["debug"].asBool())
        imshow("Thresh", images.threshHold_image);
    }

    if (contours.size() > 0)
    {
      char cmsg[32];
      snprintf(cmsg, sizeof(cmsg), "%.4f", contour_data[1].Angle);
      s_send(socket, string(cmsg));
    }
    
    //check if ESC is pressed to exit the program;
    if ((cvWaitKey(10) & 255) == 27)
      break;
  }
  
  handle_signal(0);
}

// XXX user is a const char * setting name, do not modify
static void trackbar_callback(int pos, void *user)
{
  const char *setting = static_cast<const char *>(user);

  settings[setting] = pos;
}

#define add_setting_slider(name, max_value)				\
  createTrackbar(name, "Control", nullptr, 255, trackbar_callback, const_cast<char *>(name)); \
  setTrackbarPos(name, "Control", settings[name].asInt())

void init(Image_capsule &images, Settings &settings)
{
  if(settings["mode"].asInt() == Settings::Mode::STREAM)
  {
    if(!capture.open(settings["stream-path"].asString())) {
      fprintf(stderr, "Failed to open mjpg stream for reading: %s\n", settings["stream-path"].asCString());
      handle_signal(-1);
    }
  }
  else
  {
    capture = VideoCapture(settings["camera-index"].asInt());
  }
  
  if (!capture.isOpened())
  {
    fprintf(stderr, "Error, image source not found.\n");
    handle_signal(-1);
  }
  
  if (settings["GUI"].asBool())
  {
    //make all the windows needed
    namedWindow("RGB", WINDOW_AUTOSIZE);
    if (settings["debug"].asBool())
    {
      namedWindow("Thresh", WINDOW_AUTOSIZE);
      namedWindow("Control", WINDOW_AUTOSIZE);

      //make trackbars to control the HSV min max values
      add_setting_slider("lowH", 255);
      add_setting_slider("lowS", 255);
      add_setting_slider("lowV", 255);
      add_setting_slider("highH", 255);
      add_setting_slider("highS", 255);
      add_setting_slider("highV", 255);
    }
  }

  int height, width;
  Size size;
  capture >> images.frame;
  height = images.frame.size().height;
  width = images.frame.size().width;
  size = Size(width, height);
  
  //make our filtered images
  images.hsv_image = Mat(size, CV_8UC3);
  images.threshHold_image = Mat(size, CV_8UC1);
  images.contour_image = Mat(size, CV_8UC1);
  images.hull_image = Mat(size, CV_8UC1);
}

void getContours(Image_capsule &images, vector< vector<Point> > &contours, vector <Vec4i> &hierarchy)
{
  //filter until only contours appear
  Canny(images.threshHold_image, images.contour_image, 255, 255, 3);
  morphologyEx(images.contour_image, images.contour_image, MORPH_CLOSE, Mat(), Point(-1, -1),1);
  findContours(images.contour_image, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
}

int calculate_threshold_area(size_t contour_count, vector<vector<Point>> &hull)
{
  int maxArea = 500;
  for (size_t i = 0; i < contour_count; i++)
  {
    int area = contourArea(hull[i]);
    if (area > maxArea)
      maxArea = area;
  }
  return maxArea / 3;
}

double radian_to_degrees(double radian)
{
  return radian * 180 / M_PI;
}

void setAngle(contourData object)
{
  double cx = 400;
  double f = 476.7;
  object.Angle = atan((object.X - cx) / f);
}
void findConvexHull(Image_capsule &images, vector< vector<Point> > &contours, vector<vector<Point> > &hull
                    , contourData data[])
{
  for (size_t i = 0; i < contours.size(); i++)
    convexHull(Mat(contours[i]), hull[i], false);
  
  int threshold_area = 200;
  //printf("%d\n",threshold_area);
  for (size_t i = 0; i < contours.size(); i++)
  {
    int area = contourArea(hull[i]);
    if (area > threshold_area)
    {
      Scalar color = Scalar(255, 0, 0);
      drawContours(images.frame, hull, i, color, 1, 8, vector<Vec4i>(), 0, Point());
      Moments M = moments(hull[i]);
      int u = int(M.m10 / M.m00);
      int v = int(M.m01 / M.m00);

      circle(images.frame, Point(u, v), 2, color, 4);

      data[i].X = u;
      data[i].Y = v;
      data[i].Area = area;
    }
  }
}

void findTape(contourData *data,contourData *tape)
{
  int pos_thresh = 5;
  for(int i = 0;i < sizeof(data);i++)
  {
    bool merged = false;
    if(data[i].X != NULL)
    {
      for(int y = i + 1;y < sizeof(data);i++)
      {
	if(data[y].X != NULL)
	{
	  if (data[i].X < data[y].X + pos_thresh || data[i].X > data[y].X - pos_thresh)
	  {
	    if (data[i].Y < data[y].Y + pos_thresh || data[i].Y > data[y].Y - pos_thresh)
	    {
	      tape[i].X = (data[i].X + data[y].X) / 2;
	      tape[i].Y = (data[i].Y + data[y].Y) / 2;
	      tape[i].Area = (data[i].Area + data[y].Area) / 2;
	      merged = true;
	    }
	  }
	  if(merged == false)
	  {
	    tape[i].X = data[i].X;
	    tape[i].Y = data[i].Y;
	    tape[i].Area = data[i].Area;
	  }
	}
      }
    }
  }
}

void findGoal(contourData *tape,contourData *goal)
{
  int pos_thresh = 5;
  for(int i = 0;i < sizeof(tape);i++)
  {
    if(tape[i].X != NULL)
    {
      for(int y = i + 1;y < sizeof(tape);i++)
      {
	if(tape[y].X != NULL)
	{
	  if (tape[i].X < tape[y].X + pos_thresh || tape[i].X > tape[y].X - pos_thresh)
	  {
	    if (tape[i].Y < tape[y].Y + pos_thresh || tape[i].Y > tape[y].Y - pos_thresh)
	    {
	      goal[i].X = (tape[i].X + tape[y].X) / 2;
	      goal[i].Y = (tape[i].Y + tape[y].X) / 2;
	      goal[i].Area = tape[i].Area + tape[y].Area;
	    }
	  }
	}
      }
    }
  }
}

// void findBoundingBox(Image_capsule &images, vector< vector<Point> > &contours)
// {
//   vector< vector<Point> > contours_poly(contours.size());
//   vector<Rect> boundRect(contours.size());
//   vector<Rect> filteredRect(boundRect.size());

//   for (size_t i = 0; i < contours.size(); i++)
//   {
//     approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
//     boundRect[i] = boundingRect(Mat(contours_poly[i]));
//   }

//   for (size_t i = 0; i < contours.size(); i++)
//   {
//     Scalar color = Scalar(255, 0, 0);
//     rectangle(images.frame, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
//   }
// }


// //// currently unused algorithm
// Arectangle rects[10];
// Arectangle finalRects[10];

// // helper function:
// // finds a cosine of angle between vectors
// // from pt0->pt1 and from pt0->pt2
// static double vector_cos(Point pt1, Point pt2, Point pt0)
// {
//   double dx1 = pt1.x - pt0.x;
//   double dy1 = pt1.y - pt0.y;
//   double dx2 = pt2.x - pt0.x;
//   double dy2 = pt2.y - pt0.y;
//   return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2));
// }

// void findSquares(Image_capsule &images, vector< vector<Point> > &contours)
// {
//   int pos_thresh = 3;

//   //approximation of sqaures and their properties
//   vector<Point> approx;
//   vector< vector<Point> > squares;
//   // test each contour to see if its a square
//   for (size_t i = 0; i < contours.size(); i++)
//   {
//     approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true) * 0.02, true);
//     if (approx.size() == 4 &&
//         fabs(contourArea(Mat(approx))) > 1000 &&
//         isContourConvex(Mat(approx)))
//     {
//       double maxCosine = 0.3;

//       for (int j = 2; j < 5; j++)
//       {
//         // find the maximum cosine of the angle between joint edges
//         double cosine = fabs(vector_cos(approx[j % 4], approx[j - 2], approx[j - 1]));
//         maxCosine = MAX(maxCosine, cosine);
//       }

//       if (maxCosine < 0.6)
//         squares.push_back(approx);
//     }
//   }
//   //go through the squares to find their size and draw them
//   for (size_t i = 0; i < squares.size(); i++)
//   {
//     const Point *p = &squares[i][0];
//     int n = (int)squares[i].size();

//     double cx = (p[2].x + p[0].x) / 2;
//     double cy = (p[2].y + p[0].y) / 2;
//     circle(images.frame, Point(cx, cy), 1, Scalar(0, 0, 0), 3, CV_AA);

//     double area = abs(p[2].x - p[0].x) * abs(p[2].y - p[0].y);
//     rects[i].cx = cx;
//     rects[i].cy = cy;
//     rects[i].area = area;
//     //printf("Square# %d, Center: X: %d Y: %d, Area: %d\n",(int) i,(int) cx,(int) cy,(int) area);
//     polylines(images.frame, &p, &n, 1, true, Scalar(255, 0, 0), 2, CV_AA);
//   }

//   //merge duplicate squares into one average
//   if (squares.size() > 1)
//   {
//     int c = 0;
//     for (size_t i = 0; i < squares.size(); i++)
//     {
//       for (size_t y = i + 1; y < squares.size(); y++)
//       {
//         if (i != y)
//         {
//           if (rects[i].cx < rects[y].cx + pos_thresh || rects[i].cx > rects[y].cx - pos_thresh)
//           {
//             if (rects[i].cy < rects[y].cy + pos_thresh || rects[i].cy > rects[y].cy - pos_thresh)
//             {
//               finalRects[c].cx = (rects[i].cx + rects[y].cx) / 2;
//               finalRects[c].cy = (rects[i].cy + rects[y].cy) / 2;
//               finalRects[c].area = (rects[i].area + rects[y].area) / 2 ;
//               printf("Square# %d, Center: X: %d Y: %d, Area: %d\n"
//                      , (int) i, (int) finalRects[i].cx, (int) finalRects[i].cy, (int) finalRects[i].area);
//               c++;
//             }
//           }
//         }
//       }
//     }
//   }
// }
