/*************************************************************************
	> File Name: IM_server.c
	> Author: liucheng
	> Mail: 1035377294@qq.com 
	> Created Time: Mon 17 Mar 2014 04:02:52 PM CST
 ************************************************************************/

#include"IM.h"
#define LISTENQ 8
#define NUM_THREADS 5
struct nameList *head,*tail;
/*packet initial*/
int sockfdnum[NUM_THREADS];
int threads_tag[NUM_THREADS];
void packetInit(struct packet *pack,char service,
                char status,char *send_name,char *recv_name,char *data){
    pack->service=service;
    pack->status=status;
    if(send_name==NULL)
        memset(pack->send_name,0,sizeof(pack->send_name));
    if(recv_name==NULL)
        memset(pack->recv_name,0,sizeof(pack->recv_name));
    if(data==NULL)
        memset(pack->data,0,sizeof(pack->data));
    if(send_name!=NULL)
        strcpy(pack->send_name,send_name);
    if(recv_name!=NULL)
        strcpy(pack->recv_name,recv_name);
    if(data!=NULL)
        strcpy(pack->data,data);
}
/*charge name is or not being*/
bool chargeUsernameCanUse(char* name){
    struct nameList *p;
    p=head;
    while(p!=NULL){
        if(strcmp(p->name,name)==0)
            return false;
        else 
            p=p->next;
    }
    return true;
}
/*handle a name if it is to login*/
void login(char *name,int sockfd){
    struct packet sendline;
    struct nameList *newnode;
	struct nameList *p;
    if(head==NULL){
        head=(struct nameList*)malloc(sizeof(struct nameList));
        tail=head;
        strcpy(head->name,name);
        head->clifd=sockfd;
        head->next=NULL;
        packetInit(&sendline,SERVICE_LOGIN,STATUS_REPLY,name,NULL,"name_not_being");
        send(sockfd,&sendline,sizeof(sendline),0);
        printf("can use the name :%s\n",name);
    }
    else{
        if(chargeUsernameCanUse(name)){
            newnode=(struct nameList *)malloc(sizeof(struct nameList));
            strcpy(newnode->name,name);
            newnode->clifd=sockfd;
            newnode->next=NULL;
            tail->next=newnode;
            tail=tail->next;
            packetInit(&sendline,SERVICE_LOGIN,STATUS_REPLY,name,NULL,"name_not_being");
            send(sockfd,&sendline,sizeof(sendline),0);
            printf("can use the name :%s\n",name);
        }
        else{
            packetInit(&sendline,SERVICE_LOGIN,STATUS_REPLY,name,NULL,"name_being");
            send(sockfd,&sendline,sizeof(sendline),0);
            printf("can not use the name :%s\n",name);
        }
    }
	p=head;
	while(p!=NULL){
		printf("Tell %s , %s is online\n",p->name,name);
		packetInit(&sendline,SERVICE_LOGIN,STATUS_ONLINE,p->name,NULL,name);
		send(p->clifd,&sendline,sizeof(sendline),0);
		p=p->next;
	}
}
/*send all the username to someone*/
void sendAllUserName(int sockfd,char *name){
    struct nameList *p;
    struct packet sendline;
    p=head;
    char data[MAXDATA];
	char enter[2];
    memset(&enter,0,sizeof(enter));
    memset(&data,0,sizeof(data));
    enter[0]='\n';
    while(p!=NULL){
        strcat(data,p->name);
		strcat(data,enter);
        p=p->next;
    }
    printf("Ask for nameList:%s",data);
    packetInit(&sendline,SERVICE_SHOWNAMELIST,STATUS_REPLY,p->name,NULL,data);
    send(sockfd,&sendline,sizeof(sendline),0);
}

