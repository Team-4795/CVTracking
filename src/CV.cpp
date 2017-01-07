#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;

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
  CvCapture* capture = cvCaptureFromCAM(0);
  if( !capture )
    {
      fprintf( stderr, "ERROR: capture is NULL \n" );
      getchar();
      return -1;
    }
  //get the current image off the camera
  IplImage* frame = cvQueryFrame(capture);
  //make all the windows needed
  namedWindow("RGB", WINDOW_AUTOSIZE );
  namedWindow("Thresh", WINDOW_AUTOSIZE);
  namedWindow("Control",WINDOW_AUTOSIZE);
  //get the camera image properties
  height = frame->height;
  width = frame->width;
  step = frame->widthStep;
  CvSize size = cvSize(width,height);
  //make our filtered images
  IplImage* hsv_image = cvCreateImage(size,IPL_DEPTH_8U,3);
  IplImage* threshHold_image = cvCreateImage(size,IPL_DEPTH_8U,1);
  //make trackbars to control the HSV min max values
  cvCreateTrackbar("lowH","Control",&lowH,255);
  cvCreateTrackbar("lowS","Control",&lowS,255);
  cvCreateTrackbar("lowV","Control",&lowV,255);

  cvCreateTrackbar("highH","Control",&highH,255);
  cvCreateTrackbar("highS","Control",&highS,255);
  cvCreateTrackbar("highV","Control",&highV,255);
  
  //convert our min max HSV values to scalar
  CvScalar hsv_min = cvScalar(lowH,lowS,lowV);
  CvScalar hsv_max = cvScalar(highH,highS,highV);


  //main loop
  while(running)
    {
      //get a fresh image from the camera
      frame = cvQueryFrame(capture);
      
      //make sure the scalars are updated with the new HSV values
      CvScalar hsv_min = cvScalar(lowH,lowS,lowV);
      CvScalar hsv_max = cvScalar(highH,highS,highV);
      if(!frame)
	{
	  fprintf(stderr,"ITS A (FRAME)TRAP");
	  getchar();
	  break;
	}
      //filter to HSV and then the color picker filter
      cvCvtColor(frame,hsv_image,CV_BGR2HSV);
      cvInRangeS(hsv_image,hsv_min,hsv_max,threshHold_image);
      
      //find circles in the image
      
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
      
      //show the raw image and the filtered image
      cvShowImage("RGB", frame);
      cvShowImage("Thresh",threshHold_image);
      //check if ESC is pressed to exit the program;
      cvReleaseMemStorage(&storage);
      if((cvWaitKey(10) & 255) == 27) break;
    }
  return 0;
}
