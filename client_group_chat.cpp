#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "thread_poll.h"
#include <iostream>
#define EPOLL_SIZE 1024
#define BUFFER_SIZE 1024
epoll_event client_events[EPOLL_SIZE];
int create_client_fd(sockaddr_in * c_addr)
{
    int clinet = socket(PF_INET,SOCK_STREAM,0);
    bzero(c_addr,sizeof(*c_addr));
    c_addr->sin_family = AF_INET;
    c_addr->sin_port = htons(9527);
    inet_pton(AF_INET,"127.0.0.1",&c_addr->sin_addr);
    printf("clinet is %d \n",clinet);
    //bind(clinet, (struct sockaddr *)c_addr, sizeof( struct sockaddr_in));
    assert( clinet > 0);
    return clinet;
}
void *recv_other(void * arg)
{
    char buffer[BUFFER_SIZE];
    memset(buffer,'\0',BUFFER_SIZE);
    int recv_fd = *(int *)arg;
    if(recv_fd < 0)
    {
        return NULL;
    }
    while (1)
    {
        int ret = recv(recv_fd,buffer,BUFFER_SIZE,0);
        if(ret == -1)
        {
            printf("recv error!! \n");
            return NULL;
        }
        else if(ret == 0)
        {

        }
        else
        {
            printf("group chat: %s \n",buffer);
            memset(buffer,'\0',BUFFER_SIZE);
        }
    }
    
}
int main()
{
    sockaddr_in client_addr;
    int client = create_client_fd(&client_addr);
    socklen_t len = sizeof(client_addr);
    int epoll_table = epoll_create(1);
    if (epoll_table == -1) {
        perror("epoll_create1");
        return -1;
    }
    epoll_event ev;
    ev.data.fd = 0;
    ev.events = EPOLLIN;
    for(int i = 0; i < EPOLL_SIZE;i++)
    {
        client_events[i].data.fd = -1;
        client_events[i].events = 0;
    }
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }
    if (epoll_ctl(epoll_table, EPOLL_CTL_ADD, 0, &ev) == -1) {
        perror("epoll_ctl");
        close(epoll_table);
        return -1;
    }
    ev.data.fd = client;
    ev.events = EPOLLIN;

    if (epoll_ctl(epoll_table, EPOLL_CTL_ADD, client, &ev) == -1) {
        perror("epoll_ctl");
        close(epoll_table);
        return -1;
    }
    pthread_t thread_id;
    int ret = connect(client,(struct sockaddr*)&client_addr,len);
    assert(ret != -1);
    while(1)
    {
        int IO_count = epoll_wait(epoll_table,client_events,EPOLL_SIZE,-1);
        for(int i = 0;i < IO_count;i++)
        {
            if(client_events[i].data.fd == 0 && client_events[i].events & EPOLLIN)
            {
                int result = splice(STDIN_FILENO,NULL,pipefd[1],NULL,1024, SPLICE_F_MOVE);
                if(result == -1)
                {
                   perror("splice1");
                   close(0);
                   break; 
                }
                result = splice(pipefd[0],NULL,client,NULL,1024, SPLICE_F_MOVE);
                
                if(result == -1)
                {
                   perror("splice2");
                   close(0);
                   break; 
                }
                printf("client: send success!!!!!! \n");
            }
            else if(client_events[i].events & EPOLLIN)
            {
                pthread_create(&thread_id,NULL,recv_other,&client_events[i].data.fd);
                epoll_ctl(epoll_table,EPOLL_CTL_DEL,client,NULL);
                printf("create thread success!!!!!! \n");
            }
            
        }
    }
    close(client);

}