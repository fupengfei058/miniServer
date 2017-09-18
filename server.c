#include "./include/server.h"
#include "./include/cJSON.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

/**
 * 创建套接字描述符
 */
int server_start() {
	int sock_fd;
	struct sockaddr_in server_addr;
    int flags;

    server_addr.sin_family = AF_INET; // 协议族是IPV4协议
    server_addr.sin_port = htons(PORT); // 将主机的无符号短整形数转换成网络字节顺序
    server_addr.sin_addr.s_addr = inet_addr(HOST); // 用inet_addr函数将字符串host转为int型

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    flags = fcntl(sock_fd, F_GETFL, 0);
    flags |= O_NONBLOCK; //设置成非阻塞

    if(fcntl(sock_fd, F_SETFL, flags)) {
        perror("set_flag_error");
        exit(1);
    }

    if (bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind_error");
        exit(1);
    }

    if (listen(sock_fd, REQUEST_QUEUE_LENGTH) == -1) {
        perror("listen error");
        exit(1);
    }

    return sock_fd;
}

/**
 * 注册epoll事件
 * 
 * @param epoll_fd epoll句柄
 * @param fd 	   socket句柄
 * @param state    监听状态
 *
 */
void epoll_register(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

/**
 * 注销epoll事件
 *
 * @param epoll_fd epoll句柄
 * @param fd       socket句柄
 * @param state    监听状态
 */
void epoll_cancel(int epoll_fd, int fd, int state) {
    struct epoll_event event;
    event.events = state;
    event.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
}

/**
 * 处理客户端连接
 *
 * @param server_fd
 * @param epoll_fd
 */
void accept_client(int server_fd, int epoll_fd) {
	int client_fd;
	struct sockaddr_in client_addr;
    socklen_t len_client_addr = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &len_client_addr);
    epoll_register(epoll_fd, client_fd, EPOLLIN);
}

/**
 * 处理客户端请求
 *
 * @param client_fd
 * @param epoll_fd
 */
void deal_client(int client_fd, int epoll_fd) {
	char *response_content;
	char http_request[BUFF_SIZE], response_header[BUFF_SIZE], http_response[BUFF_SIZE];
	memset(http_request, 0, BUFF_SIZE);
	recv(client_fd, http_request, BUFF_SIZE, 0);

	if (strlen(http_request) == 0) {
        epoll_cancel(epoll_fd, client_fd, EPOLLIN);
        return;
    }

    response_content = deal_request(http_request, client_fd);
    sprintf(response_header, header_tmpl, strlen(response_content));
    sprintf(http_response, "%s%s", response_header, response_content);
    send(client_fd, http_response, sizeof(http_response), 0);
}

/**
 * 处理请求参数
 *
 * @param request_content
 * @param client_fd
 */
char *deal_request(char *request_content, int client_fd) {
	struct sockaddr_in client_addr;
	socklen_t len_client_addr = sizeof(client_addr);
    struct request cgi_request;
    parse_request(request_content, &cgi_request);
    cgi_request.SERVER_PORT = PORT;
    getpeername(client_fd, (struct sockaddr *) &client_addr, &len_client_addr); //获取对方地址
    inet_ntop(AF_INET, &client_addr.sin_addr, cgi_request.REMOTE_ADDR, sizeof(cgi_request.REMOTE_ADDR)); //将网络字节序二进制值转换成点分十进制串
    char *request_json = create_json(&cgi_request);
    char *response_json = exec_php(request_json);

    // todo parse json and build http_response
    return response_json;
}

/**
 * request
 *
 */
