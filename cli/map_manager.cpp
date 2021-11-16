#include"map_manager.h"

bool Map_Manager::find_userbuf(int userid){
    if(userid_buffer.find(userid) == userid_buffer.end())
        return false;
    return true;
}

bool Map_Manager::find_userhead(int userid){
    if(userid_header.find(userid) == userid_header.end())
        return false;
    return true;
}

void Map_Manager::insert_u_s(int userid, int sockfd){
    userid_sockfd.insert(std::pair<int, int>(userid, sockfd));
    sockfd_userid.insert(std::pair<int, int>(sockfd, userid));
}

void Map_Manager::insert_u_b(int userid, Buffer buffer){
    userid_buffer.insert(std::pair<int, Buffer>(userid, buffer));
}

void Map_Manager::insert_u_h(int userid, HEADERMSG header){
    userid_header.insert(std::pair<int, HEADERMSG>(userid, header));
}

void Map_Manager::delete_u_s(int userid){
    int sockfd = get_sockfd_from_userid(userid);
    userid_sockfd.erase(userid);
    sockfd_userid.erase(sockfd);
}

void Map_Manager::delete_u_b(int userid){
    userid_buffer.erase(userid);
}

int Map_Manager::get_sockfd_from_userid(int userid){
    if (userid_sockfd.find(userid) == userid_sockfd.end())
        return -1;
    else
        return userid_sockfd.at(userid);
}

int Map_Manager::get_userid_from_sockfd(int sockfd){
    if (sockfd_userid.find(sockfd) == sockfd_userid.end())
        return -1;
    else
        return sockfd_userid.at(sockfd);
}

void Map_Manager::clear_header(int userid){
    userid_header.erase(userid);
}