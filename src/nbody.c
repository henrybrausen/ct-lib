#include <math.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"

#include "bhtree.h"
#include "pool.h"
#include "taskqueue.h"

// N-Body Simulation Example
// Uses naive O(N^2) approach to the N-body problem

#define POOLSIZE 1000
#define NUMBODIES 1000000
#define SIM_DT 0.001
#define MINDIST 1e-3L

struct body {
  double mass;
  double x, y, z;
  double vx, vy, vz;
  double ax, ay, az;
  double axnew, aynew, aznew;
};

struct body bodies[NUMBODIES];

struct pool task_p;
struct pool taskarg_p;

struct taskqueue task_q;

struct bh_tree tree;

// Argument to an nbody task is semi-open range [begin, end)
struct nbody_task_arg {
  size_t begin, end;
};

double rand_double(double min, double max)
{
  return min + (max - min) * ((double)rand() / (double)RAND_MAX);
}

void init_bodies()
{
  srand(time(NULL));

  for (size_t i = 0; i < NUMBODIES; ++i) {
    bodies[i].mass = rand_double(1.0, 10.0);
    bodies[i].x = rand_double(-100.0, 100.0);
    bodies[i].y = rand_double(-100.0, 100.0);
    bodies[i].z = rand_double(-100.0, 100.0);
    bodies[i].vx = bodies[i].vy = bodies[i].vz = 0.0;
    bodies[i].ax = bodies[i].ay = bodies[i].az = 0.0;
    bodies[i].axnew = bodies[i].aynew = bodies[i].aznew = 0.0;
  }
}

void init()
{
  init_bodies();

  pool_init(&task_p, POOLSIZE, sizeof(struct task));
  pool_init(&taskarg_p, POOLSIZE, sizeof(struct nbody_task_arg));
  taskqueue_init(&task_q);
  bh_tree_init(&tree, 10 * NUMBODIES);
}

void bb_update()
{
  double min_coord = bodies[0].x, max_coord = bodies[0].x;
  for (size_t i = 0; i < NUMBODIES; ++i) {
    double min_i = fmin(bodies[i].x, fmin(bodies[i].y, bodies[i].z));
    double max_i = fmax(bodies[i].x, fmax(bodies[i].y, bodies[i].z));
    if (min_i < min_coord) min_coord = min_i;
    if (max_i > max_coord) max_coord = max_i;
  }
  bh_tree_set_bb(&tree, (struct bh_vec3){min_coord, min_coord, min_coord},
                 (struct bh_vec3){max_coord, max_coord, max_coord});
}

void build_tree()
{

  bh_tree_clear(&tree);
  bb_update();
  for (size_t i = 0; i < NUMBODIES; ++i) {
    struct bh_vec3 p = {bodies[i].x, bodies[i].y, bodies[i].z};
    if (bh_tree_insert(&tree, p, bodies[i].mass) != 0) {
      printf("ERROR ON INSERT\n");
      exit(EXIT_FAILURE);
    }
  }
}

void nbody_compute_accel(void *arg)
{
  double dx, dy, dz, temp;
  struct nbody_task_arg *range = (struct nbody_task_arg *)arg;

  for (size_t i = range->begin; i != range->end; ++i) {
    for (size_t j = 0; j < NUMBODIES; ++j) {
      if (i == j) { continue; }
      dx = bodies[j].x - bodies[i].x;
      dy = bodies[j].y - bodies[i].y;
      dz = bodies[j].z - bodies[i].z;

      temp = sqrt(dx * dx + dy * dy + dz * dz);
      if (temp < MINDIST) { temp = MINDIST; }
      temp = temp * temp * temp;
      temp = bodies[j].mass / temp;

      bodies[i].axnew += temp * dx;
      bodies[i].aynew += temp * dy;
      bodies[i].aznew += temp * dz;
    }
  }
}

void nbody_compute_accel_bh(void *arg)
{
  struct nbody_task_arg *range = (struct nbody_task_arg *)arg;
  struct bh_vec3 acc_i;
  struct bh_vec3 p_i;
  for (size_t i = range->begin; i != range->end; ++i) {
    p_i = (struct bh_vec3){bodies[i].x, bodies[i].y, bodies[i].z};
    bh_tree_solve_acc(&tree, &p_i, &acc_i);
    bodies[i].axnew = acc_i.x;
    bodies[i].aynew = acc_i.y;
    bodies[i].aznew = acc_i.z;
  }
}

