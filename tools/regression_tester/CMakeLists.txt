set(SRC
regression_tester.cpp
)
add_executable(regression_tester ${SRC})
target_link_libraries(regression_tester Qt5::Core)
