#include "PthreadPool.h"

void PthreadPool::submitTask(AbstractTask* t)
{
    //往任务队列提交一个任务后 释放信号量
    pthread_mutex_lock(&mutex);
    taskDeque.push_back(t);
    pthread_mutex_unlock(&mutex);

    sem_post(&sem);
}

void PthreadPool::waitForFinish()
{
    sem_destroy(&sem);
    for(auto &pth : pthread_array)
    {
        pthread_join(pth, NULL);
    }
}

void * PthreadPool::run(void * args)
{
    PthreadPool *pool = static_cast<PthreadPool *>(args);
    //获取信号量 然后从任务队列取出一个任务
    while(sem_wait(& pool->sem) == 0)
    {
        pthread_mutex_lock(& pool->mutex);
        auto &task = pool->taskDeque.front();
        pool->taskDeque.pop_front();
        pthread_mutex_unlock(& pool->mutex);

        task->run();
        delete task;
    }
    return NULL;
}

PthreadPool::PthreadPool()
{
    //信号量初始化 线程初始化
    sem_init(&sem, 0, 0);
    mutex = PTHREAD_MUTEX_INITIALIZER;
    for(auto &pth : pthread_array)
    {
        //创建是将this传入
        pthread_create(&pth, NULL, run, this);
    }
}

