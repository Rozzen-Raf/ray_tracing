#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "types.h"
struct thread_pool;

typedef struct thread_pool *thread_pool_t;
#ifdef __cplusplus
extern "C"
{
#endif
    /// @brief Creates a thread pool with the specified number of threads
    ///
    /// @param thread_count Number of threads in the thread pool
    ///
    /// @return Returns a pointer to the thread pool. If an error occurred during creation, it returns NULL
    thread_pool_t create_thread_pool(size_t thread_count);

    /// @brief Destroys the thread pool and frees the resources allocated for its operation
    ///
    /// @param th_pool thread pool for destruction
    void destroy_thread_pool(thread_pool_t th_pool);

    /// @brief Add task to the thread pool
    /// @param th_pool threadpool to which the work will be added
    /// @param function_p pointer to function to add as work
    /// @param arg pointer to an argument
    /// @return 0 on success, -1 otherwise
    int add_task_thread_pool(thread_pool_t th_pool, void (*function_p)(void *), void *arg);

    void wait_thread_pool(thread_pool_t th_pool);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif