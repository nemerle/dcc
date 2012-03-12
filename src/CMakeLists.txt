SET(dcc_test_SOURCES tests/comwrite.cpp)
include_directories(${GMOCK_INCLUDE_DIRS} ${GMOCK_ROOT}/gtest/include)
enable_testing()
add_executable(tester ${dcc_test_SOURCES})
target_link_libraries(tester
    ${GMOCK_BOTH_LIBRARIES} ${REQ_LLVM_LIBRARIES})
add_test(dcc-tests tester)
