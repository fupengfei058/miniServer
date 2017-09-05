#define FD_SIZE 1024
#define MAX_EVENTS 256

int server_start();

void epoll_register(int epoll_fd, int fd, int state);

void epoll_cancel(int epoll_fd, int fd, int state);