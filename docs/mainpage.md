N-Body Solver {#mainpage}
=========================

## Introduction
This project implements a fast, parallel n-body solver in C, along with a number of useful threading / parallelization constructs.

## Threading constructs
The major abstraction provided by the threading constructs in this library is the [threadpool](@ref threadpool).

The threadpool manages a collection of worker threads, which consume work from a task queue. The basic scheme is illustrated below:

| Basic Operation of threadpool |
| :---- |
| ![](img/threadpool.png) |

In this diagram, the main application thread calls [threadpool_push_task()](@ref threadpool_push_task) to queue up a number of tasks for execution.

The threadpool manages a number of worker threads. Each worker thread consumes tasks from the list of queued tasks. The tasks are then executed in parallel.

The main application thread also pushes a synchronization barrier onto the queue by calling [threadpool_push_barrier()](@ref threadpool_push_barrier).
A synchronization barrier requires that all threads reach the barrier before any given thread is allowed to continue consuming work from the queue.

By default, the worker threads are asleep, and will not consume tasks from the queue. To begin task execution, the application code calls [threadpool_notify()](@ref threadpool_notify) to wake up the worker threads and begin task execution.

Finally, the application code calls [threadpool_wait()](@ref threadpool_wait) to block until all tasks are completed.

## Examples
- [Parallel Array Sum](@ref sum_example.c)

