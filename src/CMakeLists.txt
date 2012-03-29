SET(dcc_test_SOURCES
    tests/comwrite.cpp
    tests/project.cpp
    tests/loader.cpp

)
include_directories(${GMOCK_INCLUDE_DIRS} ${GMOCK_ROOT}/gtest/include)
add_executable(tester ${dcc_test_SOURCES})
ADD_DEPENDENCIES(tester dcc_lib)

target_link_libraries(tester dcc_lib disasm_s
    ${GMOCK_BOTH_LIBRARIES} ${REQ_LLVM_LIBRARIES})
add_test(dcc-tests tester)
