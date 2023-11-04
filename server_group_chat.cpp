#include "server_group_chat.h"
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
pthread_mutex_t epoll_table_clock;
pthread_mutex_t sock_list_lock;
epoll_event events[EPOLL_SIZE];

int sock_fd_erase(std::vector <int> *sock_lst,int target)
{
    auto it = std::find(sock_lst->begin(), sock_lst->end(), target);
if (it != sock_lst->end()) {
    // 找到目标数据

    // 将目标数据从向量中删除
    sock_lst->erase(it);
    return 1;
}
return -1;
}

void forward_data(std::vector <int> *sock_lst,int myself_fd,char * buf)
{
    for(auto it = sock_lst->begin(); it != sock_lst->end();it++)
    {
        int element = *it;
        printf("forward_begin!!!!!! \n");
        if(element != myself_fd)
        {
            
            send(element,buf,strlen(buf),0);
            printf("send to user %d success.\n",myself_fd);
        }
    }
}

void epoll_detele(int epoll_index,int sock_fd)
{
    pthread_mutex_lock(&epoll_table_clock);
    epoll_ctl(epoll_index,EPOLL_CTL_DEL,sock_fd,NULL);
    pthread_mutex_unlock(&epoll_table_clock);
}

int create_socket(sockaddr_in &server_add)
{
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(9527);
    inet_pton(AF_INET,"127.0.0.1",&server_add.sin_addr);
    int server = socket(AF_INET,SOCK_STREAM,0);
    assert(server >= 0);
    socklen_t server_len = sizeof(server_add);
    int ret = bind(server,(struct sockaddr*)&server_add,server_len);
    assert(ret != -1);
    return server;
}

int sock_not_block(int s_fd)
{
        // 获取当前套接字文件描述符的标志位
    int flags = fcntl(s_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("Failed to get socket flags");
        return -1;
    }

    // 设置套接字文件描述符为非阻塞
    if (fcntl(s_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Failed to set socket flags");
        return -1;
    }

    // close(s_fd);
}

int accept_connect(int server_fd)
{
    sockaddr_in client;
    bzero(&client,sizeof(client));
    socklen_t len = sizeof(client);
    int result_fd = accept(server_fd,(struct sockaddr*)&client,&len);
    if(result_fd < 0)
    {
        printf("server connect error!!!!!! \n");
        return -1;
    }
    int relt = sock_not_block(result_fd);
    if(relt < 0)
    {
        printf("socket_fd not block error!!!!!! \n");
        return -1;
    }

    return result_fd;

}

void connect_thread_event(void * arg)
{
    printf("connect_thread_event start.\n");
    struct Args *temp = (struct Args *)arg;
    int sock_fd = temp->fd_id;
    int temp_epoll_id = temp->epoll_table_id;
    int client_fd = accept_connect(sock_fd);
    if(client_fd < 0)
    {
        printf("accept_connectn error!!!!!! \n");
        delete temp;
        return;
    }
    epoll_event ev;
    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

    pthread_mutex_lock(&epoll_table_clock);
    int epoll_add = epoll_ctl(temp_epoll_id,EPOLL_CTL_ADD,client_fd,&ev);
    pthread_mutex_unlock(&epoll_table_clock);

    pthread_mutex_lock(&sock_list_lock);
    temp->sock_fds->push_back(client_fd);
    pthread_mutex_unlock(&sock_list_lock);

    printf("connect sockect_fd: %d \n",client_fd);
    printf("connect success!!!!!!!! \n");
    delete temp;
}

void recv_sock_thread(void * arg)
{
    struct Args *temp = (struct Args *)arg;
    int sock_fd = temp->fd_id;
    int temp_epoll_id = temp->epoll_table_id;
    int ep_index = temp->events_index;
    int temp_server_fd = temp->sever_fd;
    char buffer[BUFFER_SIZE];
    memset(buffer,'\0',BUFFER_SIZE);
    printf("this thread socket_fd: %d \n",sock_fd);
    if(sock_fd >= 0)
    {
        while(1)
        {
            int ret = recv(sock_fd,buffer,BUFFER_SIZE,0);
            //printf("返回值是 %d \n",ret);
            if(ret < 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    continue;
                }
                printf("连接发生错误:  %d\n",errno);
                close(sock_fd);
                pthread_mutex_lock(&sock_list_lock);
                int ret = sock_fd_erase(temp->sock_fds,sock_fd);
                pthread_mutex_unlock(&sock_list_lock);
                if(ret)
                {
                    printf("socket fd delete success!!!!! \n");
                    break;
                }
                printf("socket fd delete fail,delete_fd: %d \n",sock_fd);
                break;
            }
            else if(ret == 0)
            {
                close(sock_fd);
                printf("连接结束 \n");                
                pthread_mutex_lock(&sock_list_lock);
                int ret = sock_fd_erase(temp->sock_fds,sock_fd);
                pthread_mutex_unlock(&sock_list_lock);
                if(ret)
                {
                    printf("socket fd delete success!!!!! \n");
                    break;
                }
                printf("socket fd delete fail,delete_fd: %d \n",sock_fd);
                break;
            }
            else
            {
                printf("recv_data: %s \n",buffer);
                pthread_mutex_lock(&sock_list_lock);
                forward_data(temp->sock_fds,sock_fd,buffer);
                pthread_mutex_unlock(&sock_list_lock);
                memset(buffer,'\0',BUFFER_SIZE);
            }
        }
    }
    else
    {
        pthread_mutex_lock(&epoll_table_clock);
        epoll_ctl(temp_epoll_id,EPOLL_CTL_DEL,sock_fd,NULL);
        events[ep_index].data.fd = -1;
        pthread_mutex_unlock(&epoll_table_clock);
        close(sock_fd);
    }
    delete temp;
    return;
}