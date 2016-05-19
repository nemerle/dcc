add_executable(dispsig dispsig)

target_link_libraries(dispsig dcc_hash)
qt5_use_modules(dispsig Core)

add_executable(srchsig srchsig)

target_link_libraries(srchsig dcc_hash dcc_lib)
qt5_use_modules(srchsig Core)
