set(SRC
makedsig
fixwild.cpp
LIB_PatternCollector.cpp
LIB_PatternCollector.h
TPL_PatternCollector.cpp
TPL_PatternCollector.h
)
add_executable(makedsig ${SRC})
target_link_libraries(makedsig dcc_hash)
qt5_use_modules(makedsig Core)