void parse_request(char *http_request, struct request *cgi_request) {
	char request_str[2048];
    char top_body[2048];
    int header_len;
    char *delimiter = "\n";
    char *line;

    strcpy(request_str, http_request);

    line = strtok(http_request, delimiter);
    sscanf(line, "%s%s%s", cgi_request->REQUEST_METHOD, top_body, cgi_request->SERVER_PROTOCOL);

    memset(cgi_request->SCRIPT_NAME, 0, strlen(cgi_request->SCRIPT_NAME));
    for (int i = 0; i < strlen(top_body); i++) {
        if (top_body[i] == '?') {
            strncpy(cgi_request->SCRIPT_NAME, top_body, i);
            strncpy(cgi_request->QUERY_STRING, top_body + i + 1, strlen(top_body) - i);
            break;
        }
    }

    if (0 == strlen(cgi_request->SCRIPT_NAME)) {
        strcpy(cgi_request->SCRIPT_NAME, top_body);
        memset(cgi_request->QUERY_STRING, 0, strlen(cgi_request->QUERY_STRING));
    }

    while ((line = strtok(NULL, delimiter))) {
        parse_line(line, cgi_request);
    }
}

void parse_line(char *line, struct request *cgi_request) {

}

char *create_json(struct request *cgi_request) {
	cJSON *root;
    root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "REQUEST_METHOD", cgi_request->REQUEST_METHOD);
    cJSON_AddStringToObject(root, "SCRIPT_NAME", cgi_request->SCRIPT_NAME);
    cJSON_AddStringToObject(root, "SERVER_PROTOCOL", cgi_request->SERVER_PROTOCOL);
    cJSON_AddStringToObject(root, "SERVER_NAME", cgi_request->SERVER_NAME);
    cJSON_AddStringToObject(root, "QUERY_STRING", cgi_request->QUERY_STRING);
    cJSON_AddStringToObject(root, "REMOTE_ADDR", cgi_request->REMOTE_ADDR);
    cJSON_AddStringToObject(root, "CONTENT_TYPE", cgi_request->CONTENT_TYPE);
    cJSON_AddStringToObject(root, "POST_DATA", cgi_request->POST_DATA);
    cJSON_AddNumberToObject(root, "SERVER_PORT", cgi_request->SERVER_PORT);
    cJSON_AddNumberToObject(root, "CONTENT_LENGTH", cgi_request->CONTENT_LENGTH);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

/**
 * response
 *
 */
void parse_json(char *response_json, struct response *cgi_response) {
    cJSON *json = cJSON_Parse(response_json);

    cgi_response.CONTENT_TYPE = cJSON_GetObjectItem(cJSON, "CONTENT_TYPE");
    cgi_response.HTTP_CODE = cJSON_GetObjectItem(cJSON, "HTTP_CODE");
    cgi_response.RESPONSE_CONTENT = cJSON_GetObjectItem(cJSON, "RESPONSE_CONTENT");

    return;
}

/**
 * 执行PHP脚本
 *
 * @param args
 */
char *exec_php(char *args) {
	char command[BUFF_SIZE] = "php index.php ";
	FILE *fp;
	static char buff[BUFF_SIZE];
	char line[BUFF_SIZE];
	strcat(command, args);
	memset(buff, 0, BUFF_SIZE);
	if((fp = popen(command, "r")) == NULL) {
		strcpy(buff, "服务器内部错误");
	} else {
		while (fgets(line, BUFF_SIZE, fp) != NULL) {
			strcat(buff, line);
		}
	}
	return buff;
}

int main() {
    int i;
    int event_num;
    int server_fd;
    int epoll_fd;
    int fd;
    struct epoll_event events[MAX_EVENTS];

    server_fd = server_start();
    epoll_fd = epoll_create(FD_SIZE);
    epoll_register(epoll_fd, server_fd, EPOLLIN | EPOLLET);// 这里注册socketEPOLL事件为ET模式

    while (1) {
        event_num = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);
        for (i = 0; i < event_num; i++) {
            fd = events[i].data.fd;
            if ((fd == server_fd) && (events[i].events == EPOLLIN)) {
                accept_client(server_fd, epoll_fd);
            } else if (events[i].events == EPOLLIN) {
                deal_client(fd, epoll_fd);
            } else if (events[i].events == EPOLLOUT)
                continue;
        }
    }
}