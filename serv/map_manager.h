#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include<unordered_map>
#include"buffer_manager.h"
#include"message_manager.h"

class Map_Manager{
    private:
        std::unordered_map<int, int> userid_connfd;
        std::unordered_map<int, int> connfd_userid;

    public:
        std::unordered_map<int, HEADERMSG> connfd_header;
        std::unordered_map<int, Buffer> connfd_buffer;
        bool find_connbuf(int connfd);
        bool find_connhead(int connfd);
        void insert_u_c(int userid, int connfd);
        void insert_c_b(int connfd, Buffer buffer);
        void insert_c_h(int connfd, HEADERMSG header);
        void delete_u_c(int userid);
        void delete_c_b(int connfd);
        int get_userid_from_connfd(int connfd);
        int get_connfd_from_userid(int userid);
        void clear_header(int connfd);
        ~Map_Manager() = default;
};

#endif