#include "thread_pool.h"
#include <pthread.h>
#include "malloc.h"
#include <unistd.h>
#include "time.h"

#ifdef THPOOL_DEBUG
#define THPOOL_DEBUG 1
#else
#define THPOOL_DEBUG 0
#endif

#if !defined(DISABLE_PRINT) || defined(THPOOL_DEBUG)
#define err(str) fprintf(stderr, str)
#else
#define err(str)
#endif

#define ALLOC(a) malloc(a)
// Structures
typedef struct event
{
    int counter;
    pthread_cond_t event_cond;
    pthread_mutex_t event_mtx;
} event;

typedef struct task
{
    struct task *prev;
    void *arg;
    void (*function)(void *arg);
} task;

typedef struct task_queue
{
    // sync
    pthread_mutex_t rwlock;

    event *has_task;

    task *front;
    task *back;
    volatile size_t size;
} task_queue;

typedef struct thread
{
    size_t id;
    pthread_t thread_ptr;
    struct thread_pool *pool;
} thread;

typedef struct thread_pool
{
    thread **threads;
    pthread_mutex_t th_count_mtx;
    pthread_cond_t thread_end_wait;
    task_queue queue;

    volatile size_t count_th_alive;
    volatile size_t count_th_working;

    volatile int stop;
} thread_pool;

// Prototypes

static int task_queue_init(task_queue *queue);
static void task_queue_clear(task_queue *queue);
static void task_queue_push(task_queue *queue, task *new_task);
static task *task_queue_pop(task_queue *queue);
static void task_queue_destroy(task_queue *queue);

static int thread_init(thread_pool_t pool, thread **thr, size_t id);
static void thread_exec(thread *thr);
static void thread_destroy(thread *thr);

static void event_init(event *ev);
static void event_reset(event *ev);
static void event_wait(event *ev);
static void event_notify(event *ev);
static void event_notify_all(event *ev);
static void event_deinit(event *ev);
static void event_destroy(event *ev);

// Impl
thread_pool_t create_thread_pool(size_t thread_count)
{
    if (thread_count < 0)
    {
        thread_count = 0;
    }

    thread_pool_t th_pool = NULL;
    th_pool = (thread_pool_t)ALLOC(sizeof(thread_pool));
    if (th_pool == NULL)
    {
        err("create_thread_pool(): failed to allocate memory for thread pool");
        return NULL;
    }

    th_pool->count_th_alive = 0;
    th_pool->count_th_working = 0;
    th_pool->stop = 0;

    if (task_queue_init(&th_pool->queue) == -1)
    {
        err("create_thread_pool(): failed to allocate memory for task queue");
        return NULL;
    }

    th_pool->threads = (thread **)ALLOC(thread_count * sizeof(thread *));
    if (th_pool->threads == NULL)
    {
        err("create_thread_pool(): failed to allocate memory for threads");
        return NULL;
    }

    pthread_mutex_init(&th_pool->th_count_mtx, NULL);
    pthread_cond_init(&th_pool->thread_end_wait, NULL);

    for (size_t i = 0; i < thread_count; ++i)
    {
        thread_init(th_pool, &th_pool->threads[i], i);
    }

    return th_pool;
}

int add_task_thread_pool(thread_pool_t th_pool, void (*function_p)(void *), void *arg)
{
    task *new_task;

    new_task = (task *)ALLOC(sizeof(task));

    if (new_task == NULL)
    {
        err("add_task_thread_pool(): failed to allocate memory for new task");
        return -1;
    }

    new_task->function = function_p;
    new_task->arg = arg;

    task_queue_push(&th_pool->queue, new_task);
    return 0;
}

void wait_thread_pool(thread_pool_t th_pool)
{
    pthread_mutex_lock(&th_pool->th_count_mtx);
    while (th_pool->queue.size || th_pool->count_th_working)
    {
        pthread_cond_wait(&th_pool->thread_end_wait, &th_pool->th_count_mtx);
    }
    pthread_mutex_unlock(&th_pool->th_count_mtx);
}

void destroy_thread_pool(thread_pool_t th_pool)
{
    if (th_pool == NULL)
        return;

    volatile int num_threads = th_pool->count_th_alive;
    th_pool->stop = 1;

    double TIMEOUT = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    time(&start);
    while (tpassed < TIMEOUT && th_pool->count_th_alive)
    {
        event_notify_all(th_pool->queue.has_task);
        time(&end);
        tpassed = difftime(end, start);
    }

    while (th_pool->count_th_alive)
    {
        event_notify_all(th_pool->queue.has_task);
        sleep(1);
    }

    pthread_mutex_destroy(&th_pool->th_count_mtx);
    pthread_cond_destroy(&th_pool->thread_end_wait);

    task_queue_destroy(&th_pool->queue);

    for (int i = 0; i < num_threads; ++i)
    {
        thread_destroy(th_pool->threads[i]);
    }

    free(th_pool->threads);
    free(th_pool);
}

