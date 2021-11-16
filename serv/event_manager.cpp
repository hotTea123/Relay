#include "event_manager.h"

void EventManager::do_event(int fd, int state, int type) {
    epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epfd, type, fd, &ev);
}

EventManager::EventManager(int epollfd) {
    epfd = epollfd;
}

void EventManager::add_event(int fd, int state) {
    do_event(fd, state, EPOLL_CTL_ADD);
}
void EventManager::modify_event(int fd, int state) {
    do_event(fd, state, EPOLL_CTL_MOD);
}
void EventManager::delete_event(int fd, int state) {
    do_event(fd, state, EPOLL_CTL_DEL);
}