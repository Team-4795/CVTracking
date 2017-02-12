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
    printf("Termination due to abnormal signal: %d\n", signal);
    exit(1);
  }
}

void show_help(void)
{
  printf("CVTracking [-udih] [-c <camera index>] [-s <image path>]\n"
	 "  -u  User mode (camera view only)\n"
	 "  -d  Debug mode (camera, threshold, control views, settings sliders)\n"
	 "  -h  Show this help menu\n"
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
  sigaction(SIGHUP, &action, NULL);
  sigaction(SIGQUIT, &action, NULL);
  sigaction(SIGUSR1, &action, NULL);
  sigaction(SIGUSR2, &action, NULL);
  action.sa_handler = handle_signal;
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGSEGV, &action, NULL);
  sigaction(SIGPIPE, &action, NULL);
  sigaction(SIGTERM, &action, NULL);

  // parse command line arguments
  int arg;
  while ((arg = getopt(argc, argv, "udhis:c:")) != -1)
    switch (arg)
    {
    case 'u':
      settings["GUI"] = true;
      break;
    case 'd':
      settings["GUI"] = true;
      settings["debug"] = true;
      break;
    case 'c':
      settings["camera-index"] = (int) strtol(optarg, NULL, 10);
      break;
    case 's':
      settings["static-image"] = true;
      settings["static-path"] = optarg;
      break;
    case 'i':
      settings["streamed-image"] = true;
      break;
    case 'h':
    default:
      show_help();
      return (optopt == 'h' ? 0 : 1);
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
  contourData contour_data;
  
  context_t context(1);
  socket_t socket(context, ZMQ_PUB);

  socket.bind("tcp://*:5808");

  init(images, HSVs, settings);

  //main loop
  while (settings["running"].asBool())
  {
    //make sure the scalars are updated with the new HSV values
    HSVs.hsv_min = Scalar(settings["lowH"].asInt(), settings["lowS"].asInt(), settings["lowV"].asInt());
    HSVs.hsv_max = Scalar(settings["highH"].asInt(), settings["highS"].asInt(), settings["highV"].asInt());
    if(!settings["static-image"].asBool())
    {
      //get a fresh image from the camera
      capture >> images.frame;
      if (!capture.isOpened())
      {
	fprintf(stderr, "ERROR: capture is NULL \n");
	getchar();
	return -1;
      }
      
    }
    else if(settings["static-image"].asBool())
    {
      images.frame = imread("static_image.jpg",CV_LOAD_IMAGE_COLOR);
    }
    else if(settings["streamed-image"].asBool())
    {
      capture.open("axis-camera.local/mjpg/video.mjpg");
    }
    //filter to HSV and then the color picker filter
    cvtColor(images.frame, images.hsv_image, CV_BGR2HSV);
    inRange(images.hsv_image, HSVs.hsv_min, HSVs.hsv_max, images.threshHold_image);
    //find contours in the image
    vector< vector<Point> > contours;
    vector <Vec4i> hierarchy;
    getContours(images, contours, hierarchy);

    vector< vector<Point> > hull(contours.size());
    findConvexHull(images, contours, hull,contour_data);
    //findBoundingBox(images,contours);
    //findSquares(images,hull);

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
      snprintf(cmsg, sizeof(cmsg), "%.4f", contour_data.Angle);
      s_send(socket, string(cmsg));
    }
    sleep(0.1);

    //check if ESC is pressed to exit the program;
    if ((cvWaitKey(10) & 255) == 27)
      break;
  }

  handle_signal(0);
  return 0;
}

// XXX user is a const char * setting name, do not modify
static void trackbar_callback(int pos, void *user)
{
  const char *setting = static_cast<const char *>(user);

  settings[setting] = pos;
}

#define add_setting_slider(name, max_value) \
  createTrackbar(name, "Control", NULL, 255, trackbar_callback, const_cast<char *>(name)); \
  setTrackbarPos(name, "Control", settings[name].asInt())

void init(Image_capsule &images, HSV_capsule &HSVs, Settings &settings)
{
  //image properties
  int height, width;

  Size size;
  if(settings["streamed-image"].asBool())
  {
    if(!capture.open("http://axis-camera.local/mjpg/video.mjpg"))
    {
      printf("Cant open the axis-cam!\n");
    }
  }
  else
  {
    capture = VideoCapture(settings["camera-index"].asInt());
  }
  
  
  if (!capture.isOpened())
  {
    fprintf(stderr, "ERROR: capture is NULL \n");
    getchar();
    return;
  }
  
  capture >> images.frame;
  if (settings["GUI"].asBool())
  {
    //make all the windows needed
    namedWindow("RGB", WINDOW_AUTOSIZE);
    if (settings["debug"].asBool())
    {

      namedWindow("Thresh", WINDOW_AUTOSIZE);
      namedWindow("Control", WINDOW_AUTOSIZE);
    }

  }
  //get the camera image properties
  height = images.frame.size().height;
  width = images.frame.size().width;
  size = Size(width, height);
  //make our filtered images
  images.hsv_image = Mat(size, CV_8UC3);
  images.threshHold_image = Mat(size, CV_8UC1);
  images.contour_image = Mat(size, CV_8UC1);
  images.hull_image = Mat(size, CV_8UC1);
  if(settings["debug"].asBool()) {
    //make trackbars to control the HSV min max values
    add_setting_slider("lowH", 255);
    add_setting_slider("lowS", 255);
    add_setting_slider("lowV", 255);
    add_setting_slider("highH", 255);
    add_setting_slider("highS", 255);
    add_setting_slider("highV", 255);
  }
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

void findConvexHull(Image_capsule &images, vector< vector<Point> > &contours, vector<vector<Point> > &hull
                    , contourData data)
{
  for (size_t i = 0; i < contours.size(); i++)
    convexHull(Mat(contours[i]), hull[i], false);
  int count = 1;
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

      double cx = 400;
      double f = 476.7;
      data.Angle = atan((u - cx) / f);
      data.X = u;
      data.Y = v;
      data.Area = area;
      char cmsg[50];
      snprintf(cmsg, sizeof(cmsg), "%.4f", radian_to_degrees(data.Angle));
      //printf("target#%d: Area: %d X:%d Y:%d Angle: %f \n", count, area, u, v, radian_to_degrees(angle));
      putText(images.frame,cmsg,Point(u,v),FONT_HERSHEY_PLAIN,1.0,CV_RGB(255,255,0),2.0);
      count++;
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
