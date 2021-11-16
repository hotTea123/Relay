#include"relay_client.h"
#include"message_manager.h"
#include"event_manager.h"

Relay_Client::Relay_Client(const char*  addr, int servPort, int session_count, int len)
{
    strcpy(ip, addr);
    user_count = 2 * session_count;
    port = servPort;
    data = getline(len);
    strlen = len;
    send_msg_count = 0;
    recv_msg_count = 0;
}

int Relay_Client::setnonblocking(int fd){
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}


int Relay_Client::connToServ(const char* serverip, int servport){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        std::cout << "create socket error " << strerror(errno) << std::endl;
        return -1;
    }
    //userid_sockfd[userid] = sockfd;    //添加至map

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(servport);
    inet_pton(AF_INET, serverip, &addr.sin_addr);

    // //支持端口复用
    // int reuseadd_on = 1;
    // setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseadd_on, sizeof(reuseadd_on));

    //设置非阻塞
    setnonblocking(sockfd);

    int n;
    if((n = connect(sockfd, (struct sockaddr *)&addr,sizeof(addr))) < 0){
        if(errno != EINPROGRESS){
            return -1;
        }
    }
    sockfd_state.insert(std::pair<int, USER_STATE>(sockfd, CON_NNAME));
    return sockfd;
}


void Relay_Client::run_cli(){
    int epfd = epoll_create(1);
    if(epfd < 0){
        std::cout << "create epollfd error "<< std::endl;
        return;
    }
    EventManager event_manager(epfd);
    
    int sockfd, userid;
    for(int i = 0;i < user_count;i++){
        sockfd = connToServ(ip, port);
        if(sockfd < 0)
            std::cout << "connect error " << std::endl;
        event_manager.add_event(sockfd, EPOLLIN);
    }
    while(1){
        struct epoll_event events[user_count];
        int nready = epoll_wait(epfd, events, user_count, -1);
        for(int i = 0;i < nready;i++){
            struct epoll_event event;
            sockfd = events[i].data.fd; 
            //等待服务器分配id
            if(events[i].events & EPOLLIN && sockfd_state.at(sockfd) == CON_NNAME){
                int number;
                read(sockfd, &number, sizeof(number));
                userid = ntohl(number);
                sockfd_state.at(sockfd) = CON_HNAME;
                map_manager.insert_u_s(userid, sockfd);
                //偶数用户等待发送数据，改变event状态；奇数用户等待接受数据，event状态不变
                if(userid % 2 == 0)
                    event_manager.modify_event(sockfd, EPOLLOUT);
            }
            //发送数据
            else if(events[i].events & EPOLLOUT){   
                userid = map_manager.get_userid_from_sockfd(sockfd);
                int iswrite = send_to_serv(userid);
                if(iswrite < 0){
                    close(sockfd);
                    map_manager.delete_u_s(userid);
                }else if(iswrite == 1){    //客户端发送完毕, 切换接收模式
                    event_manager.modify_event(sockfd, EPOLLIN);
                }else{          //客户端还未发送完毕，需要下次接着发送
                    continue;
                }
            }

            //这段代码跟服务器接收的代码也很像
            else if(events[i].events & EPOLLIN){
                int nread = recv_from_serv(sockfd);
                userid = map_manager.get_userid_from_sockfd(sockfd);
                if(nread == -1){
                    close(sockfd);
                    map_manager.delete_u_s(userid);
                    map_manager.delete_u_b(userid);
                }else if(nread == 1){    //客户端接收完毕，切换发送模式
                    event_manager.modify_event(sockfd, EPOLLOUT);
                }else{
                    continue;
                } 
            }
        }
    }
}

