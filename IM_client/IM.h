/*************************************************************************
	> File Name: IM.h
	> Author: liucheng
	> Mail: 1035377294@qq.com 
	> Created Time: Sun 16 Mar 2014 01:26:38 PM CST
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>
#include<stdbool.h>
#define MAXNAME 64
#define MAXDATA 4096
#define SERV_PORT 6666
/*define of service*/
#define SERVICE_MESSAGE 0x00
#define SERVICE_LOGIN 0x01
#define SERVICE_LOGOUT 0x02
#define SERVICE_SHOWNAMELIST 0x03
#define SERVICE_ASKISONLINE 0x04
#define SERVICE_TELLFULL 0x05
/*define of status*/
#define STATUS_ONLINE 0x00
#define STATUS_AWAY 0x01
#define STATUS_REQUEST 0x02
#define STATUS_REPLY 0x03
#define STATUS_FULL 0x04
#define STATUS_NOTFULL 0x05
/*define of struct packet*/
struct packet{
    char service;
    char status;
    char send_name[MAXNAME];
    char recv_name[MAXNAME];
    char data[MAXDATA];
};
struct nameList{
    char name[MAXNAME];
    int clifd;
    struct nameList *next;
};
struct send_thread_data{
	int sockfd;
	char send_name[MAXNAME];
	char recv_name[MAXNAME];
};
struct receive_thread_data{
	int sockfd;
};

