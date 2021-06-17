cmake_minimum_required(VERSION 3.13.4)


if (GB_TEST_SANITY)
    add_executable(test_sanity test_sanity.c)
    target_link_libraries(test_sanity LINK_PRIVATE TotalGB)
endif()

if (GB_TEST_BENCH)
    add_executable(test_bench test_bench.c)
    target_link_libraries(test_bench LINK_PRIVATE TotalGB)
endif()
