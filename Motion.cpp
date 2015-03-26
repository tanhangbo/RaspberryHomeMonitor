#include <cv.h>
#include <highgui.h>
#include <pthread.h>

#include <iostream>
#include <thread>
#include <mutex>

#include "global.h"


#define KEY_ESC 27
#define DELAY_TIME 30
#define ERODE_TIMES 1
#define CAMERA_INDEX 1


bool have_event = false;


std::mutex frame_lock;

IplImage *frame;
IplImage *frame1;
IplImage *frame2;
IplImage *frame3;
IplImage *frame_gray1;
IplImage *frame_gray2;
IplImage *frame_gray3;
IplImage *frame_result1;

bool send_one_image = false;
bool send_one_video = false;



//note:#define cvCaptureFromCAM cvCreateCameraCapture





void pop3_handlee(int remote_command)
{
	//process command here
	printf("I want to handle %d\n", remote_command);
	if (remote_command == 100) {//image
		send_one_image = true;
	} else if (remote_command == 101) {
		send_one_video = true;
	}
}


void* camera_present_thread(void *arg)
{
	while (true) {
		/*CvCapture *capture = cvCreateCameraCapture(CAMERA_INDEX);
		if (NULL == capture)
			printf("unplugd!!!\n");
		else
			cvReleaseCapture(&capture); 
		*/
		sleep(1);
	}
}


void* pop3_thread(void *arg)
{
	pop3_main(&pop3_handlee);
}


//http://www.thegeekstuff.com/2012/04/create-threads-in-linux/
//todo : need free resource
void* play_sound(void *arg)
{
	while (true) {
		if (have_event) {
			system("aplay ALARM8.WAV");
			sleep(65);
		}
		sleep(1);//1 second
	}
	return NULL;
}



//buffer disable have_event
void* event_thread(void *arg)
{
	while (true) {
		if (have_event) {
			frame_lock.lock();
			if(!cvSaveImage("XIMAGE.jpg", frame))
				printf("Could not save: %s\n","XIMAGE.jpg");			
			frame_lock.unlock();
			smtp_entry("./XIMAGE.jpg");

			sleep(60);//1 minute
			have_event = false;
		}
		if (send_one_image) {
			send_one_image = false;
			frame_lock.lock();
			if(!cvSaveImage("IMAGE.jpg", frame))
				printf("Could not save: %s\n","IMAGE.jpg");			
			frame_lock.unlock();
			smtp_entry("./IMAGE.jpg");
		}
		sleep(1);//1 second
	}

	return NULL;
}



bool detect_motion()
{
	int black_pixels = 0;

	cvCopy(frame2, frame1);
	cvCopy(frame3, frame2);
	cvCopy(frame, frame3);

	//Failed to load OpenCL runtime 
	cvCvtColor(frame1, frame_gray1, CV_BGR2GRAY);
	cvCvtColor(frame2, frame_gray2, CV_BGR2GRAY);
	cvCvtColor(frame3, frame_gray3, CV_BGR2GRAY);

	cvAbsDiff(frame_gray1 ,frame_gray3, frame_gray1);// |gray1 - gray3| -> gray1
	cvAbsDiff(frame_gray2 ,frame_gray3, frame_gray2);// |gray2 - gray3| -> gray2

	cvAnd(frame_gray1, frame_gray2, frame_result1);
	/* draw the result, and can see the motion */
        cvThreshold(frame_result1, frame_result1, 35, 255, CV_THRESH_BINARY);


	//http://blog.csdn.net/futurewu/article/details/10047409
	//消除较小独点如噪音
	cvErode(frame_result1, frame_result1, 0, ERODE_TIMES);
		
	black_pixels = cvCountNonZero(frame_result1);
	if (black_pixels > 1000) {
		printf("MOTION_%d!\n", black_pixels);
		return true;
	}
	return false;
}

/*

注意： cvQueryFrame返回的指针总是指向同一块内存。建议cvQueryFrame后拷贝一份。
而且返回的帧需要FLIP后才符合OPENCV的坐标系。 若返回值为NULL，说明到了视频的最后一帧。

*/
int main(int argc,char **argv)
{

	int err = 0;
	pthread_t tid[10];


	cvNamedWindow("camera_player", CV_WINDOW_AUTOSIZE);
	CvCapture *capture = cvCreateCameraCapture(CAMERA_INDEX);
	assert(NULL != capture);


	frame = cvQueryFrame(capture);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, 640);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, 480);

	frame = cvQueryFrame(capture);
	frame1 =  cvCreateImage(cvGetSize(frame),frame->depth,3);
	frame2 =  cvCreateImage(cvGetSize(frame),frame->depth,3);
	frame3 =  cvCreateImage(cvGetSize(frame),frame->depth,3);

	frame_gray1 = cvCreateImage(cvGetSize(frame),frame->depth,1);
	frame_gray2 = cvCreateImage(cvGetSize(frame),frame->depth,1);
	frame_gray3 = cvCreateImage(cvGetSize(frame),frame->depth,1);
	frame_result1 = cvCreateImage(cvGetSize(frame),frame->depth,1);

	CvVideoWriter *video_writer = NULL;

	frame = cvQueryFrame(capture);
	cvCopy(frame, frame1);
	frame = cvQueryFrame(capture);
	cvCopy(frame, frame2);
	frame = cvQueryFrame(capture);
	cvCopy(frame, frame3);


	err = pthread_create(&(tid[0]), NULL, &play_sound, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));
	else
		printf("\n Thread created successfully\n");

	err = pthread_create(&(tid[1]), NULL, &event_thread, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));
	else
		printf("\n Thread created successfully\n");

	err = pthread_create(&(tid[2]), NULL, &pop3_thread, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));
	else
		printf("\n Thread created successfully\n");

	err = pthread_create(&(tid[3]), NULL, &camera_present_thread, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));
	else
		printf("\n Thread created successfully\n");




	while (true) {
		frame_lock.lock();
		frame = cvQueryFrame(capture);
		frame_lock.unlock();

		if (!frame) {
			printf("\n critical error when capture\n");
			break;
		}


		if (!have_event) {
			/* if have_event , detect 60s later */
			have_event = detect_motion();
		}



		//todo :get it into a thread to make it record
		//every 1minute to avoid attachment too big
		/* write video */
		if (have_event) {
			if(NULL == video_writer) {

				time_t current_time;
				struct tm * time_info;
				char timeString[50] = {0};
				time(&current_time);
				time_info = localtime(&current_time);
				strftime(timeString, 50, "%m%d_%H%M%S.avi", time_info);
				printf("creating video:%s\n", timeString);
				video_writer =  cvCreateVideoWriter(timeString,
					CV_FOURCC('M', 'J', 'P', 'G'), 25.0, cvSize(640, 480));
			}
			cvWriteFrame(video_writer, frame);
		} else {
			if (video_writer)
				cvReleaseVideoWriter(&video_writer);
		}



		cvShowImage("camera_player",frame);

		//usleep(1000*DELAY_TIME);
		//must need it to display
		char c = cvWaitKey(DELAY_TIME);
		if (KEY_ESC == c)
			break;

	}

	//do not forget to release the resource

	cvReleaseCapture(&capture);
	cvDestroyWindow("camera_player");
	return 0;
}
