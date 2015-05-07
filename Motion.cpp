#include <cv.h>
#include <highgui.h>
#include <pthread.h>

#include <iostream>
#include <thread>
#include <mutex>

#include "global.h"

#include <queue>
using namespace std;  



queue<IplImage *> webcam_buf;

int push_count = 0;
int get_count = 0;

char video_path[50] = {0};
bool network_busy = false;

bool send_one_image = false;
bool send_one_video = false;

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



void* pop3_thread(void *arg)
{
	pop3_main(&pop3_handlee);
}




/*
void* play_sound(void *arg)
{
	while (true) {
		if (have_event) {
			system("aplay BUZZ4.WAV");
			sleep(65);
		}
		sleep(1);//1 second
	}
	return NULL;
}

*/





IplImage *frame_gray1 = NULL;
IplImage *frame_gray2 = NULL;
IplImage *frame_gray3 = NULL;
IplImage *frame_result1 = NULL;
std::mutex frame_lock;


enum cur_state {
	INIT,
	NORMAL,
	DETECTED,
	RECORDING,
	END_REC,

};

//todo:1.检查内存泄露
//using cvsaveimage at a fixed interval and compress to zip

	//cvCopy( const CvArr* src, CvArr* dst
bool detect_motion(IplImage *frame1, IplImage *frame2, IplImage *frame3)
{
	int black_pixels = 0;


	//printf("ENTER\n");
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
	//printf(">%d\n", black_pixels);
	if (black_pixels > 1000) {
		printf("MOTION_%d!\n", black_pixels);
		return true;
	}
	return false;
}

#if !RASPBERRY
CvVideoWriter *create_video(CvVideoWriter **video_writer, IplImage *frame)
{

	time_t current_time;
	struct tm * time_info;
	memset(video_path, 0, sizeof(video_path));
	time(&current_time);
	time_info = localtime(&current_time);
	strftime(video_path, 50, "./record/avi/%m%d_%H%M%S.avi", time_info);
	printf("creating video:%s\n", video_path);
	*video_writer =  cvCreateVideoWriter(video_path,
				CV_FOURCC('M', 'J', 'P', 'G'), 25.0, cvGetSize(frame));

}
#endif


void create_command_string(char *command, int command_size, char* file_name, int file_name_size)
{
	time_t current_time;
	struct tm * time_info;
	memset(command, 0, command_size);
	memset(file_name, 0, file_name_size);
	time(&current_time);
	time_info = localtime(&current_time);
	strftime(file_name, file_name_size, "%m%d_%H%M%S.zip", time_info);
	sprintf(command, "zip %s ./record/jpg/*", file_name);
}


