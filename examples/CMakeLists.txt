add_executable(pool_example pool_example.c)
target_link_libraries(pool_example ct_lib)

add_executable(sum_example sum_example.c)
target_link_libraries(sum_example ct_lib)

if (NOT TARGET examples)
  add_custom_target(examples)
  add_dependencies(examples pool_example sum_example)
endif()

