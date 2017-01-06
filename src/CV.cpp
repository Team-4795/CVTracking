#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char** argv )
{
  int height,width,step,channels;
  
  int lowH = 111;
  int highH = 157;
  
  int lowS = 129;
  int highS = 255;
  
  int lowV = 0;
  int highV = 203;
  
  bool running = true;
  
  CvCapture* capture = cvCaptureFromCAM(0);
  if( !capture )
    {
      fprintf( stderr, "ERROR: capture is NULL \n" );
      getchar();
      return -1;
    }
  IplImage* frame = cvQueryFrame(capture);
  
  namedWindow("RGB", WINDOW_AUTOSIZE );
  namedWindow("HSV", WINDOW_AUTOSIZE );
  namedWindow("Thresh", WINDOW_AUTOSIZE);
  namedWindow("Control",WINDOW_AUTOSIZE);
  
  height = frame->height;
  width = frame->width;
  step = frame->widthStep;
  CvSize size = cvSize(width,height);

  IplImage* hsv_image = cvCreateImage(size,IPL_DEPTH_8U,3);
  IplImage* threshHold_image = cvCreateImage(size,IPL_DEPTH_8U,1);

  cvCreateTrackbar("lowH","Control",&lowH,255);
  cvCreateTrackbar("lowS","Control",&lowS,255);
  cvCreateTrackbar("lowV","Control",&lowV,255);

  cvCreateTrackbar("highH","Control",&highH,255);
  cvCreateTrackbar("highS","Control",&highS,255);
  cvCreateTrackbar("highV","Control",&highV,255);
  
  CvScalar hsv_min = cvScalar(lowH,lowS,lowV);
  CvScalar hsv_max = cvScalar(highH,highS,highV);


  
  while(running)
    {
     CvScalar hsv_min = cvScalar(lowH,lowS,lowV);
     CvScalar hsv_max = cvScalar(highH,highS,highV);
      frame = cvQueryFrame(capture);
      if(!frame)
	{
	  fprintf(stderr,"ITS A (FRAME)TRAP");
	  getchar();
	  break;
	}
      cvShowImage("RGB", frame);
      
      cvCvtColor(frame,hsv_image,CV_BGR2HSV);
      cvInRangeS(hsv_image,hsv_min,hsv_max,threshHold_image);
      
      cvShowImage("HSV",hsv_image);
      cvShowImage("Thresh",threshHold_image);
      
      if((cvWaitKey(10) & 255) == 27) break;
    }
  return 0;
}
