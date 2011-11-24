#CC = gcc -g -O -D__UNIX__
PROJECT(dcc_original)
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_BUILD_TYPE Debug)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D__UNIX__)
INCLUDE_DIRECTORIES(include  ${Boost_INCLUDE_DIRS})
if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D__UNIX__ -D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(/W4)
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall --std=c++0x")
endif()

set(dcc_SOURCES
src/dcc.cpp
src/ast.cpp
src/backend.cpp
src/bundle.cpp
src/chklib.cpp
src/comwrite.cpp
src/control.cpp
src/dataflow.cpp
src/disassem.cpp
src/error.cpp
src/fixwild.cpp
src/frontend.cpp
src/graph.cpp
src/hlicode.cpp
src/icode.cpp
src/idioms.cpp
src/locident.cpp
src/parser.cpp
src/perfhlib.cpp
src/procs.cpp
src/proplong.cpp
src/reducible.cpp
src/scanner.cpp
src/symtab.cpp
src/udm.cpp
src/BasicBlock.cpp
)
set(dcc_HEADERS
    include/ast.h
    include/bundle.h
    include/dcc.h
    include/disassem.h
    include/dosdcc.h
    include/error.h
    include/graph.h
    include/hlicode.h
    include/icode.h
    include/locident.h
    include/perfhlib.h
    include/scanner.h
    include/state.h
    include/symtab.h
    include/types.h
    include/Procedure.h
    include/StackFrame.h
    include/BasicBlock.h
)
ADD_EXECUTABLE(dcc_original ${dcc_SOURCES} ${dcc_HEADERS})

