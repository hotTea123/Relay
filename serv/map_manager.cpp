#include"map_manager.h"

bool Map_Manager::find_userbuf(int userid){
    if(userid_buffer.find(userid) == userid_buffer.end())
        return false;
    return true;
}

bool Map_Manager::find_connbuf(int connfd){
    if(connfd_buffer.find(connfd) == connfd_buffer.end())
        return false;
    return true;
}

bool Map_Manager::find_connhead(int userid){
    if(connfd_header.find(userid) == connfd_header.end())
        return false;
    return true;
}

void Map_Manager::insert_u_s(int userid, int sockfd){
    userid_sockfd.insert(std::pair<int, int>(userid, sockfd));
    sockfd_userid.insert(std::pair<int, int>(sockfd, userid));
}

void Map_Manager::insert_u_c(int userid, int connfd){
    userid_connfd.insert(std::pair<int, int>(userid, connfd));
    connfd_userid.insert(std::pair<int, int>(connfd, userid));
}

void Map_Manager::insert_u_b(int userid, Buffer buffer){
    userid_buffer.insert(std::pair<int, Buffer>(userid, buffer));
}

void Map_Manager::insert_c_b(int connfd, Buffer buffer){
    connfd_buffer.insert(std::pair<int, Buffer>(connfd, buffer));
}

void Map_Manager::insert_c_h(int connfd, HEADERMSG header){
    connfd_header.insert(std::pair<int, HEADERMSG>(connfd, header));
}


void Map_Manager::delete_u_s(int userid){
    int sockfd = get_sockfd_from_userid(userid);
    userid_sockfd.erase(userid);
    sockfd_userid.erase(sockfd);
}

void Map_Manager::delete_u_c(int userid){
    int connfd = get_connfd_from_userid(userid);
    userid_connfd.erase(userid);
    connfd_userid.erase(connfd);
}

int Map_Manager::get_userid_from_connfd(int connfd){
    if (connfd_userid.find(connfd) == connfd_userid.end())
        return -1;
    else
        return connfd_userid.at(connfd);
}

int Map_Manager::get_sockfd_from_userid(int userid){
    if (userid_sockfd.find(userid) == userid_sockfd.end())
        return -1;
    else
        return userid_sockfd.at(userid);
}

int Map_Manager::get_connfd_from_userid(int userid){
    if (userid_connfd.find(userid) == userid_connfd.end())
        return -1;
    else
        return userid_connfd.at(userid);
}

int Map_Manager::get_userid_from_sockfd(int sockfd){
    if (sockfd_userid.find(sockfd) == sockfd_userid.end())
        return -1;
    else
        return sockfd_userid.at(sockfd);
}

void Map_Manager::clear_header(int connfd){
    connfd_header.erase(connfd);
}