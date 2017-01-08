#include <stdio.h>
#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;



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
  //image properties
  int height,width,step,channels;
  //HSV min max contraints
  int lowH = 170;
  int highH = 199;
  
  int lowS = 132;
  int highS = 255;
  
  int lowV = 0;
  int highV = 203;
  //whileloop condition
  bool running = true;
  //this sets up the camera and allows us to get images from it
  VideoCapture capture(0);
  if(!capture.isOpened())
    {
      fprintf( stderr, "ERROR: capture is NULL \n" );
      getchar();
      return -1;
    }
  //get the current image off the camera
  Mat frame;
  capture >> frame;
  //make all the windows needed
  namedWindow("RGB", WINDOW_AUTOSIZE );
  namedWindow("Thresh", WINDOW_AUTOSIZE);
  namedWindow("Control",WINDOW_AUTOSIZE);
  //get the camera image properties
  height = frame.size().height;
  width = frame.size().width;
  step = frame.step;
  Size size = Size(width,height);
  //make our filtered images
  Mat hsv_image = Mat(size,CV_8UC3);
  Mat threshHold_image = Mat(size,CV_8UC1);
  Mat contour_image = Mat(size,CV_8UC1);
  //make trackbars to control the HSV min max values
  createTrackbar("lowH","Control",&lowH,255);
  createTrackbar("lowS","Control",&lowS,255);
  createTrackbar("lowV","Control",&lowV,255);

  createTrackbar("highH","Control",&highH,255);
  createTrackbar("highS","Control",&highS,255);
  createTrackbar("highV","Control",&highV,255);

  //convert our min max HSV values to scalar
  Scalar hsv_min = Scalar(lowH,lowS,lowV);
  Scalar hsv_max = Scalar(highH,highS,highV);

  int thresh = 100;
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
      vector< vector<Point> > squares;
      vector <Vec4i> hierarchy;
      
      Canny(threshHold_image,contour_image,thresh,thresh*2,3);
      dilate(contour_image,contour_image, Mat(), Point(-1,-1));
      findContours(contour_image,contours,hierarchy,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,Point(0,0));
      
      vector<Point> approx;

      // test each contour
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

      for( size_t i = 0; i < squares.size(); i++ )
	{
	  const Point* p = &squares[i][0];
	  int n = (int)squares[i].size();
	  polylines(frame, &p, &n, 1, true, Scalar(0,255,0), 3, CV_AA);
	}
      //show the raw image and the filtered image
      imshow("RGB", frame);
      imshow("Thresh",threshHold_image);
      //check if ESC is pressed to exit the program;
      //cvReleaseMemStorage(&storage);
      if((cvWaitKey(10) & 255) == 27) break;
    }
  return 0;
}





//circle algorithim
/*
// Memory for hough circles
CvMemStorage* storage = cvCreateMemStorage(0);
// hough detector works better with some smoothing of the image
cvSmooth(threshHold_image,threshHold_image, CV_GAUSSIAN, 9, 9 );
//hough transform to detect circle
CvSeq* circles = cvHoughCircles(threshHold_image, storage, CV_HOUGH_GRADIENT, 2
,threshHold_image->height/4, 100, 50, 10, 400);
for (int i = 0; i < circles->total; i++) {
//get the parameters of circles detected
float* p = (float*)cvGetSeqElem(circles, i);
printf("Object! x=%f y=%f r=%f\n\r", p[0], p[1], p[2]);

// draw a circle with the centre and the radius obtained from the hough transform
cvCircle(frame, cvPoint(cvRound(p[0]), cvRound(p[1])), 
2, CV_RGB(255, 255, 255), -1, 8, 0);
cvCircle(frame, cvPoint(cvRound(p[0]), cvRound(p[1])), 
cvRound(p[2]), CV_RGB(0, 255, 0), 2, 8, 0);
}
*/
