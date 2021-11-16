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

#define MAXEPOLLSIZE 10000
#define LISTENQ 5000


class Relay_Server{
    private:
        //int sockfd;
        int epfd;
        int listenfd;
        int connfd;
        int number;//服务器在线连接数,分配的号码

    public:
        Relay_Server();
        Map_Manager map_manager;
        bool initServer(const char* ip,int port);
        int setnonblocking(int fd);
        void add_event(int epfd, int fd, int state);
        void modify_event(int epfd, int fd, int state);
        void delete_event(int epfd, int fd, int state);
        int recv_from_cli(int connfd);
        void chars_to_vector(char *buf, int len);
        void runServer();
        ~Relay_Server();
};



#endif