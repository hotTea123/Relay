#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include<unordered_map>
#include"buffer_manager.h"
#include"message_manager.h"

class Map_Manager{
    private:
        std::unordered_map<int, int> userid_sockfd;
        std::unordered_map<int, int> sockfd_userid;

    public:
        std::unordered_map<int, Buffer> userid_buffer;
        std::unordered_map<int, HEADERMSG> userid_header;
        bool find_userbuf(int userid);     //userid_buffer中是否存在userid
        bool find_userhead(int userid);
        void insert_u_s(int userid, int sockfd);
        void insert_u_b(int userid, Buffer buffer);
        void insert_u_h(int userid, HEADERMSG header);
        void delete_u_s(int userid);
        void delete_u_b(int userid);
        int get_sockfd_from_userid(int userid);
        int get_userid_from_sockfd(int sockfd);
        void clear_header(int userid);
        ~Map_Manager() = default;
};

#endif