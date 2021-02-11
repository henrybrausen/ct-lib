
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

