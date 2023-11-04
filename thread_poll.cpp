#include "thread_poll.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
TASK_Queue::TASK_Queue()
{
    pthread_mutex_init(&m_mutex,NULL);
}
TASK_Queue::~TASK_Queue()
{
    pthread_mutex_destroy(&m_mutex);
}
int TASK_Queue::add_TASK(TASK task)
{
    pthread_mutex_lock(&m_mutex);
    my_T_que.push(task);
    printf("push task success!!!!! \n");
    pthread_mutex_unlock(&m_mutex);
    return 1;
}
int TASK_Queue::add_TASK(void(*f)(void * arg),void * arg)
{
    pthread_mutex_lock(&m_mutex);
    my_T_que.push(TASK(f,arg));
    printf("push task success!!!!! \n");
    pthread_mutex_unlock(&m_mutex);
    return 1;
}
TASK TASK_Queue::take_TASK()
{
    TASK t;
    pthread_mutex_lock(&m_mutex);
    if(my_T_que.size() > 0)
    {
    t = my_T_que.front();
    my_T_que.pop();
    }
    pthread_mutex_unlock(&m_mutex);
    return t;
}

thread_poll::thread_poll(int max,int min)
{
    do
    {
        que = new TASK_Queue;
        shutdown = false;
        if (que == nullptr)
        {
            shutdown = true;
            break;
        }
        woker_ids = new pthread_t [max];
        if(que == nullptr)
        {
            shutdown = true;
            break;
        }
        memset(woker_ids,0,sizeof(pthread_t) * max);
        if (pthread_mutex_init(&poll_mutex,NULL) != 0 || pthread_cond_init(&not_empty,NULL) != 0)
        {
            shutdown = true;
            break;
        }
        pthread_create(&maneger_id,NULL,maneger_thread,this);
        printf("maneger_thread success!!!!! \n");
        for(int i = 0; i < min; i++)
        {
            pthread_create(&woker_ids[i],NULL,wokers,this);
            printf("woker_thread_id: %ld  create!!!!!! \n",woker_ids[i]);

        }
        Max_num = max;
        Min_num = min;
        live_num = min;
        busy_num = 0;
        del_num = 0;
        return;
        /* code */
    } while (0);
    
    if(woker_ids) delete []woker_ids;
    if(que) delete que;    

}

void *thread_poll::maneger_thread (void * arg)
{
    thread_poll *poll = (thread_poll *)arg;
    if(poll == nullptr)
    {
        printf("create maneger_thread error!!!!!");
        return NULL;
    }
    while(!poll->shutdown)
    {
    sleep(3);
    
    if(poll->que->TASK_count() > poll->live_num && !poll->shutdown)
    {
        pthread_mutex_lock(&poll->poll_mutex);
        int count = 0;
        for(int i = 0; count < poll->COUNTER && i < poll->Max_num && poll->live_num < poll->Max_num;i++)
        {
            if(poll->woker_ids[i] == 0)
            {
                pthread_create(&poll->woker_ids[i],NULL,wokers,poll);
                printf("woker_thread_id: %ld  create!!!!!! \n",poll->woker_ids[i]);
                count++;
                poll->live_num++;
            }

        }
        pthread_mutex_unlock(&poll->poll_mutex);
    }
    if(poll->busy_num * 2 < poll->live_num && poll->live_num > poll->Min_num)
    {
     pthread_mutex_lock(&poll->poll_mutex);
     poll->del_num = COUNTER;
     for(int i = 0; i < COUNTER; i++)
     {
        pthread_cond_signal(&poll->not_empty); 
     }
     pthread_mutex_unlock(&poll->poll_mutex);
    }
    }
    return NULL;
}

void* thread_poll::wokers(void * arg)
{
    thread_poll *poll = (thread_poll *)arg;
    if(poll == nullptr)
    {
        printf("create maneger_thread error!!!!!");
        return NULL;
    }
    while(1)
    {
        pthread_mutex_lock(&poll->poll_mutex);
        while(poll->que->TASK_count() == 0 && !poll->shutdown)
        {
            pthread_cond_wait(&poll->not_empty,&poll->poll_mutex);
            if(poll->del_num > 0)
            {
                poll->del_num--;
                if(poll->live_num > poll->Min_num)
                {
                    poll->live_num--;
                    pthread_mutex_unlock(&poll->poll_mutex);
                    poll->thread_poll_del_worker();
                }
            }
        }
        if(poll->shutdown)
        {
            pthread_mutex_unlock(&poll->poll_mutex);
            poll->thread_poll_del_worker();
        }
        TASK task = poll->que->take_TASK();
        poll->busy_num++;
        pthread_mutex_unlock(&poll->poll_mutex);
        printf("woker_thread_id : %ld ,task start........\n",pthread_self());
        task.funtion(task.arg);
        // printf("woker_thread_id : %ld ,task end........\n",pthread_self());
        // delete task.arg;
        // task.arg = NULL;

        pthread_mutex_lock(&poll->poll_mutex);
        poll->busy_num--;
        pthread_mutex_unlock(&poll->poll_mutex);
    }
}

int thread_poll::thread_poll_del_worker()
{
    pthread_t t = pthread_self();
    for(int i = 0;i < Max_num;i++)
    {
        if(woker_ids[i] == t)
        {
            woker_ids[i] = 0;
            break;
        }
    }
    printf("woker_thread_id :%ld  delete!!!!! \n",t);
    // pthread_join(t,NULL);
    pthread_exit(NULL);

}

int thread_poll::thread_poll_add(void(* f)(void * arg),void * arg)
{
    this->que->add_TASK(f,arg);
    pthread_cond_signal(&this->not_empty);
    printf("push task end!!!!! \n");
    return 1;
}

int thread_poll::thread_poll_alive_num()
{
    pthread_mutex_lock(&poll_mutex);
    int temp = live_num;
    pthread_mutex_unlock(&poll_mutex);
    return temp;
}

int thread_poll::thread_poll_busy_num()
{
    pthread_mutex_lock(&poll_mutex);
    int temp = busy_num;
    pthread_mutex_unlock(&poll_mutex);
    return temp;
}

thread_poll::~thread_poll()
{
    shutdown = true;
    pthread_join(maneger_id,NULL);
    for(int i = 0;i < live_num;i++)
    {
        pthread_cond_signal(&not_empty);
    }
    if(que)
    {
        delete que;
    }
    if(woker_ids)
    {
        delete []woker_ids;
    }
    pthread_cond_destroy(&not_empty);
    pthread_mutex_destroy(&poll_mutex);
}