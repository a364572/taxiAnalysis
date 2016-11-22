#include "PthreadPool.h"
#include<iostream>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<sys/time.h>

using namespace std;
static void signal_handle(int sig)
{
    if(sig == SIGALRM)
    {
        cout << "alarm" << endl;
    }
    return;
}
void PthreadPool::submitTask(AbstractTask* t)
{
    //往任务队列提交一个任务后 释放信号量
    while(taskDeque.size() > MAX_PTHREAD_NUMBER)
        ;
    pthread_mutex_lock(&mutex);
    taskDeque.push_back(t);
    pthread_mutex_unlock(&mutex);

    sem_post(&sem);
}

void PthreadPool::waitForFinish()
{
    while(!taskDeque.empty());
    flag = 0;
    for(auto pth : pthread_array)
    {
        pthread_join(pth, NULL);
    }
}

void * PthreadPool::run(void * args)
{
    PthreadPool *pool = static_cast<PthreadPool *>(args);
    //获取信号量 然后从任务队列取出一个任务
    
    struct timespec ts;
    struct timeval tv;
    while(pool->flag || !pool->taskDeque.empty())
    {
        gettimeofday(&tv, NULL);
        ts.tv_sec = tv.tv_sec + 5;
        ts.tv_nsec = 0;
        if(sem_timedwait(&pool->sem, &ts) < 0)
        {
            continue;
        }
        
        pthread_mutex_lock(& (pool->mutex));
        auto task = pool->taskDeque.front();
        pool->taskDeque.pop_front();
        pthread_mutex_unlock(& (pool->mutex));

        task->run();
        delete task;
    }
    return NULL;
}

PthreadPool::PthreadPool()
{
    struct sigaction sigact;
    memset((char *)&sigact, 0, sizeof(sigact));
    sigact.sa_handler = signal_handle;
    sigact.sa_restorer = nullptr;
    sigact.sa_flags = SA_NOMASK;
    sigaction(SIGALRM, &sigact, NULL);
    //信号量初始化 线程初始化
    sem_init(&sem, 0, 0);
    mutex = PTHREAD_MUTEX_INITIALIZER;
    flag = 1;
    for(auto &pth : pthread_array)
    {
        //创建是将this传入
        pthread_create(&pth, NULL, run, this);
    }
}

PthreadPool::~PthreadPool()
{
    sem_destroy(&sem);
}
