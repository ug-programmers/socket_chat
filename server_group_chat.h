#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sys/epoll.h>
#define EPOLL_SIZE 1024
#define BUFFER_SIZE 1024
struct Args
{
    int epoll_table_id;
    int fd_id;
    int events_index;
    int sever_fd;
    std::vector <int>* sock_fds;
};
int sock_fd_erase(std::vector <int> *sock_lst,int target);
void forward_data(std::vector <int> *sock_lst,int myself_fd,char * buf);
void epoll_detele(int epoll_index,int sock_fd);
int create_socket(sockaddr_in &server_add);
int sock_not_block(int s_fd);
int accept_connect(int server_fd);
void connect_thread_event(void * arg);
void recv_sock_thread(void * arg);