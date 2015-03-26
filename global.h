#ifndef GLOBAL_H
#define GLOBAL_H




#define USER_NAME "admin"
#define MAIL_ACCOUNT	"admin@163.com"
#define MAIL_PASSWORD	"admin"
#define SMTP_SERVER	"smtp.163.com"
#define POP3_SERVER	"pop3.163.com"


#define IMAGE 100
#define VIDEO 101


int pop3_main(void (*event_handler)(int));
int smtp_entry(const char *file_name);


#endif
