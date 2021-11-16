#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H
#include <sys/epoll.h>
#include <fcntl.h>

class EventManager {
    private:
        int epfd;
        void do_event(int fd, int state, int type);

    public:
        EventManager(int epollfd);
        void add_event(int fd, int state);
        void modify_event(int fd, int state);
        void delete_event(int fd, int state);
        ~EventManager() = default;
};
#endif