#!/bin/bash


g++  -std=c++0x Motion.cpp smtp/smtp.cpp smtp/base64.cpp pop3/pop3.cpp -o Motion `pkg-config opencv --cflags --libs`
