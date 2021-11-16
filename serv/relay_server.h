#ifndef RELAYSERVER_H
#define RELAYSERVER_H

#include  <unistd.h>
#include  <sys/types.h>       /* basic system data types */
#include  <sys/socket.h>      /* basic socket definitions */
#include  <netinet/in.h>      /* sockaddr_in{} and other Internet defns */
#include  <arpa/inet.h>       /* inet(3) functions */
#include  <sys/epoll.h> /* epoll function */
#include  <fcntl.h>     /* nonblocking */
#include  <sys/resource.h> /*setrlimit */
 
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <utility>
#include <map>
#include <queue>

#include"event_manager.h"
#include"map_manager.h"

#define MAXEPOLLSIZE 20000
#define LISTENQ 5000
#define IPaddress "127.0.0.1"


class Relay_Server{
    private:
        //int sockfd;
        int listenfd;
        int connfd;
        int number;//服务器在线连接数,分配的号码

    public:
        Relay_Server();
        Map_Manager map_manager;
        bool initServer(const char* ip,int port);
        int setnonblocking(int fd);
        int recv_from_cli(int connfd);
        int relay_to_cli(int connfd);
        void runServer();
        ~Relay_Server();
};



#endif