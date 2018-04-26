#define PORT 8080
#define HOST "127.0.0.1"
#define FD_SIZE 1024
#define MAX_EVENTS 256
#define BUFF_SIZE 1024
#define REQUEST_QUEUE_LENGTH 10
#define FCGI_HOST "127.0.0.1"
#define FCGI_PORT 9000      // php-fpm监听的端口地址

char *header_tmpl = "HTTP/1.1 200 OK\r\n"
        "Server: ZBS's Server V1.0\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Content-Type: text/html\r\n\r\n";

struct request {
    char REQUEST_METHOD[4];
    char SCRIPT_NAME[64];
    char SERVER_PROTOCOL[8];
    char SERVER_NAME[32];
    int SERVER_PORT;
    char QUERY_STRING[2048];
    char REMOTE_ADDR[16];
    char CONTENT_TYPE[16];
    char POST_DATA[2048];
    int CONTENT_LENGTH;
} request;

int server_start();

void epoll_register(int epoll_fd, int fd, int state);

void epoll_cancel(int epoll_fd, int fd, int state);

void accept_client(int server_fd, int epoll_fd);

void deal_client(int client_fd, int epoll_fd);

char *deal_request(char *request_content, int client_fd);

char *create_json(struct request *cgi_request);

char *exec_php(char *args);

void parse_line(char *line, struct request *cgi_request);

void parse_request(char *http_request, struct request *cgi_request);