int Relay_Client::send_to_serv(int userid){
    //向服务器发送数据
        
    //报文头
    HEADERMSG header;
    header.srcid = userid;
    if(userid % 2 == 0)
        header.decid = userid + 1;
    else
        header.decid = userid - 1;
    header.len = strlen + HEADLEN;
    
    int sockfd = map_manager.get_sockfd_from_userid(userid);
    if(!map_manager.find_userbuf(userid)){
    //还未创建自己的缓冲区，创建一个,并写入数据(包括报文头部和数据字段)
        Buffer buffer(sockfd);
        map_manager.insert_u_b(userid, buffer);
    }
    map_manager.userid_buffer.at(userid).put_to_buffer(data, strlen);
    
    int nwrite, send_count = 0;
    //写入头部
    nwrite = write(sockfd, (void *)&header, HEADLEN);
    if(nwrite < 0){
        //说明对面已经下机了
        std::cout << "sendHeader error " << std::endl;
        return -1;
    }

    //发送报文实体
    while(!map_manager.userid_buffer.at(userid).is_empty()){
        //判定用户缓冲区空的条件是writeindex = readindex
        nwrite = write(sockfd, map_manager.userid_buffer.at(userid).to_char(),
                        map_manager.userid_buffer.at(userid).length());
        if(nwrite < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                std::cout << "write EWOULDBLOCK" << std::endl;
                return 2;
            }
            std::cout << "send error " << strerror(errno) << std::endl;
            return -1;
        }else{
            send_count += nwrite;
            map_manager.userid_buffer.at(userid).read_buffer(nwrite);
        }
    }

    //缓冲区的数据发送完毕了，重新初始化buffer
    map_manager.userid_buffer.at(userid).reinit(0);
    std::cout << "client" << userid << " have sent " << send_count;
    std::cout << " characters successfully" << std::endl;
    send_msg_count++;
    std::cout << "successfully send number: " << send_msg_count << std::endl;
    return 1;
}

int Relay_Client::recv_from_serv(int sockfd){
    int userid = map_manager.get_userid_from_sockfd(sockfd);
    HEADERMSG header;
    int data_len;
    //判断是否已经读入头文件
    if(!map_manager.find_userhead(userid)){
        //读报文头
        int nread = read(sockfd, (void *)&header, sizeof(HEADERMSG));
        if(nread < 0){
            //对面已经下机
            std::cout << "readHeader error" << std::endl;
            return -1;
        }
        map_manager.insert_u_h(userid, header);
        if(!map_manager.find_userbuf(userid)){
        //还未创建自己的缓冲区，创建一个空的
            Buffer buffer(sockfd);
            map_manager.insert_u_b(userid, buffer);
        }
        data_len = header.len - HEADLEN;
        //map_manager.userid_buffer.at(userid).head_to_buf(header);
        //重定义大小
        map_manager.userid_buffer.at(userid).reinit(data_len);
    }else{
        header = map_manager.userid_header.at(userid);
        data_len = header.len - HEADLEN;
    }

    int recv_count = map_manager.userid_buffer.at(userid).get_write_index();
    //接收报文实体
    while(recv_count != data_len){
        int nread = read(sockfd, map_manager.userid_buffer.at(userid).to_char(),
            data_len - recv_count);
        if(nread < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                std::cout << "read EWOULDBLOCK" << std::endl;
                return 2;
            }
            else{
                std::cout << "receive error " << strerror(errno) << std::endl;
                return -1;
            } 
        }else{
            recv_count += nread;
            //map_manager.userid_buffer.at(userid).read_index += nwrite;
            map_manager.userid_buffer.at(userid).write_buffer(nread);  //readindex移动
        } 
    }
    //接收完毕
    std::cout << "client" << userid << " have received" << std::flush;
    //write(STDOUT_FILENO, map_manager.userid_buffer.at(userid).to_char(), data_len);
    std::cout << std::endl;
    sockfd_state.at(sockfd) = RECV_DATA;
    if(map_manager.userid_buffer.at(userid).get_buf() == data){
        //清空缓冲区,打屏后缓冲区并不变
        recv_msg_count++;
        std::cout << "successfully receive number: " << recv_msg_count << std::endl;
    }else{
        std::cout << "inconsistent data" << std::endl;
    }
    map_manager.userid_buffer.at(userid).reinit(0);
    map_manager.clear_header(userid);
    return 1;
}

std::vector<char> Relay_Client::getline(int n){
    //后可写一个随机生成数n，让他随机发送任意长度的报文
    std::string character = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,./;\"'<>?";
    int i;
    char ch;
    std::vector<char> buf;
    int len = character.size();
    srand((unsigned int)time((time_t *)NULL));
    for(i = 0;i < n;i++){
        ch = character[rand() % len];
        buf.push_back(ch);
    }
    return buf;
}