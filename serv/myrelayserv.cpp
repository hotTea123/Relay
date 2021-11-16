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
#include <map>
using namespace std;


 
#define MAXEPOLLSIZE 10000
#define MAXLINE 10240
 
int handle(int connfd);

int findfd(int fd, map<int, int> mapfd);

void modify_event(int epfd, int fd, int state);
int add_event(int epollfd, int fd, int state);
void delete_event(int epollfd, int fd, int state);
 
int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}


int main(int argc, char **argv)
{
    int servPort = 6888;
    int listenq = 1024;
 
    int listenfd, connfd, epfd, num, i, nread, curfds, pairfd, single = 0, acceptCount = 0;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct epoll_event events[MAXEPOLLSIZE];
    //struct rlimit rt;
    char buf[MAXLINE];
    map<int, int> mapfd; 
    map<int, int>::iterator iter;

 
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servPort);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
      perror("can't create socket file");
        return -1;
    }
 

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
 
    if (setnonblocking(listenfd) < 0) {
        perror("setnonblock error");
    }
 
    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind error");
        return -1;
    }
    if (listen(listenfd, listenq) == -1)
    {
        perror("listen error");
        return -1;
    }

    epfd = epoll_create(1);
    add_event(epfd, listenfd, EPOLLIN);
    curfds = 1;
    printf("epollserver startup,port %d, max connection is %d, backlog is %d\n", servPort, MAXEPOLLSIZE, listenq);
    
    
    for (;;) {
        /* 等待有事件发生 */
        num = epoll_wait(epfd, events, curfds, -1);
        // if (num == -1)
        // {
        //     perror("epoll_wait\n");
        //     continue;
        // }
        /* 处理所有事件 */
        for (i = 0; i < num; ++i)
        {
            int fd = events[i].data.fd;
            //处理新连接请求
            if((fd == listenfd) && (events[i].events & EPOLLIN))
            {
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr,&socklen);
                if (connfd < 0)
                {
                    perror("accept error");
                    continue;
                }

                sprintf(buf, "accept from %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                printf("%d:%s", ++acceptCount, buf); 
                memset(buf, 0, MAXLINE);

                if (curfds >= MAXEPOLLSIZE) {
                    fprintf(stderr, "too many connection, more than %d\n", MAXEPOLLSIZE);
                    close(connfd);
                    continue;
                }
                if (setnonblocking(connfd) < 0) {
                    perror("setnonblocking error");
                }
                //ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                if (add_event(epfd, connfd, EPOLLIN) < 0)
                {
                    fprintf(stderr, "add socket '%d' to epoll failed: %s\n", connfd, strerror(errno));
                    return -1;
                }
                curfds++;
                //加入map
                if(single == 0){    //是否存在还未配对的fd
                    mapfd.insert(pair<int, int>(-1, connfd)); 
                    single = 1;
                }else{
                    iter = mapfd.find(-1);
                    pairfd = iter->second;
                    mapfd.erase(iter);
                    mapfd.insert(pair<int, int>(connfd, pairfd));
                    single = 0; 
                }
            }
            
            // 处理客户端发送请求
            else if(events[i].events & EPOLLIN){
                int nread = read(fd, buf, MAXLINE);
                //找到配对的fd
                pairfd = findfd(fd, mapfd);
                if (pairfd == -1){
                    fprintf(stderr, "no peer\n");
                    continue;
                }
                if (nread == -1){
                    perror("read error:");
                    close(fd);
                    delete_event(epfd, fd, EPOLLIN);
                } else if (nread == 0) {
                    fprintf(stderr, "client close.\n");
                    close(fd);
                    delete_event(epfd, fd, EPOLLIN);
                } else {
                    int nwrite = write(pairfd, buf, strlen(buf));
                    if (nwrite == -1){
                        perror("write error:");
                    }
                }
            }
        }  
      }
    close(listenfd);
    return 0;
}


int findfd(int fd, map<int, int> mapfd){
    map<int, int>::iterator iter;
    //在key中
    if(mapfd.count(fd))
        return mapfd[fd];
    iter = mapfd.begin();
    while(iter != mapfd.end()){
        if(iter->second == fd)
            return iter->first;
        iter++;
    }
    return -1;
}



void modify_event(int epfd, int fd, int state) {
  struct epoll_event ev;
  ev.events = state;
  ev.data.fd = fd;
  epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

int add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    return epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}