/*client logout*/
void logout(char *name){
    struct nameList *p,*q;
	p=head;
	struct packet pack;
	while(strcmp(p->name,name)!=0){
		q=p;
		p=p->next;
	}
    int i;
    //for(i=0;i<NUM_THREADS;i++){
    //    if(sockfdnum[i]==p->clifd)
    //        threads_tag[i]=0;
    //}
	if(p==head){
		head=head->next;
		free(p);
	}
	else if(p==tail&&tail!=head){
		tail=q;
		free(p);
		tail->next=NULL;
	}
	else{
		q->next=p->next;
		free(p);
	}
	p=head;
	while(p!=NULL){
		printf("Tell %s,%s is offline\n",p->name,name);
		packetInit(&pack,SERVICE_LOGOUT,STATUS_AWAY,p->name,NULL,name);
		send(p->clifd,&pack,sizeof(pack),0);
        sendAllUserName(p->clifd,p->name);
		p=p->next;
	}	
}
void resentMessage(char send_name[MAXNAME],char recv_name[MAXNAME],char data[MAXDATA]){
    struct nameList *p;
    struct packet sendline;
    p=head;
    if(strcmp(recv_name," ")){
        //printf("who is resent to %s",recv_name);
        while(p!=NULL){
            printf("is checking %s\n",p->name);
            if(strcmp(recv_name,p->name)==0){
                printf("find it\n");
                break;
            }
            else p=p->next;
        }
        if(p==NULL) return;
        printf("resent to %s data %s sockfd %d\n ",p->name,data,p->clifd);
        packetInit(&sendline,SERVICE_MESSAGE,STATUS_REPLY,send_name,recv_name,data);
        send(p->clifd,&sendline,sizeof(sendline),0);
    }
    else{
        printf("resent to all people from %s\n",send_name);
        while(p!=NULL){
            printf("name : %s ,data: %s\n",p->name,data);
            packetInit(&sendline,SERVICE_MESSAGE,STATUS_REPLY,send_name," ",data);
            send(p->clifd,&sendline,sizeof(sendline),0);
            p=p->next;
        }
    }
}
/*handle*/
void *handleClient(void *sockfd){
    struct packet sendline,recvline;
    int  n;
	while((n=recv((int)sockfd,&recvline,sizeof(recvline),0))>0){
        printf("%s\n","String received from and resent to the client:");
        printf("received from :%s\n",recvline.send_name);
        switch((char)(recvline.service)){
            case SERVICE_MESSAGE:{
                //printf("resent from %s to %s\n",recvline.send_name,recvline.recv_name);
                resentMessage(recvline.send_name,recvline.recv_name,recvline.data);
                break;
            }
            case SERVICE_LOGIN:{
                printf("handle login: %s\n",recvline.send_name);
                login(recvline.send_name,(int)sockfd);
                break;
            }
            case SERVICE_LOGOUT:{
                logout(recvline.send_name);
                break;
            }
            case SERVICE_SHOWNAMELIST:{
                sendAllUserName((int)sockfd,recvline.send_name);
                break;
            }
            case SERVICE_ASKISONLINE:{
                printf("ask if %s is online!!!\n",recvline.recv_name);
                if(!chargeUsernameCanUse(recvline.recv_name)){
                    printf("%s can chat with\n",recvline.recv_name);
                    packetInit(&sendline,SERVICE_ASKISONLINE,STATUS_ONLINE,recvline.send_name,NULL,NULL);
                    send((int)sockfd,&sendline,sizeof(sendline),0);
                }
                else{
                    printf("%s can not chat with\n",recvline.recv_name);
                    packetInit(&sendline,SERVICE_ASKISONLINE,STATUS_AWAY,recvline.send_name,NULL,NULL);
                    send((int)sockfd,&sendline,sizeof(sendline),0);
                }
                break;
            }
            default: printf("ERROR IN SERVICE!!!!!!\n");
        }
    }
	struct nameList *p;
	p=head;
	int i;
	for(i=0;i<NUM_THREADS;i++){
		if(sockfdnum[i]==(int)sockfd)
			threads_tag[i]=0;
	}
	while(p!=NULL){
		if(p->clifd==(int)sockfd)
			break;
		else p=p->next;
	}
	if(p!=NULL)
		logout(p->name);
}
int main(int argc,char **argv){
    pthread_t threads[NUM_THREADS];
	int listenfd,connfd,n;
    //int sockfdnum[NUM_THREADS];
	socklen_t clilen;
	struct packet sendline;
    //int threads_tag[NUM_THREADS];
	int rc,i=0;
    for(i=0;i<NUM_THREADS;i++)
        threads_tag[i]=0;
	struct sockaddr_in cliaddr,servaddr;
	if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("Problem in creating the socket");
		exit(2);
    }
    const int on = 1; 
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SERV_PORT);
	bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	listen(listenfd,LISTENQ);
	printf("%s\n","Server running...waiting for connecting.");
	while(1){
		clilen=sizeof(cliaddr);
		connfd=accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
		printf("%s\n","Received request...");
        for(i=0;i<NUM_THREADS;i++){
            if(threads_tag[i]==0){
		        threads_tag[i]=1;
                sockfdnum[i]=connfd;
                printf("Main: creating thread %1d\n",i);
		        packetInit(&sendline,SERVICE_TELLFULL,STATUS_NOTFULL,NULL,NULL,NULL);
				send(connfd,&sendline,sizeof(sendline),0);
				rc=pthread_create(&threads[i],NULL,handleClient,(void *)connfd);
                break;
            }
			if(i==NUM_THREADS-1){
				printf("can not Create Threads anymore\n");
				packetInit(&sendline,SERVICE_TELLFULL,STATUS_FULL,NULL,NULL,NULL);
				printf("send success\n");
				send(connfd,&sendline,sizeof(sendline),0);
				printf("send success\n");
				break;
			}
		}
		if(rc){
			printf("ERROR;return code from pthread_create() is %d\n",rc);
			exit(-1);
		
		}
	}
    pthread_exit(NULL);
}
