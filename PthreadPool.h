#ifndef __PTHREADPOOL_H_
#define __PTHREADPOOL_H_
//线程池 模板类需要实现run方法

#include<pthread.h>
#include<semaphore.h>
#include<deque>
#include "TaxiTask.h"

#define MAX_PTHREAD_NUMBER 20

class PthreadPool
{
public:
    sem_t sem;                      //任务同步
    pthread_mutex_t mutex;          //互斥量获取任务 
    std::deque<AbstractTask *> taskDeque;        //任务队列
    pthread_t pthread_array[MAX_PTHREAD_NUMBER];
    int flag;

    PthreadPool();

    void submitTask(AbstractTask *t);
    void waitForFinish();

    static void *run(void *);

    virtual ~PthreadPool();
};

#endif
