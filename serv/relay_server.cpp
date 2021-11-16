#include"relay_server.h"
#include"event_manager.h"
#include"message_manager.h"
#include"map_manager.h"
#include<iostream>

Relay_Server::Relay_Server(){
    number = 0;
}

Relay_Server::~Relay_Server(){
    close(listenfd);
}

int Relay_Server::setnonblocking(int fd){
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

bool Relay_Server::initServer(const char* ip,int port){
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0){
        std::cout << "can't create socket"<< std::endl;
        return false;
    }
    setnonblocking(listenfd);

    // int opt = 1;
    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if(bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        std::cout << "bind error "<< std::endl;
        return false;
    }

    if(listen(listenfd, LISTENQ) < 0){
        std::cout << "listen error "<< std::endl;
        return false;
    }

    std::cout << "RelayServer have started up, port " << ip << ", max connection is ";
    std::cout << MAXEPOLLSIZE << std::endl;
    return true;
}


void Relay_Server::runServer(){
    if(!initServer(IPaddress, 8067)){
        std::cout << "init server error" << std::endl;
        return;
    }

    int epfd = epoll_create(1);
    if(epfd < 0){
        std::cout << "create epollfd error "<< std::endl;
        return;
    }
    EventManager event_manager(epfd);
    event_manager.add_event(listenfd, EPOLLIN);

    struct sockaddr_in cliaddr;
    int userid;
    socklen_t socklen = sizeof(struct sockaddr_in);
    while(1){
        int i;
        struct epoll_event events[MAXEPOLLSIZE];
        int nready = epoll_wait(epfd, events, MAXEPOLLSIZE, -1);
        if (nready == -1){ 
            perror("epoll_wait error ");
            continue;
        }

        for(i = 0;i < nready;i++){
            int fd = events[i].data.fd;
            if(fd == listenfd && events[i].events & EPOLLIN){
                //处理新连接请求
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &socklen);
                if (connfd < 0)
                {
                    perror("accept error ");
                    continue;
                }
                std::cout << "client"<< number++ << ":Accept from " << inet_ntoa(cliaddr.sin_addr);
                std::cout << ":" << cliaddr.sin_port << std::endl;

                //给客户端分配userid
                userid = htonl(number - 1);   //userid从0开始
                write(connfd, &userid, sizeof(userid));
                map_manager.insert_u_c(number - 1, connfd);
                event_manager.add_event(connfd, EPOLLIN);
            }

            else if(events[i].events & EPOLLIN){
                //处理新的报文
                connfd = fd;
                userid = map_manager.get_userid_from_connfd(connfd);
                //收数据
                int nread, nwrite;
                nread = recv_from_cli(connfd);
                if(nread == -1){
                    std::cout << "receive error " << strerror(errno) << std::endl;
                    close(connfd);
                    map_manager.delete_u_c(userid);
                }else if(nread == 2){
                    //发生EWOULDBLOCK
                    std::cout << "read EWOULDBLOCK " << std::endl;
                }else if(nread == 1){    //服务器接收完毕，转发出去
                    nwrite = relay_to_cli(connfd);
                    if(nwrite == -1){
                        std::cout << "send error " << strerror(errno) << std::endl;
                        close(connfd);
                        map_manager.delete_u_c(userid);
                    }else{
                        //缓冲区的数据发送完毕了，重新初始化buffer
                        map_manager.connfd_buffer.at(connfd).reinit(0);
                        map_manager.clear_header(connfd);
                    }
                }
            }
        }
    }
}


int Relay_Server::recv_from_cli(int connfd){
    HEADERMSG header;
    int data_len;
    //判断是否已经读入头文件
    if(!map_manager.find_connhead(connfd)){
        //读报文头
        int nread = read(connfd, (void *)&header, sizeof(HEADERMSG));
        if(nread < 0){
            std::cout << "readHeader error" << std::endl;
            return -1;
        }
        map_manager.insert_c_h(connfd, header);
        if(!map_manager.find_connbuf(connfd)){
        //还未创建自己的缓冲区，创建一个空的
            Buffer buffer(connfd);
            map_manager.insert_c_b(connfd, buffer);
        }
        data_len = header.len - HEADLEN;
        //重定义大小
        map_manager.connfd_buffer.at(connfd).reinit(data_len);
    }else{
        header = map_manager.connfd_header.at(connfd);
        data_len = header.len - HEADLEN;
    }

    int recv_count = map_manager.connfd_buffer.at(connfd).get_write_index();
    //接收报文实体
    while(recv_count != data_len){
        int nread = read(connfd, map_manager.connfd_buffer.at(connfd).to_char(),
            data_len - recv_count);
        //int nread = read(connfd, &buf[recv_count], len - recv_count);
        if(nread < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN)
                return 2;
            else
                return -1;    //出错
        }else{
            recv_count += nread;
            //map_manager.userid_buffer.at(userid).read_index += nwrite;          
            map_manager.connfd_buffer.at(connfd).write_buffer(nread);  //readindex移动
        }
    }
    return 1;
}

int Relay_Server::relay_to_cli(int connfd){
    //转发
    int send_count = 0, nwrite;
    HEADERMSG header = map_manager.connfd_header.at(connfd);
    int decid = header.decid;
    int des_connfd = map_manager.get_connfd_from_userid(decid);

    //先写头部
    nwrite = write(des_connfd, (void *)&header, HEADLEN);

    //写数据
    while(!map_manager.connfd_buffer.at(connfd).is_empty()){
        nwrite = write(des_connfd, map_manager.connfd_buffer.at(connfd).to_char(), 
            map_manager.connfd_buffer.at(connfd).len());
        if(nwrite < 0){
            return -1;
        }else{
            send_count += nwrite;
            map_manager.connfd_buffer.at(connfd).read_buffer(nwrite);  //readindex移动
        }
    }
    return 1;    //写完了
}