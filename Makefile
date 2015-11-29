
CC := g++


all: Motion pop3_api smtp_api
	$(CC)  Motion.o pop3.o smtp.o base64.o $(shell pkg-config opencv --libs) -o Motion

Motion:Motion.cpp
	$(CC) -c -std=c++0x $(shell pkg-config opencv --cflags) Motion.cpp -o Motion.o

pop3_api: pop3/pop3.cpp
	$(CC)  -c pop3/pop3.cpp -o pop3.o

smtp_api: smtp/smtp.cpp smtp/base64.cpp
	$(CC)  -c smtp/smtp.cpp -o smtp.o
	$(CC)  -c smtp/base64.cpp -o base64.o

.PHONY: clean clean_file

clean:
	-rm *.o
	-rm Motion

clean_file:
	-rm record/jpg/*.jpg
	-rm record/avi/*.avi
	-rm *.zip -r


