add_executable(parselib parsehdr.cpp)

target_link_libraries(parselib dcc_hash)
qt5_use_modules(parselib Core)
