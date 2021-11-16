#ifndef RELAYCLIENT_H
#define RELAYCLIENT_H

#include"message_manager.h"
#include"map_manager.h"

#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<queue>
#include<map>
#include<string.h>
#include<iostream>

#define MAXCLICOUNT 10000

typedef enum{
    CON_NNAME = 0,  //连接成功,但没名字
    CON_HNAME = 1,  //连接成功，且被分配了名字 
    SEND = 2,   //发送成功
    RECV_HEADER = 3,    //接受头部成功
    RECV_DATA = 4     //接收数据成功
}USER_STATE;

class Relay_Client
{
    private:
        int user_count;//用户数量
        int epfd;
        std::unordered_map<int, USER_STATE> sockfd_state;
        int port;
        char ip[10];
        int send_msg_count;   //已经成功发送的消息数目
        int recv_msg_count;   //已经成功接受的消息数目
        std::vector<char> data;

    public:
        Relay_Client(int session_count, const char* addr, int port);
        Map_Manager map_manager;
        int connToServ(const char* servaddr, int servport);//连接到服务器成功返回socketfd,失败返回的socketfd为-1 
        int setnonblocking(int fd);
        int send_to_serv(int userid);
        int recv_from_serv(int sockfd);
        std::vector<char> getline(int n);
        int run_cli();
        ~Relay_Client() = default;
};

#endif