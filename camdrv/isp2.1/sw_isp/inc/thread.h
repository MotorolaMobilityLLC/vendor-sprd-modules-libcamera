#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_THREAD_NUM	4

#define MULTI_THREAD

typedef struct worker
{
	void *(*process) (void *arg);
	void *arg;
	struct worker *next;

} CThread_worker;

typedef struct {
	int core;
	void *pool;
} thread_arg;

typedef struct
{
	pthread_mutex_t queue_lock;
	pthread_cond_t  queue_ready;
	pthread_mutex_t pro_lock;
	pthread_cond_t  pro_ready;

	CThread_worker *queue_head;
	int shutdown;
	pthread_t *threadid;
	int *thread_createflag;
	int max_thread_num;
	int cur_queue_size;
	pthread_attr_t  *attr;

	int thread_done;

	thread_arg args[MAX_THREAD_NUM];
} CThread_pool;
#ifdef __cplusplus
extern "C" {
#endif
int sprd_pool_add_worker(void *pool, void *(*process) (void *arg), void *arg);
int sprd_pool_init(CThread_pool **pool_ptr, int threadNum);
int sprd_pool_destroy(CThread_pool *pool);

void sprd_sync_thread_done(void *arg, int sync_num);
void sprd_reset_thread_done(void *arg);
void sprd_one_thread_done(void *arg);
#ifdef __cplusplus
}
#endif
#endif