void nbody_update_pos(void *arg)
{
  struct nbody_task_arg *range = (struct nbody_task_arg *)arg;

  for (size_t i = range->begin; i != range->end; ++i) {
    bodies[i].x += SIM_DT * bodies[i].vx + bodies[i].ax * SIM_DT * SIM_DT / 2.0;
    bodies[i].y += SIM_DT * bodies[i].vy + bodies[i].ay * SIM_DT * SIM_DT / 2.0;
    bodies[i].z += SIM_DT * bodies[i].vz + bodies[i].az * SIM_DT * SIM_DT / 2.0;
  }
}

void nbody_update_vel(void *arg)
{
  struct nbody_task_arg *range = (struct nbody_task_arg *)arg;

  for (size_t i = range->begin; i != range->end; ++i) {
    bodies[i].vx += SIM_DT * (bodies[i].ax + bodies[i].axnew) / 2.0;
    bodies[i].vy += SIM_DT * (bodies[i].ay + bodies[i].aynew) / 2.0;
    bodies[i].vz += SIM_DT * (bodies[i].az + bodies[i].aznew) / 2.0;

    bodies[i].ax = bodies[i].axnew;
    bodies[i].ay = bodies[i].aynew;
    bodies[i].az = bodies[i].aznew;
  }
}

void *nbody_thread_func(void *arg)
{
  struct task *my_task;

  for (;;) {
    my_task = taskqueue_wait_for_work(&task_q);
    my_task->func(my_task->func_args);

    // Return objects to pool
    pool_release(&taskarg_p, my_task->func_args);
    pool_release(&task_p, my_task);
  }
  return (void *)0;
}

void generate_tasks_from_func(size_t num_tasks, void (*task_func)(void *))
{
  struct task *t;
  struct nbody_task_arg *ta;
  size_t bodies_per_task = NUMBODIES / num_tasks;
  for (size_t i = 0; i < NUMBODIES; i += bodies_per_task) {
    if ((t = pool_acquire(&task_p)) == NULL) {
      printf("could not acquire task from pool\n");
      exit(EXIT_FAILURE);
    }
    if ((ta = pool_acquire(&taskarg_p)) == NULL) {
      printf("could not acquire taskarg from pool\n");
      exit(EXIT_FAILURE);
    }
    ta->begin = i;
    ta->end =
        (i + bodies_per_task < NUMBODIES) ? (i + bodies_per_task) : NUMBODIES;
    t->func = task_func;
    t->func_args = ta;
    taskqueue_push(&task_q, t);
  }
}

#define NUMTHREADS 32
#define NUMACCELTASKS 200
#define NUMADVTASKS 3

int main(int argc, char *argv[])
{
  printf("nbody-solver version %d.%d\n", NBODY_VERSION_MAJOR,
         NBODY_VERSION_MINOR);
  init();

  printf("Spawning threads...\n");

  pthread_t tids[NUMTHREADS];

  for (size_t i = 0; i < NUMTHREADS; ++i) {
    int err = pthread_create(&tids[i], NULL, nbody_thread_func, NULL);
    if (err != 0) {
      printf("Cannot create thread %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  printf("Body 0 (x,y,z) = (%f, %f, %f)\n", bodies[0].x, bodies[0].y,
         bodies[0].z);
  for (int i = 0; i < 10; ++i) {
    printf("Iteration #%d\n", i);

    int count;

    printf("Updating positions...\n");
    generate_tasks_from_func(NUMADVTASKS, nbody_update_pos);
    taskqueue_work_ready(&task_q);
    while ((count = taskqueue_count(&task_q)) != 0) {
      // printf("Remaining tasks: %d\n", count);
      // sleep(1);
    }

    printf("Building tree...\n");
    build_tree();

    printf("Computing forces ...\n");
    generate_tasks_from_func(NUMACCELTASKS, nbody_compute_accel_bh);
    taskqueue_work_ready(&task_q);
    while ((count = taskqueue_count(&task_q)) != 0) {
      // printf("Remaining tasks: %d\n", count);
      // sleep(1);
    }

    printf("Updating velocities...\n");
    generate_tasks_from_func(NUMADVTASKS, nbody_update_vel);
    taskqueue_work_ready(&task_q);
    while ((count = taskqueue_count(&task_q)) != 0) {
      // printf("Remaining tasks: %d\n", count);
      // sleep(1);
    }

    printf("Body 0 (x,y,z) = (%f, %f, %f)\n", bodies[0].x, bodies[0].y,
           bodies[0].z);
  }

  return 0;
}
