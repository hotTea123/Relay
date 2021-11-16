#include"buffer_manager.h"


Buffer::Buffer(int sockfd){
    fd = sockfd;
    write_index = 0;
    read_index = 0;
    //buf.assign(str.begin(), str.end());
}

bool Buffer::is_empty(){
    if(write_index == read_index)
        return true;
    else
        return false;
}

int Buffer::length(){
    return write_index - read_index;
}

void Buffer::put_to_buffer(std::vector<char> str, int len){
    //将一个vector放入buffer
    buf.insert(buf.end(), str.begin(), str.end());
    write_index += str.size();
}

void Buffer::head_to_buf(HEADERMSG head){
    buf.insert(buf.end(), (char*)&head, (char*)&head + HEADLEN);
    write_index += HEADLEN;
}

char* Buffer::to_char(){
    return &buf[read_index];
}


void Buffer::read_buffer(int n){
    read_index += n;
}

void Buffer::write_buffer(int n){
    write_index += n;
}

void Buffer::reinit(int size){
    write_index = 0;
    read_index = 0;
    std::vector<char>().swap(buf);    //清空buf
    buf.resize(size);
}

std::vector<char> Buffer::get_buf(){
    return buf;
}

int Buffer::get_write_index(){
    return write_index;
}