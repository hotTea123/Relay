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

void Relay_Server::modify_event(int epfd, int fd, int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

void Relay_Server::add_event(int epfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void Relay_Server::delete_event(int epfd, int fd, int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
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

    epfd = epoll_create(1);
    if(epfd < 0){
        std::cout << "create epollfd error "<< std::endl;
        return false;
    }

    add_event(epfd, listenfd, EPOLLIN);
    std::cout << "RelayServer startup, port " << ip << ", max connection is ";
    std::cout << MAXEPOLLSIZE << ", backlog is " << LISTENQ<< std::endl;
    return true;
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
        //map_manager.userid_buffer.at(userid).head_to_buf(header);
        //重定义大小
        map_manager.connfd_buffer.at(connfd).reinit(data_len);
    }else{
        header = map_manager.connfd_header.at(connfd);
        data_len = header.len - HEADLEN;
    }

    int recv_count = map_manager.connfd_buffer.at(connfd).get_write_index();
    //接收报文实体
    while(recv_count != data_len){
        //char *buf = new char[len];
        int nread = read(connfd, map_manager.connfd_buffer.at(connfd).to_char(),
            data_len - recv_count);
        //int nread = read(connfd, &buf[recv_count], len - recv_count);
        if(nread < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                std::cout << "read EWOULDBLOCK" << std::endl;
                break;
            }
            else{
                std::cout << "receive error " << strerror(errno) << std::endl;
                return -1;
            } 
        }else{
            recv_count += nread;
            //map_manager.userid_buffer.at(userid).read_index += nwrite;          
            map_manager.connfd_buffer.at(connfd).write_buffer(nread);  //readindex移动
        }
    }
    if(recv_count == data_len)
        return 1;    //发送完毕
    else{
        return 2;    //没发送完
    }
}


void Relay_Server::runServer(){
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
                add_event(epfd, connfd, EPOLLIN);
            }

            else if(events[i].events & EPOLLIN){
                //处理新的报文
                connfd = fd;
                //收数据
                int nread = recv_from_cli(connfd);
                if(nread == -1){
                    userid = map_manager.get_userid_from_connfd(connfd);
                    close(connfd);
                    map_manager.delete_u_c(userid);
                }else if(nread == 2){
                    continue;
                }else if(nread == 1){    //服务器接收完毕，转发出去
                    //struct epoll_event event;
                    int decid = map_manager.connfd_header.at(connfd).decid;
                    int des_connfd = map_manager.get_connfd_from_userid(decid);

                    // //将数据从src_connfd的buffer存到des_connfd的buffer
                    // if(map_manager.find_connbuf(des_connfd) == false){
                    //     //des没有buffer，创建一个
                    //     Buffer des_buffer(des_connfd);
                    //     map_manager.insert_c_b(des_connfd, des_buffer);
                    // }
                    int send_count = 0, nwrite;
                    //直接转发
                    //写头部
                    while(1){
                        //相当于会一直卡在这，所以是不好的实现，还是得复制到另一个缓冲区操作
                        HEADERMSG header = map_manager.connfd_header.at(connfd);
                        nwrite = write(des_connfd, (void *)&header, HEADLEN);
                        if(nwrite < 0){
                            if(errno == EWOULDBLOCK || errno == EAGAIN){
                                std::cout << "sendHeader EWOULDBLOCK" << std::endl;
                                continue;
                            }else{
                                std::cout << "sendHeader error " << std::endl;
                                break;
                            } 
                        }
                        else{
                            break;
                        }
                    }

                    //写数据
                    while(!map_manager.connfd_buffer.at(connfd).is_empty()){
                        nwrite = write(des_connfd, map_manager.connfd_buffer.at(connfd).to_char(), 
                            map_manager.connfd_buffer.at(connfd).length());
                        if(nwrite < 0){
                            if(errno == EWOULDBLOCK || errno == EAGAIN){
                                std::cout << "write EWOULDBLOCK" << std::endl;
                                break;
                            }else{
                                std::cout << "send error " << strerror(errno) << std::endl;
                                break;
                            }
                        }else{
                            send_count += nwrite;
                            map_manager.connfd_buffer.at(connfd).read_buffer(nwrite);  //readindex移动
                        }
                    }
                    if(map_manager.connfd_buffer.at(connfd).is_empty()){
                        //缓冲区的数据发送完毕了，重新初始化buffer
                        map_manager.connfd_buffer.at(connfd).reinit(0);
                        map_manager.clear_header(connfd);
                    }
                }
            }
            //         //不再触发EPOLLOUT了
            //         std::vector<char> str = map_manager.connfd_buffer.at(connfd).get_buf();
            //         map_manager.connfd_buffer.at(des_connfd).put_to_buffer(str, data_len);
            //         //这里必须指定长度data_len，因为原来的buffer的长度可能不等于data_len,会大一些
            //         // map_manager.connfd_buffer.at(des_connfd).put_to_buffer(
            //         //     map_manager.connfd_buffer.at(connfd).get_buf());

            //         //清空原来connfd的buffer
            //         map_manager.connfd_buffer.at(connfd).reinit(0);
            //         add_event(epfd, des_connfd, EPOLLOUT);
            //     }else{          //服务器还未接收完毕，需要下次继续接收
            //         continue;
            //     }
            // }

            //感觉都不用再触发这个EPPOLLOUT，在上一轮epollin直接写入(pazzuled)
            //----------------------------------------------------
            // else if(events[i].events & EPOLLOUT){
            //     //转发报文
            //     int send_count;
            //     connfd = fd;
            //     //这段代码跟客户端发送的代码很像，可以考虑封装到一个class
            //     //增设一个报文管理类，处理报文的接收、发送等操作
            //     while(!map_manager.connfd_buffer.at(connfd).is_empty()){ 
            //         int nwrite = write(connfd, map_manager.connfd_buffer.at(connfd).to_char(), 
            //             map_manager.connfd_buffer.at(connfd).length());
            //         if(nwrite < 0){
            //             std::cout << "send error" << strerror(errno) << std::endl;
            //             break;
            //         }else if(errno == EWOULDBLOCK || errno == EAGAIN){
            //             break;
            //         }else{
            //             send_count += nwrite;
            //             map_manager.connfd_buffer.at(connfd).read_buffer(nwrite);  //readindex移动
            //         }
            //     }
            //     if(map_manager.connfd_buffer.at(connfd).is_empty()){
            //         //缓冲区的数据发送完毕了，重新初始化buffer
            //         map_manager.connfd_buffer.at(connfd).reinit(0);
            //         delete_event(epfd, connfd, EPOLLOUT);
            //     }
            // }
        }
    }
}