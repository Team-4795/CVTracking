#include <stdio.h>
#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

//image properties
int height,width,step,channels;
//HSV min max contraints
int lowH = 70;
int highH = 96;
  
int lowS = 56;
int highS = 255;
  
int lowV =142;
int highV = 255;

int thresh = 100;
//whileloop condition
bool running = true;
//this sets up the camera and allows us to get images from it
Size size;
//make our filtered images
Mat hsv_image;
Mat threshHold_image;
Mat contour_image;
//get the current image off the camera
Mat frame;
Scalar hsv_min;
Scalar hsv_max;
VideoCapture capture(1);

void findBoundingBox( vector< vector<Point> > contours);
void getContours(vector< vector<Point> > &contours,vector <Vec4i> hierarchy);
void findSqaures( vector< vector<Point> > contours);

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
static double angle( Point pt1, Point pt2, Point pt0 )
{
  double dx1 = pt1.x - pt0.x;
  double dy1 = pt1.y - pt0.y;
  double dx2 = pt2.x - pt0.x;
  double dy2 = pt2.y - pt0.y;
  return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

int main(int argc, char** argv )
{
  if(!capture.isOpened())
    {
      fprintf( stderr, "ERROR: capture is NULL \n" );
      getchar();
      return -1;
    }
  capture.set(CV_CAP_PROP_BRIGHTNESS,0);
  capture >> frame;
  //make all the windows needed
  namedWindow("RGB", WINDOW_AUTOSIZE );
  namedWindow("Thresh", WINDOW_AUTOSIZE);
  namedWindow("Control",WINDOW_AUTOSIZE);
  //get the camera image properties
  height = frame.size().height;
  width = frame.size().width;
  step = frame.step;
  size = Size(width,height);
  //make our filtered images
  hsv_image = Mat(size,CV_8UC3);
  threshHold_image = Mat(size,CV_8UC1);
  contour_image = Mat(size,CV_8UC1);
  //make trackbars to control the HSV min max values
  createTrackbar("lowH","Control",&lowH,255);
  createTrackbar("lowS","Control",&lowS,255);
  createTrackbar("lowV","Control",&lowV,255);

  createTrackbar("highH","Control",&highH,255);
  createTrackbar("highS","Control",&highS,255);
  createTrackbar("highV","Control",&highV,255);

  createTrackbar("ThreshHold","Control",&thresh,100);
  //convert our min max HSV values to scalar
  hsv_min = Scalar(lowH,lowS,lowV);
  hsv_max = Scalar(highH,highS,highV);

  //main loop
  while(running)
    {
      //get a fresh image from the camera
      capture >> frame;
      if(!capture.isOpened())
	{
	  fprintf( stderr, "ERROR: capture is NULL \n" );
	  getchar();
	  return -1;
	}
      //make sure the scalars are updated with the new HSV values
      Scalar hsv_min = Scalar(lowH,lowS,lowV);
      Scalar hsv_max = Scalar(highH,highS,highV);
      //filter to HSV and then the color picker filter
      cvtColor(frame,hsv_image,CV_BGR2HSV);
      inRange(hsv_image,hsv_min,hsv_max,threshHold_image);
      
      //find contours in the image
      vector< vector<Point> > contours;
      vector <Vec4i> hierarchy;
      
      getContours(contours,hierarchy);

      findBoundingBox(contours);
      
     
      //show the raw image and the filtered image
      imshow("RGB", frame);
      imshow("Thresh",threshHold_image);
      //check if ESC is pressed to exit the program;
      //cvReleaseMemStorage(&storage);
      if((cvWaitKey(10) & 255) == 27) break;
    }
  return 0;
}


void getContours(vector< vector<Point> > &contours,vector <Vec4i> hierarchy)
{
  //filter until only contours appear
  Canny(threshHold_image,contour_image,255,255,3);
  dilate(contour_image,contour_image, Mat(), Point(-1,-1));
  findContours(contour_image,contours,hierarchy,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,Point(0,0));
}

void findBoundingBox( vector< vector<Point> > contours)
{
  vector< vector<Point> > contours_poly(contours.size());
  vector<Rect> boundRect(contours.size());


  for(int i = 0;i < contours.size(); i++)
    {
      approxPolyDP(Mat(contours[i]),contours_poly[i],3,true);
      boundRect[i] = boundingRect( Mat(contours_poly[i]) );
    }
  for( int i = 0; i< contours.size(); i++ )
    {
      Scalar color = Scalar( 0, 0,0 );
      rectangle( frame, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
    }
}

void findSqaures( vector< vector<Point> > contours)
{

  //approximation of sqaures and their properties
  vector<Point> approx;
  vector< vector<Point> > squares;
  // test each contour to see if its a square
  for( size_t i = 0; i < contours.size(); i++ )
    {
      approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);
      if( approx.size() == 4 &&
	  fabs(contourArea(Mat(approx))) > 1000 &&
	  isContourConvex(Mat(approx)) )
	{
	  double maxCosine = 0.3;

	  for( int j = 2; j < 5; j++ )
	    {
	      // find the maximum cosine of the angle between joint edges
	      double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
	      maxCosine = MAX(maxCosine, cosine);
	    }

	  if( maxCosine < 0.6 )
	    squares.push_back(approx);
	}
    }
  //go through the squares to find their size and draw them
  for(size_t i = 0; i < squares.size(); i++)
    {
      const Point* p = &squares[i][0];
      int n = (int)squares[i].size();

      polylines(frame, &p, &n, 1, true, Scalar(0, 255, 0), 3, CV_AA);

      double cx = (p[2].x + p[0].x) / 2;
      double cy = (p[2].y + p[0].y) / 2;
      circle(frame, Point(cx, cy), 1, Scalar(0, 0, 0), 8, CV_AA);

      double area = abs(p[2].x - p[0].x) * abs(p[2].y - p[0].y);
      printf("Square# %d, Center: X: %d Y: %d, Area: %d\n",(int) i,(int) cx,(int) cy,(int) area);
    }
      
}

