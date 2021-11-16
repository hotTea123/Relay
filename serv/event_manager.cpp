#include "event_manager.h"

void EventManager::do_event(int fd, int state, int type) {
    epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollFd, type, fd, &ev);
}

EventManager::EventManager(int epfd) {
    epollFd = epfd;
}

void EventManager::add_event(int fd, int state) {
    do_event(fd, state, EPOLL_CTL_ADD);
    setnonblocking(fd);
}
void EventManager::modify_event(int fd, int state) {
    do_event(fd, state, EPOLL_CTL_MOD);
    setnonblocking(fd);
}
void EventManager::delete_event(int fd, int state) {
    do_event(fd, state, EPOLL_CTL_DEL);
}

int EventManager::setnonblocking(int fd){
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}