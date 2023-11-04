#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "thread_poll.h"
#include "server_group_chat.h"

extern epoll_event events[EPOLL_SIZE];
int main()
{
    struct sockaddr_in server_add;
    server_add.sin_port = 9527;
    int server_fd = create_socket(server_add);
    assert(server_fd >= 0);
    listen(server_fd,5);
    sockaddr_in client;
    bzero(&client,sizeof(client));
    socklen_t len = sizeof(client);
    std::vector <int> sock_fd_list;
    int epoll_fd = epoll_create(1);
    for(int i = 0;i < EPOLL_SIZE;i++)
    {
        events[i].data.fd = -1;
        events[i].events = 0;
    }
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev);
    printf("server id : %d \n",server_fd);
    thread_poll sock_poll(10,3);

    while(1)
    {

        int ret = epoll_wait(epoll_fd,events,EPOLL_SIZE,-1);
        printf("触发事件\n");
        for(int i = 0;i < ret; i++)
        {
            if(events[i].data.fd == server_fd)
            {
                printf("this is server events is : %d \n",i);
                struct Args *server_arg = new struct Args;
                server_arg->epoll_table_id = epoll_fd;
                server_arg->fd_id = server_fd;
                server_arg->sock_fds = &sock_fd_list;
                sock_poll.thread_poll_add(connect_thread_event,server_arg);

            }
            else if (events[i].events & EPOLLIN)
            {
                printf("this is clinet events is : %d \n",i);
                struct Args *client_arg = new struct Args;
                client_arg->epoll_table_id = epoll_fd;
               // client_arg->fd_id = server_fd;
               client_arg->fd_id = events[i].data.fd;
                client_arg->events_index = i;
                client_arg->sever_fd = server_fd;
                client_arg->sock_fds = &sock_fd_list;
                sock_poll.thread_poll_add(recv_sock_thread,client_arg);
                epoll_detele(epoll_fd,events[i].data.fd);
            }
            else{
               printf("other : \n"); 
            }
        }
    }


}