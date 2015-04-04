#!/usr/bin/python
# -*- coding: utf-8 -*-
#python2.7 mailtest.py


from smtplib import SMTP
from smtplib import SMTPRecipientsRefused
from poplib import POP3
from time import sleep 

#for attach img
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage



import sys 

smtpserver = 'smtp.163.com'
pop3server = 'pop.163.com'
email_addr = 'usr@163.com'
username = 'usr'
password = 'IWantToFly' 


#https://docs.python.org/2/library/smtplib.html
def send_mail():

	mail_attach = '1.jpg'

	msg = MIMEMultipart()
	msg["To"] = ''
	msg["From"] = ''
	msg["Subject"] = 'MY_CMD:1'

	msg_text = MIMEText('<img src="cid:1.jpg">' , 'html')   
	msg.attach(msg_text)

	fp = open(mail_attach, 'rb')                                                    
	img = MIMEImage(fp.read())
	fp.close()
	img.add_header('Content-ID', mail_attach)
	msg.attach(img)

	send_svr = SMTP(smtpserver)
	#send_svr.set_debuglevel(1)
	#print send_svr.ehlo()[0] #服务器属性等

	send_svr.login(username,password)
	try:
	   errs = send_svr.sendmail(email_addr,email_addr,msg.as_string())
	except SMTPRecipientsRefused:
	   print 'server refused....'
	   sys.exit(1)
	send_svr.quit()  
	assert len(errs) == 0,errs 
	print 'send end'



#https://docs.python.org/2/library/poplib.html
def recv_mail():
	recv_svr = POP3(pop3server)
	recv_svr.user(username)
	recv_svr.pass_(password)


	#msg_cnt is the last mail index, mail index is range(1, msg_cnt)
	msg_cnt, mailbox_size = recv_svr.stat()
	for i in range(1, msg_cnt):
		try:
			#only list msg header to reduce size, use retr to retrive the whole body
			rsp,msg,siz = recv_svr.top(i, 0)

			#print	msg
			for msg_one_entry in msg:
				if msg_one_entry.find('Subject:') == 0:
					#response to command here
					print msg_one_entry
		except:
			print '_______error'


#recv_mail()
send_mail()





