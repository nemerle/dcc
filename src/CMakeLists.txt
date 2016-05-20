set(dcc_LIB_SOURCES
    CallConvention.cpp
    ast.cpp
    backend.cpp
    bundle.cpp
    chklib.cpp
    comwrite.cpp
    control.cpp
    dataflow.cpp
    disassem.cpp
    DccFrontend.cpp
    error.cpp
    fixwild.cpp
    graph.cpp
    hlicode.cpp
    hltype.cpp
    machine_x86.cpp
    icode.cpp
    RegisterNode
    idioms.cpp
    idioms/idiom1.cpp
    idioms/arith_idioms.cpp
    idioms/call_idioms.cpp
    idioms/epilogue_idioms.cpp
    idioms/mov_idioms.cpp
    idioms/neg_idioms.cpp
    idioms/shift_idioms.cpp
    idioms/xor_idioms.cpp
    locident.cpp
    liveness_set.cpp
    parser.h
    parser.cpp
    procs.cpp
    project.cpp
    Procedure.cpp
    proplong.cpp
    reducible.cpp
    scanner.cpp
    symtab.cpp
    udm.cpp
    BasicBlock.cpp
    dcc_interface.cpp

    MemoryChunk
    MemorySegment
    MemorySegmentCoordinator

    Command.cpp
    Command.h
    Loaders.cpp
    Loaders.h
    FollowControlFlow.cpp
    FollowControlFlow.h

    AutomatedPlanner
)
set(dcc_UI_SOURCES
    ui/DccMainWindow.ui
    ui/DccMainWindow.h
    ui/DccMainWindow.cpp
    ui/FunctionViewWidget.ui
    ui/FunctionViewWidget.h
    ui/FunctionViewWidget.cpp
    ui/FunctionListDockWidget.ui
    ui/FunctionListDockWidget.cpp
    ui/FunctionListDockWidget.h
    ui/RenderTags.cpp
    ui/RenderTags.h
    ui/CommandQueueView.cpp
    ui/CommandQueueView.h
    ui/CommandQueueView.ui
)
set(dcc_HEADERS
    ../include/ast.h
    ../include/bundle.h
    ../include/BinaryImage.h
    ../include/DccFrontend.h
    ../include/Enums.h
    ../include/dcc.h
    ../include/disassem.h
    ../include/dosdcc.h
    ../include/error.h
    ../include/graph.h
    ../include/hlicode.h
    ../include/machine_x86.h
    ../include/icode.h
    ../include/idioms/idiom.h
    ../include/idioms/idiom1.h
    ../include/idioms/arith_idioms.h
    ../include/idioms/call_idioms.h
    ../include/idioms/epilogue_idioms.h
    ../include/idioms/mov_idioms.h
    ../include/idioms/neg_idioms.h
    ../include/idioms/shift_idioms.h
    ../include/idioms/xor_idioms.h
    ../include/locident.h
    ../include/CallConvention.h
    ../include/project.h
    ../include/scanner.h
    ../include/state.h
    ../include/symtab.h
    ../include/types.h
    ../include/Procedure.h
    ../include/StackFrame.h
    ../include/BasicBlock.h
    ../include/dcc_interface.h
)

SOURCE_GROUP(Headers FILES ${dcc_HEADERS})

set(dcc_SOURCES
    dcc.cpp
)

SOURCE_GROUP(Source FILES ${dcc_SOURCES} ${dcc_LIB_SOURCES})

ADD_LIBRARY(dcc_lib STATIC ${dcc_LIB_SOURCES} ${dcc_HEADERS})
qt5_use_modules(dcc_lib Core)

ADD_EXECUTABLE(dcc_original ${dcc_SOURCES} ${dcc_UI_SOURCES})
ADD_DEPENDENCIES(dcc_original dcc_lib)
TARGET_LINK_LIBRARIES(dcc_original dcc_lib dcc_hash disasm_s)
qt5_use_modules(dcc_original Core Widgets)
SET_PROPERTY(TARGET dcc_original PROPERTY CXX_STANDARD 11)
SET_PROPERTY(TARGET dcc_original PROPERTY CXX_STANDARD_REQUIRED ON)



if(dcc_build_tests)
ADD_SUBDIRECTORY(tests)
endif()
