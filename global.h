#ifndef GLOBAL_H
#define GLOBAL_H




#define USER_NAME "admin"
#define MAIL_ACCOUNT	"admin@qq.com"
#define MAIL_PASSWORD	"admin"
#define SMTP_SERVER	"smtp.qq.com"
#define POP3_SERVER	"pop3.qq.com"


#define IMAGE 100
#define VIDEO 101


int pop3_main(void (*event_handler)(int));
int smtp_entry(const char *file_name);


#endif
