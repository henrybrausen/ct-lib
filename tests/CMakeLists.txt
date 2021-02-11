
# strip -DNDEBUG from flags in release modes
foreach (string_to_scrub
  CMAKE_C_FLAGS_RELEASE
  CMAKE_C_FLAGS_RELWITHDEBINFO
  CMAKE_C_FLAGS_MINSIZEREL)
  string (REGEX REPLACE
    "(^| )[/-]D *NDEBUG($| )"
    " "
    "${string_to_scrub}"
    "${${string_to_scrub}}"
  )
endforeach()

add_executable(queue_test queue_test.c)
target_link_libraries(queue_test ct_lib)
add_test(queue queue_test)

add_executable(threadpool_pps_test threadpool_pps_test.c)
target_link_libraries(threadpool_pps_test ct_lib)
add_test(threadpool_pps threadpool_pps_test)