void* dispatch_thread(void *arg)
{




	IplImage *frame = NULL;

	IplImage *frame1 = NULL;
	IplImage *frame2 = NULL;
	IplImage *frame3 = NULL;
	CvVideoWriter *video_writer = NULL;

	int video_count = 0;
	int image_count = 0;
	int init_step = 0;

	cur_state state = INIT;

	
	printf("video detect thread starts\n");

	while (true) {

		//printf("size=%d\n", webcam_buf.size());
		frame_lock.lock();
		if(!webcam_buf.empty()) {
			frame = webcam_buf.front();
			webcam_buf.pop();
			get_count++;
			//printf("%d > %d\n", push_count, get_count);
		}
		if (webcam_buf.size() > 100) {
			printf("your device is too slow, empty buffer!!\n");
			while (webcam_buf.size() > 0)
				webcam_buf.pop();
		}
		frame_lock.unlock();

		if (NULL == frame) {
			usleep(1000*5);
			continue;
		}


		//void cvCopy( const CvArr* src, CvArr* dst, const CvArr* mask=NULL );
		if (init_step < 3) {
			if (0 == init_step) {
				frame1 =  cvCreateImage(cvGetSize(frame),frame->depth,3);
				frame2 =  cvCreateImage(cvGetSize(frame),frame->depth,3);
				frame3 =  cvCreateImage(cvGetSize(frame),frame->depth,3);
				frame_gray1 = cvCreateImage(cvGetSize(frame),frame->depth,1);
				frame_gray2 = cvCreateImage(cvGetSize(frame),frame->depth,1);
				frame_gray3 = cvCreateImage(cvGetSize(frame),frame->depth,1);
				frame_result1 = cvCreateImage(cvGetSize(frame),frame->depth,1);	

				cvCopy(frame, frame1);
			} else if (1 == init_step) {
				cvCopy(frame, frame2);
			} else if (2 == init_step) {
				cvCopy(frame, frame3);
				state = NORMAL;
			}

			init_step++;
		} else {

			//cur_frame -> frame3 -> frame2 -> frame1
			cvCopy(frame2, frame1);
			cvCopy(frame3, frame2);
			cvCopy(frame, frame3);
				
		}



		switch(state) {
		case INIT:
			//printf("init state!\n");
		break;

		case NORMAL:
			if(detect_motion(frame1, frame2, frame3)) {
				printf("detected!!!\n");
				state = DETECTED;
			}

			if (send_one_image) {
				send_one_image = false;
				network_busy = true;
				if(!cvSaveImage("./record/jpg/IMAGE.jpg", frame))
					printf("Could not save: %s\n","IMAGE.jpg");
				else
					printf("Saving: %s\n","IMAGE.jpg");
				smtp_entry("./record/jpg/IMAGE.jpg");
				network_busy = false;
			}

		break;

		case DETECTED:

			//1. get current picture and send
			network_busy = true;
			if(!cvSaveImage("./record/jpg/XIMAGE.jpg", frame))
				printf("Could not save: %s\n","XIMAGE.jpg");
			else
				printf("Saving: %s\n","XIMAGE.jpg");

			smtp_entry("./record/jpg/XIMAGE.jpg");
			network_busy = false;
#if !RASPBERRY
			create_video(&video_writer, frame);
#endif

			state = RECORDING;
		break;

		case RECORDING:

			//1. get current picture with an interval
#if RASPBERRY
			if (image_count++ < IMAGE_COUNT) {
#else
			if (video_count % 20 == 0) {
#endif
				char image_path[100];
				sprintf(image_path, "./record/jpg/IMAGE_%d.jpg", video_count);
				if(!cvSaveImage(image_path, frame))
					printf("Could not save: %s\n",image_path);
			} 
#if RASPBERRY
			else {
				image_count = 0;
				state = END_REC;
			}
#endif

#if !RASPBERRY
			//2. record 1 minute video then stop and rejudge and recode
			if (video_count < VIDEO_COUNT) {
				//printf("V:%d --%d>%d\n", video_count, push_count, get_count);
				cvWriteFrame(video_writer, frame);
				video_count++;
			} else {
				video_count = 0;
				state = END_REC;
			}
#endif

		break;

		case END_REC:
#if !RASPBERRY
			if (video_writer) {
				printf("stop record\n");
				cvReleaseVideoWriter(&video_writer);
			}
#endif

			char zip_command[50];
			char zip_file_name[50];
			create_command_string(zip_command, sizeof(zip_command), zip_file_name, sizeof(zip_file_name));
			printf("executing command %s\n...", zip_command);
			network_busy = true;
			system(zip_command);
			smtp_entry(zip_file_name);
			network_busy = false;

			state = NORMAL;
		break;

		default:
			printf("switch not process\n");
		break;

		}




		//free the frame
		cvReleaseImage(&frame);
		frame = NULL;

	}


	return NULL;
}


/*

注意： cvQueryFrame返回的指针总是指向同一块内存。建议cvQueryFrame后拷贝一份。
而且返回的帧需要FLIP后才符合OPENCV的坐标系。 若返回值为NULL，说明到了视频的最后一帧。

*/
int main(int argc,char **argv)
{

	int err = 0;
	pthread_t tid[10];

#if SHOW_IMAGE_WINDOW
	cvNamedWindow("camera_player", CV_WINDOW_AUTOSIZE);
#endif

	IplImage *frame = NULL;
	CvCapture *capture = cvCreateCameraCapture(CAMERA_INDEX);
	assert(NULL != capture);

	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, 640);
	cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, 480);



	err = pthread_create(&(tid[4]), NULL, &pop3_thread, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));

	err = pthread_create(&(tid[5]), NULL, &dispatch_thread, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));

	int query = 0;
	while (true) {

#if RASPBERRY
		usleep(1000*500); /* 2pics/s */
		while (network_busy)
			usleep(1000*500);
#endif
		frame = cvQueryFrame(capture);

		if (!frame) {
			printf("\n critical error when capture\n");
			break;
		}

		/* enqueue a frame, must be copied before enqueued */
		frame_lock.lock();
		IplImage *frame_copy = cvCreateImage(cvGetSize(frame),frame->depth,3);
		cvCopy(frame, frame_copy);
		webcam_buf.push(frame_copy);
		frame_lock.unlock();
		push_count++;
#if SHOW_IMAGE_WINDOW
		cvShowImage("camera_player", frame);
#endif

		char c = cvWaitKey(DELAY_TIME);
		if (KEY_ESC == c)
			break;

	}

	//do not forget to release the resource
	cvReleaseCapture(&capture);
#if SHOW_IMAGE_WINDOW
	cvDestroyWindow("camera_player");
#endif
	return 0;
}
