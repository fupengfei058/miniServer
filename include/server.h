#define FD_SIZE 1024
#define MAX_EVENTS 256
#define BUFF_SIZE 1024

int server_start();

void epoll_register(int epoll_fd, int fd, int state);

void epoll_cancel(int epoll_fd, int fd, int state);

void accept_client(int server_fd, int epoll_fd);

void deal_client(int client_fd, int epoll_fd);

char *deal_request(char *request_content, int client_fd);

char *create_json(struct request *cgi_request);

char *exec_php(char *args);