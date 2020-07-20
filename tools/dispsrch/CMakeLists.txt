add_executable(dispsig dispsig)
target_link_libraries(dispsig dcc_hash Qt5::Core)

add_executable(srchsig srchsig)
target_link_libraries(srchsig dcc_hash dcc_lib Qt5::Core)

