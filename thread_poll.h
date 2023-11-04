#include <queue>
#include <pthread.h>
#include <unistd.h>
struct TASK
{
    void * arg;
    void(* funtion)(void * arg);
    /* data */
    TASK(/* args */)
    {
        funtion = nullptr;
        arg = nullptr;
    }
    TASK(void(*f)(void * arg),void * arg)
    {
        funtion = f;
        this->arg = arg;
    }
};

class TASK_Queue
{
private:
        std::queue <TASK> my_T_que;
        pthread_mutex_t m_mutex;
    /* data */
public:
    TASK_Queue(/* args */);
    int add_TASK(TASK task);
    int add_TASK(void(*f)(void * arg),void * arg);
    inline int TASK_count()
    {
     pthread_mutex_lock(&m_mutex);
     int temp = my_T_que.size();
     pthread_mutex_unlock(&m_mutex);
     return temp;
    }
    TASK take_TASK();
    ~TASK_Queue();
};

class thread_poll
{
private:
    /* data */
    TASK_Queue * que;
    pthread_mutex_t poll_mutex;
    int Max_num;
    int Min_num;
    int busy_num;
    int live_num;
    int del_num;
    pthread_t maneger_id;
    pthread_t *woker_ids;
    pthread_cond_t not_empty;
    bool shutdown;
    static void* maneger_thread(void *arg);
    static void * wokers(void *arg);
    static const int COUNTER = 2;

public:
    thread_poll(int max,int min);
    ~thread_poll();
    int thread_poll_add(void(* f)(void * arg),void * arg); 
    // int thread_poll_destroy();
    int thread_poll_alive_num();
    int thread_poll_busy_num();
    int thread_poll_del_worker();
};




