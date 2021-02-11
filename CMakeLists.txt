cmake_minimum_required(VERSION 3.10)

project(nbody VERSION 0.1)

configure_file(config.h.in config.h)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "Could not find pthread library.")
endif()

set (CMAKE_C_FLAGS "-Wall")

set (CMAKE_C_FLAGS_DEBUG "-O0 -g")
set (CMAKE_C_FLAGS_RELEASE "-O2 -march=native -ftree-vectorize")

set (TARGET_SRC
  src/pool.c
  src/taskqueue.c
  src/barrier.c
  src/threadpool.c
  src/bhtree.c
  src/nbody.c
  )

add_executable(nbody ${TARGET_SRC})

target_include_directories(nbody PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/src"
  )

target_link_libraries(nbody Threads::Threads m)

#add_custom_command (TARGET nbody POST_BUILD
#  COMMAND objdump -CSlr nbody > nbody.s
#  )

