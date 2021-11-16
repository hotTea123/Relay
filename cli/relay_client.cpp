#include"relay_client.h"
#include"message_manager.h"

Relay_Client::Relay_Client(int session_count, const char*  addr, int servPort)
{
    strcpy(ip, addr);
    user_count = 2 * session_count;
    epfd = epoll_create(1);
    port = servPort;
    data = getline(STRLEN);
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

int Relay_Client::send_to_serv(int userid){
    //向服务器发送数据
        
    //报文头
    HEADERMSG header;
    header.srcid = userid;
    if(userid % 2 == 0)
        header.decid = userid + 1;
    else
        header.decid = userid - 1;
    header.len = STRLEN + HEADLEN;
    
    int sockfd = map_manager.get_sockfd_from_userid(userid);
    if(!map_manager.find_userbuf(userid)){
    //还未创建自己的缓冲区，创建一个,并写入数据(包括报文头部和数据字段)
        Buffer buffer(sockfd);
        map_manager.insert_u_b(userid, buffer);
    }
    //map_manager.userid_buffer.at(userid).head_to_buf(header);
    map_manager.userid_buffer.at(userid).put_to_buffer(data, STRLEN);

    //将封装好的报文放入自己的缓冲区，如果缓冲区有东西了，则需要放到缓冲区末尾
    //？????Question：如何把一个结构体(封装好的报文)写入一个string中 
    //先将struct强转为char*,问题变成char*和string的相互转换
    //出现了错误，因为结构体中含有string，string存储的不是实际的东西，所以只能一个个写
    
    int nwrite, send_count = 0;
    //写入头部
    nwrite = write(sockfd, (void *)&header, HEADLEN);
    if(nwrite < 0){
        std::cout << "sendHeader error " << std::endl;
        return -1;
    }
        
    //将用户缓冲区的数据发送,如果没出错，一次性发送完所有的数据

    //发送报文实体
    while(!map_manager.userid_buffer.at(userid).is_empty()){
        //判定用户缓冲区空的条件是writeindex = readindex
        nwrite = write(sockfd, map_manager.userid_buffer.at(userid).to_char(),
                        map_manager.userid_buffer.at(userid).length());
        if(nwrite < 0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                std::cout << "write EWOULDBLOCK" << std::endl;
                break;
            }else{
                std::cout << "send error " << strerror(errno) << std::endl;
                return -1;
            }
        }else{
            send_count += nwrite;
            //map_manager.userid_buffer.at(userid).read_index += nwrite;
            map_manager.userid_buffer.at(userid).read_buffer(nwrite);  //readindex移动
        }
    }

    if(map_manager.userid_buffer.at(userid).is_empty()){
        //缓冲区的数据发送完毕了，重新初始化buffer
        map_manager.userid_buffer.at(userid).reinit(0);
        std::cout << "client" << userid << " have sent " << send_count;
        std::cout << " characters successfully" << std::endl;
        sockfd_state.at(sockfd) = SEND;
        send_msg_count++;
        std::cout << "successfully send number: " << send_msg_count << std::endl;
    }
    return send_count;
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

    // //重定义userid对应的buffer大小，因为一个客户要么发送，要么接收，不存在一边发送一边接收的情况
    // map_manager.userid_buffer.at(userid).reinit(header.len);


    int recv_count = map_manager.userid_buffer.at(userid).get_write_index();
    //接收报文实体
    while(recv_count != data_len){
        int nread = read(sockfd, map_manager.userid_buffer.at(userid).to_char(),
            data_len - recv_count);
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
            map_manager.userid_buffer.at(userid).write_buffer(nread);  //readindex移动
        } 
    }
    if(recv_count == data_len){
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
            std::cout << "不一样的数据！" << std::endl;
        }
        map_manager.userid_buffer.at(userid).reinit(0);
        map_manager.clear_header(userid);
        return 1;
    }else{
        return 2;
    }
}


int Relay_Client::run_cli(){
    int sockfd, userid;
    for(int i = 0;i < user_count;i++){
        sockfd = connToServ(ip, port);
        if(sockfd < 0)
            std::cout << "connect error " << std::endl;

        struct epoll_event event;
        event.data.fd = sockfd;
        event.events = EPOLLIN;   //先收到服务器发送的userid
        epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
    }

    while(1){
        struct epoll_event events[MAXCLICOUNT];
        int nready = epoll_wait(epfd, events, MAXCLICOUNT, -1);
        for(int i = 0;i < nready;i++){
            struct epoll_event event;
            sockfd = events[i].data.fd; 

            if(events[i].events & EPOLLIN && sockfd_state.at(sockfd) == CON_NNAME){
                //读第一个名字报文
                //收到自己被分配的userid
                int number;
                read(sockfd, &number, sizeof(number));
                userid = ntohl(number);
                sockfd_state.at(sockfd) = CON_HNAME;
                map_manager.insert_u_s(userid, sockfd);

                //偶数用户等待发送数据，集数用户等待接受数据
                if(userid % 2 == 0){
                    event.data.fd = sockfd;
                    event.events = EPOLLOUT;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
                }
            }

            else if(events[i].events & EPOLLOUT){   //偶数发送
                userid = map_manager.get_userid_from_sockfd(sockfd);
                int nwrite = send_to_serv(userid);
                if(nwrite < 0){
                    close(sockfd);
                    map_manager.delete_u_s(userid);
                }else if(sockfd_state.at(sockfd) == SEND){    //客户端发送完毕, 切换接受模式
                    event.data.fd = sockfd;
                    event.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
                    //清空缓冲区
                    //map_manager.userid_buffer.at(userid).reinit(0);
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
                }else if(nread == 1){   //(sockfd_state.at(sockfd) == RECV){     //客户端接收完毕，打印接收成功的标志
                    event.data.fd = sockfd;
                    event.events = EPOLLOUT;      //切换发送模式
                    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
                }else{
                    continue;
                } 
            }
        }
    }
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