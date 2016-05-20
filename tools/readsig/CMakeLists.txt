add_executable(readsig readsig.cpp)

target_link_libraries(readsig dcc_hash)
qt5_use_modules(readsig Core)
