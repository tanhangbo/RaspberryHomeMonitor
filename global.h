#ifndef GLOBAL_H
#define GLOBAL_H




#define USER_NAME "admin"
#define MAIL_ACCOUNT	"admin@qq.com"
#define MAIL_PASSWORD	"admin"
#define SMTP_SERVER	"smtp.qq.com"
#define POP3_SERVER	"pop3.qq.com"


#define KEY_ESC 27
#define DELAY_TIME 30
#define ERODE_TIMES 1
#define CAMERA_INDEX 0
#define RASPBERRY 1
#if RASPBERRY
	#define SHOW_IMAGE_WINDOW 0
	#define IMAGE_COUNT 100
#else
	#define SHOW_IMAGE_WINDOW 1
	#define VIDEO_COUNT 1000
#endif

#define IMAGE 100
#define VIDEO 101


int pop3_main(void (*event_handler)(int));
int smtp_entry(const char *file_name);


#endif
