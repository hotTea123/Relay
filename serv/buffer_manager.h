#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include<vector>
#include"message_manager.h"

class Buffer{
    private:
        std::vector<char> buf;
        int write_index;
        int read_index;
        int fd;    //fd为sockfd或者connfd

    public:
        Buffer(int sockfd);
        bool is_empty();
        int len();
        void put_vector_to_buffer(std::vector<char> str, int len);
        char* to_char();
        void read_buffer(int n);   //将buffer的数据读到socket
        void write_buffer(int n);   //将socket的数据写入buffer
        void reinit(int size);
        std::vector<char> get_buf();
        int get_write_index();
};



#endif 