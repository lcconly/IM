/*************************************************************************
	> File Name: IM_client.c
	> Author: liucheng
	> Mail: 1035377294@qq.com 
	> Created Time: Sun 16 Mar 2014 02:03:54 PM CST
 ************************************************************************/

#include<ncurses.h>
#include<assert.h>
#include"IM.h"
WINDOW *local_win_left;
WINDOW *local_win_up;
WINDOW *local_win_down;
WINDOW *local_win_right;
int row,col;
int linetag=1;
//getmaxyx(stdscr,row,col);
bool cantalk=false;
/*packet initial*/
void packetInit(struct packet *pack,char service ,
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
    //printf("copy name\n");
    if(recv_name!=NULL)
        strcpy(pack->recv_name,recv_name);
    if(data!=NULL)
        strcpy(pack->data,data);
    //printf("copy data\n");
}
/*charge username is being*/
bool creatUserName(int sockfd,char *name){
    //printf("Please enter a name to login:");
    mvwprintw(local_win_down,1,1,"Please enter a name to login:");
    struct packet sendline,recvline;
    //wrefresh(local_win_down);
    mvwscanw(local_win_down,2,1,"%64s%*[^\n]",name);
    //printf("name : %s\n",name);
    wrefresh(local_win_down);
    wrefresh(local_win_left);
    wrefresh(local_win_up);
    if(!strcmp(name,"\0"))
        return false;
    packetInit(&sendline,SERVICE_LOGIN,STATUS_REQUEST,name,NULL,NULL);
    //printf("send %s\n",sendline.send_name);
    send(sockfd,&sendline,sizeof(sendline),0);
    //printf("send success!!\n");
    //memset(&recvline,0,sizeof((struct packet*)recvline));
    //printf("memset success!!!!!|n");
    recv(sockfd,&recvline,sizeof(recvline),0);
    //printf("recv %s\n",recvline.data);
    if(strcmp(recvline.data,"name_being")==0)
        return false;
    else if(strcmp(recvline.data,"name_not_being")==0)
        return true;
    return false;
}
/*Function of showing all users*/
void show_user_list(struct send_thread_data *send_data){
    struct packet sendline;
    packetInit(&sendline,SERVICE_SHOWNAMELIST,STATUS_REQUEST,send_data->send_name,NULL,NULL);
    //printf("send for asking usernameList:%s\n",send_data->send_name);
    send(send_data->sockfd,&sendline,sizeof(sendline),0);
    usleep(100000);
}
/*handle receive*/
void *receive_handle(void *threadarg){
    struct receive_thread_data *recv_data;
    struct packet recvline;
    recv_data=(struct receive_thread_data *)threadarg;
    //char buf[MAXDATA];
    while(1){
        if(recv(recv_data->sockfd,&recvline,sizeof(recvline),0)==0)
            continue;
        if(recvline.service==SERVICE_MESSAGE&&recvline.status==STATUS_REPLY){
            //char buf[MAXDATA];
            //printf("%s : %s\n",recvline.send_name,recvline.data);
            if(linetag==(2*row/3-1)){
                linetag=1;
                wclear(local_win_up);
                wborder(local_win_up,'|','|','-','-','+','+','+','+');
            }
			if(strcmp(recvline.recv_name," "))
                mvwprintw(local_win_up,linetag,1,"%s:%s",recvline.send_name,recvline.data);
            else 
                mvwprintw(local_win_up,linetag,1,"%s to ALL:%s",recvline.send_name,recvline.data);
            linetag++;
            wrefresh(local_win_up);
            //sprintf(buf,"notify-send %s:%s\0-t\01",recvline.send_name,recvline.data);
            //printf("%s",buf);
            //system(buf);
        }
        else if(recvline.service==SERVICE_SHOWNAMELIST){
            //printf("\nAll the user online is list follow:\n");
            //printf("%s",recvline.data);
            wclear(local_win_left);
            wborder(local_win_left,' ','|',' ',' ',' ','+',' ','+');
            mvwprintw(local_win_left,1,1,"NameList:");
            int line=2;
            char *namelist;
            namelist=strtok(recvline.data,"\n");
            while(namelist!=NULL){
                //namelist=strtok(NULL,"\n");
                mvwprintw(local_win_left,line,1,"%s",namelist);
                namelist=strtok(NULL,"\n");
                line++;
            }
            wrefresh(local_win_left);
        }
        else if(recvline.service==SERVICE_LOGIN){
            //char temp[MAXDATA];
            //printf("%s is online!\n",recvline.data);
            //sprintf(buf,"notify-send %s_is_online\0-t\01",recvline.data);
            //system(buf);
            if(linetag==(2*row/3-1)){
                linetag=1;
                wclear(local_win_up);
                wborder(local_win_up,'|','|','-','-','+','+','+','+');
            }
            mvwprintw(local_win_up,linetag,1,"%s is on line",recvline.data);
            wrefresh(local_win_up);
            linetag++;
        }
        else if(recvline.service==SERVICE_LOGOUT){
            //printf("%s is offline!\n",recvline.data);
            //sprintf(buf,"notify-send %s_is_offline\0-t\01",recvline.data);
            //system(buf);
            if(linetag==(2*row/3-1)){
                linetag=1;
                wclear(local_win_up);
                wborder(local_win_up,'|','|','-','-','+','+','+','+');
            }
            mvwprintw(local_win_up,linetag,1,"%s is off line",recvline.data);
            wrefresh(local_win_up);
        }
        else if(recvline.service==SERVICE_ASKISONLINE&&recvline.status==STATUS_ONLINE){
            //assert(!cantalk);   
            cantalk=true;
        }
        else if(recvline.service==SERVICE_ASKISONLINE&&recvline.status==STATUS_AWAY)
            cantalk=false;
    }
}
/*Function of chatting with someone*/
void chat_with_one(struct send_thread_data *send_data){
    struct packet sendline,recvline;
    show_user_list(send_data);
    char name[MAXNAME];
    memset(&name,0,sizeof(name));
    char msg[MAXDATA];
    //printf("name is %s\n",send_data->send_name);
    //printf("choose one person to chat with:");
    wclear(local_win_down);
    wborder(local_win_down,'|',' ','-',' ','+',' ','+',' ');
    mvwprintw(local_win_down,1,1,"choose one person to chat with:");
    //wrefresh(local_win_down);
    mvwscanw(local_win_down,2,2,"%64s%*[^\n]",name);
    wrefresh(local_win_down);
    //sendline->service=SERVICE_ASKISONLINE;
    //sendline->status=STATUS_REQUEST;
    //sendline->send_name=send_data->send_name;
    //sendline->recv_name=name;
    packetInit(&sendline,SERVICE_ASKISONLINE,STATUS_REQUEST,send_data->send_name,name,NULL);
    send(send_data->sockfd,&sendline,sizeof(sendline),0);
    //printf("recv!!!!!!\n");
    //if(recv(send_data->sockfd,&recvline,sizeof(recvline),0)<0){
    //    perror("The server terminated prematurely");
    //    return;
    //}
    //printf("recv success!!!!\n");
    usleep(100000);
    if(!cantalk){
        //printf("user is not online\n");
        //assert(!cantalk);
        if(linetag==(2*row/3-1)){
            linetag=1;
            wclear(local_win_up);
            wborder(local_win_up,'|','|','-','-','+','+','+','+');
        }
        mvwprintw(local_win_up,linetag,1,"user is not online");
        wrefresh(local_win_up);
		return;
	}
    else if(cantalk){
        memset(&msg,0,sizeof(msg));
        while(1){
            //printf("%s : ",((struct send_thread_data *)send_data)->send_name);
			memset(&msg,0,sizeof(msg));
			wclear(local_win_down);
   		    wborder(local_win_down,'|',' ','-',' ','+',' ','+',' ');
    	    mvwprintw(local_win_down,1,1,"You are chatting with %s",name);
     	    //printf("You are chatting with %s\n",name);
     	    wrefresh(local_win_down);            
			mvwprintw(local_win_down,2,1,"%s:",
                      ((struct send_thread_data*)send_data)->send_name);
            //if(Get_Key()==12) break;
			wrefresh(local_win_down);
            if(getchar()=='')
				return;
			mvwscanw(local_win_down,2,
                     strlen(((struct send_thread_data*)send_data)->send_name)+2,"%[^\n]",msg);
            //if(!strcmp(msg,""))
            //    return;
			//char temp;
			//int len;
			//while(true){
			//	len=strlen(msg);
			//	temp=getchar();
			//	if(temp!='\n'&&temp!=EOF)
			//		msg[temp]=temp;
			//}
            wrefresh(local_win_down);
			if(!strcmp(msg ,""))
				continue;
            if(linetag==2*row/3-1){
                linetag=1;
                wclear(local_win_up);
                wborder(local_win_up,'|','|','-','-','+','+','+','+');
            }
            mvwprintw(local_win_up,linetag,1,"%s:%s",
                     ((struct send_thread_data*)send_data)->send_name,msg);
            linetag++;
            wrefresh(local_win_up);
            //wrefresh(local_win_down);
            //fgets(msg,MAXDATA,stdin);
            //msg[strlen(msg)-1]='\0';
            packetInit(&sendline,SERVICE_MESSAGE,STATUS_REQUEST,send_data->send_name,name,msg);
            send(send_data->sockfd,&sendline,sizeof(sendline),0);
			//if(Get_Key()==12){
				//assert(0);
            //    return;
            //}
			//if(getchar()=='')
			//	return;
        }
    }
}
/*Function of chatting with all people*/
void chat_with_all(struct send_thread_data *send_data){
    struct packet sendline;
    //printf("The mssage will send to all people:\n");
    //show_user_list(send_data);
    char msg[MAXDATA];
    memset(&msg,0,sizeof(msg));
    while(1){
        //printf("%s: ",send_data->send_name);
	    memset(&msg,0,sizeof(msg));
		wclear(local_win_down);
	    wborder(local_win_down,'|',' ','-',' ','+', ' ','+',' ');
	    mvwprintw(local_win_down,1,1,"The message will send to all people:");
        mvwprintw(local_win_down,2,1,"%s:",send_data->send_name);
        //wrefresh(local_win_down);
        wrefresh(local_win_down);
        if(getchar()=='')
			return;
		wrefresh(local_win_down);
		mvwscanw(local_win_down,2,strlen(send_data->send_name)+2,"%[^\n]",msg);
        //if(!strcmp(msg,""))
        //   return;
		//char temp;
		//int len;
		//while(true){
		//	len=strlen(msg);
		//	temp=getchar();
		//	if(temp!='\n'&&temp!=EOF)
		//		msg[temp]=temp;
		//}
        wrefresh(local_win_down);
		if(!strcmp(msg,""))
			continue;
        if(linetag==(2*row/3-1)){
            linetag=1;
            wclear(local_win_up);
            wborder(local_win_up,'|','|','-','-','+','+','+','+');
        }
        mvwprintw(local_win_up,linetag,1,"%s:%s",send_data->send_name,msg);
        //fgets(msg,MAXNAME,stdin);
        //msg[strlen(msg)-1]='\0';
        wrefresh(local_win_up);
        packetInit(&sendline,SERVICE_MESSAGE,STATUS_REQUEST,send_data->send_name," ",msg);
        send(send_data->sockfd,&sendline,sizeof(sendline),0);
		//if(Get_Key()==12)
		//	break;
		//if(getchar()=='')
		//	return;
	}
}
/*Function of logout*/
void logout(struct send_thread_data *send_data){
    struct packet sendline;
    packetInit(&sendline,SERVICE_LOGOUT,STATUS_REQUEST,send_data->send_name,NULL,NULL);
    send(send_data->sockfd,&sendline,sizeof(sendline),0);
}
/*handle send*/
void *send_handle(void *threadarg){
    //char func='0';
    struct send_thread_data *send_data;
    char func[2];
    memset(&func,0,sizeof(func));
    send_data=(struct send_thread_data *)threadarg;
    //system("clear");
    while(1){
        //printf("Hello,%s\n",send_data->send_name);
        //printf("Function:\n");
        //printf("1.chat with one person;\n");
        //printf("2.chat with all the people online;\n");
        //printf("3.show all the people online;\n");
        //printf("4.logout;\n");
		memset(&func,0,sizeof(func));
		wclear(local_win_down);
        wclear(local_win_up);
        wborder(local_win_up,'|','|','-','-','+','+','+','+');
        wborder(local_win_down,'|',' ','-',' ','+',' ','+',' ');
        mvwprintw(local_win_right,1,1,"Hello,%s",send_data->send_name);
        mvwprintw(local_win_right,2,1,"Function");
        mvwprintw(local_win_right,3,1,"1.chat with one");
        mvwprintw(local_win_right,4,1,"2.chat with all");
        mvwprintw(local_win_right,5,1,"3.update namelist");
        mvwprintw(local_win_right,6,1,"4.logout");
        mvwprintw(local_win_right,7,1,"input  to return");
        mvwprintw(local_win_right,8,1,"=control+B");
		wrefresh(local_win_right);
        mvwprintw(local_win_down,1,1,"fuc:");
        mvwscanw(local_win_down,1,6,"%2s%*[^\n]",func);
        wrefresh(local_win_down);
        switch(func[0]){
            case '1': {//printf("chat name: %s\n",send_data->send_name);
                       //wclear(local_win_down); 
                       //mvwprintw(local_win_down,1,1,"chat name:%s\n",send_data->send_name);
                       //wrefresh(local_win_down);
                       chat_with_one(send_data);
                       break;
                      }
            case '2': {chat_with_all(send_data);break;}
            case '3': {show_user_list(send_data);break;}
            case '4': {
                        logout(send_data);
                        wclear(local_win_down);
                        wclear(local_win_left);
                        wclear(local_win_up);
                        wclear(local_win_right);
                        delwin(local_win_down);
                        delwin(local_win_up);
                        delwin(local_win_right);
                        delwin(local_win_left);
                        endwin();
                        exit(0);
                        break;}
            default : {
                    //printf("input error\n");
                wclear(local_win_down);
                wborder(local_win_down,'|',' ','-',' ','+',' ','+',' ');
                mvwprintw(local_win_down,2,1,"input error");   
                wrefresh(local_win_down); 
				getchar();
            }
        }
    }
}
int main(int argc,char **argv){
    int sockfd;
    struct sockaddr_in servaddr;
    //struct packet sendline,recvline;
    char name[MAXNAME];
    if(argc!=2){
        perror("Usage: IMClient <IP address of the server");
        exit(1);
    }
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("Problem in creating the socket");
        exit(2);
    }
    initscr();
    getmaxyx(stdscr,row,col);
    local_win_left=newwin(row,20,0,0);
    local_win_up=newwin(2*row/3,col-41,0,21);
    local_win_down=newwin(row/3,col-21,2*row/3,21);
    local_win_right=newwin(2*row/3,20,0,col-21);
    wborder(local_win_left,' ','|',' ',' ',' ','+',' ','+');
    wborder(local_win_up,'|','|','-','-','+','+','+','+');
    wborder(local_win_down,'|',' ','-',' ','+',' ','+',' ');
    /*memset servaddr*/
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(argv[1]);
    servaddr.sin_port=htons(SERV_PORT);
    /*set socked perference*/
    const int on = 1; 
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    /*connect to the server*/
    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        //perror("Problem in connecting to the server");
        mvwprintw(local_win_up,1,1,"Problem in connecting to the server!!");
        linetag++;
        wrefresh(local_win_up);
        wrefresh(local_win_left);
        wrefresh(local_win_down);
        getchar();
        wclear(local_win_up);
        wclear(local_win_down);
        wclear(local_win_left);
        delwin(local_win_up);
        delwin(local_win_down);
        delwin(local_win_left);
        endwin();
        exit(3);
    }
	struct packet recvline;
	recv(sockfd,&recvline,sizeof(recvline),0);
	if(recvline.service==SERVICE_TELLFULL&&recvline.status==STATUS_FULL){			
        //assert(0);
        mvwprintw(local_win_up,1,1,"Server can not create thread an more!!");
		wrefresh(local_win_up);
        getchar();			
		wclear(local_win_down);
		wclear(local_win_left);
		wclear(local_win_up);	
		wclear(local_win_right);
		delwin(local_win_down);
		delwin(local_win_up);
		delwin(local_win_right);
		delwin(local_win_left);
		endwin();
		exit(0);
	}
    /*first use & set username*/
    memset(&name,0,sizeof(name));
    //printf("Welcome to use IM system!!!!\n");
    mvwprintw(local_win_up,1,1,"Welcome to use IM system!!!\n");
    linetag++;
    //mvwprintw(local_win_down,2*row/3+1,22,"Please enter name to login:");
    wrefresh(local_win_up);
    wrefresh(local_win_left);
    wrefresh(local_win_down);
    //scanw("%s%*[^\n]",name);
    while(!creatUserName(sockfd,name)){
        //printf("The name has been used!!!\n");
        if(linetag==(2*row/3-1)) {
            linetag=1;
            wclear(local_win_up);
            wborder(local_win_up,'|','|','-','-','+','+','+','+');
        }
        mvwprintw(local_win_up,linetag,1,"The name has been used or null!!!");
        linetag++;
        wrefresh(local_win_left);
        wrefresh(local_win_up);
        wrefresh(local_win_down);
    }
    /*the thread of receive and send*/
    pthread_t send_thread,receive_thread;
    struct send_thread_data send_data;
    struct receive_thread_data recv_data;
    send_data.sockfd=sockfd;
    strcpy(send_data.send_name,name);
    //strcpy(send_data.recv_name,);
    recv_data.sockfd=sockfd;
    int rc;
    rc=pthread_create(&send_thread,NULL,send_handle,(void *)&send_data);
    if(rc){
        //printf("ERRPR;return code from pthread_create() is %d\n",rc);
        wclear(local_win_up);
        wborder(local_win_up,'|','|','-','-','+','+','+','+');
        mvwprintw(local_win_up,1,1,"ERROR;return code from pthread_create() is %d\n",rc);
        wrefresh(local_win_up);
        getchar();
        wclear(local_win_up);
        wclear(local_win_left);
        wclear(local_win_down);
        delwin(local_win_left);
        delwin(local_win_up);
        delwin(local_win_down);
        endwin();
        exit(-1);
    }
    rc=pthread_create(&receive_thread,NULL,receive_handle,(void *)&recv_data);
    if(rc){
        //printf("ERROR;return code from pthread_create() is %d\n",rc);
        wclear(local_win_up);
        wborder(local_win_up,'|','|','-','-','+','+','+','+');
        mvwprintw(local_win_up,1,1,"ERROR;return code from pthread_create() is %d\n",rc);
        wrefresh(local_win_up);
        getchar();
        wclear(local_win_left);
        wclear(local_win_up);
        wclear(local_win_down);
        delwin(local_win_left);
        delwin(local_win_up);
        delwin(local_win_down);
        endwin();
        exit(-1);
    }
    pthread_join(receive_thread,NULL);
    pthread_join(send_thread,NULL);
    wclear(local_win_left);
    wclear(local_win_up);
    wclear(local_win_down);
    delwin(local_win_left);
    delwin(local_win_up);
    delwin(local_win_down);
    endwin();
    return 0;
}