static int task_queue_init(task_queue *queue)
{
    queue->size = 0;
    queue->front = NULL;
    queue->back = NULL;

    queue->has_task = (event *)ALLOC(sizeof(struct event));
    if (queue->has_task == NULL)
    {
        return -1;
    }

    event_init(queue->has_task);

    pthread_mutex_init(&(queue->rwlock), NULL);
    return 0;
}

static void task_queue_clear(task_queue *queue)
{
    for (int i = 0; i < queue->size; ++i)
    {
        free(task_queue_pop(queue));
    }

    queue->front = NULL;
    queue->back = NULL;
    queue->size = 0;
    event_reset(queue->has_task);
}

static void task_queue_push(task_queue *queue, task *new_task)
{
    pthread_mutex_lock(&(queue->rwlock));
    new_task->prev = NULL;

    switch (queue->size)
    {
    case 0:
        queue->front = new_task;
        queue->back = new_task;
        break;

    default:
        queue->back->prev = new_task;
        queue->back = new_task;
        break;
    }

    queue->size++;
    event_notify(queue->has_task);

    pthread_mutex_unlock(&(queue->rwlock));
}

static task *task_queue_pop(task_queue *queue)
{
    pthread_mutex_lock(&(queue->rwlock));

    task *pop_task = queue->front;

    switch (queue->size)
    {
    case 0:
        break;
    case 1:
        queue->front = 0;
        queue->back = 0;
        queue->size--;
        break;

    default:
        queue->front = queue->front->prev;
        queue->size--;
        event_notify(queue->has_task);
        break;
    }

    pthread_mutex_unlock(&(queue->rwlock));
    return pop_task;
}

static void task_queue_destroy(task_queue *queue)
{
    task_queue_clear(queue);
    event_destroy(queue->has_task);
    pthread_mutex_destroy(&(queue->rwlock));
}

static int thread_init(thread_pool_t pool, thread **thr, size_t id)
{
    *thr = (thread *)ALLOC(sizeof(thread));
    if (*thr == NULL)
    {
        err("thread_init(): Could not allocate memory for thread\n");
        return -1;
    }
    (*thr)->pool = pool;
    (*thr)->id = id;

    pthread_create(&(*thr)->thread_ptr, NULL, (void *(*)(void *))thread_exec, *thr);
    return 0;
}

static void thread_exec(thread *thr)
{
    thread_pool_t pool = thr->pool;

    pthread_mutex_lock(&pool->th_count_mtx);
    pool->count_th_alive++;
    pthread_mutex_unlock(&pool->th_count_mtx);

    while (!pool->stop)
    {
        event_wait(pool->queue.has_task);

        // cppcheck-suppress oppositeInnerCondition
        if (pool->stop)
            break;

        pthread_mutex_lock(&pool->th_count_mtx);
        pool->count_th_working++;
        pthread_mutex_unlock(&pool->th_count_mtx);

        void (*func)(void *);

        task *task_p = task_queue_pop(&pool->queue);
        if (task_p)
        {
            func = task_p->function;
            void *arg = task_p->arg;
            func(arg);
            free(task_p);
        }

        pthread_mutex_lock(&pool->th_count_mtx);
        pool->count_th_working--;
        if (!pool->count_th_working)
            pthread_cond_signal(&pool->thread_end_wait);
        pthread_mutex_unlock(&pool->th_count_mtx);
    }

    pthread_mutex_lock(&pool->th_count_mtx);
    pool->count_th_alive--;
    pthread_mutex_unlock(&pool->th_count_mtx);
}

static void thread_destroy(thread *thr)
{
    pthread_join(thr->thread_ptr, NULL);
    free(thr);
}

static void event_init(event *ev)
{
    pthread_mutex_init(&ev->event_mtx, NULL);
    pthread_cond_init(&ev->event_cond, NULL);

    event_reset(ev);
}

static void event_reset(event *ev)
{
    ev->counter = 0;
}

static void event_wait(event *ev)
{
    pthread_mutex_lock(&ev->event_mtx);

    while (ev->counter != 1)
    {
        pthread_cond_wait(&ev->event_cond, &ev->event_mtx);
    }
    ev->counter = 0;
    pthread_mutex_unlock(&ev->event_mtx);
}

static void event_notify(event *ev)
{
    pthread_mutex_lock(&ev->event_mtx);
    ev->counter = 1;
    pthread_cond_signal(&ev->event_cond);
    pthread_mutex_unlock(&ev->event_mtx);
}

static void event_notify_all(event *ev)
{
    pthread_mutex_lock(&ev->event_mtx);
    ev->counter = 1;
    pthread_cond_broadcast(&ev->event_cond);
    pthread_mutex_unlock(&ev->event_mtx);
}

static void event_deinit(event *ev)
{
    pthread_mutex_destroy(&(ev->event_mtx));
    pthread_cond_destroy(&(ev->event_cond));
}

static void event_destroy(event *ev)
{
    event_deinit(ev);
    free(ev);
}