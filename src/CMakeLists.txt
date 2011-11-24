SET(SOURCES ast.cpp backend.cpp bundle.cpp chklib.cpp 
comwrite.cpp control.cpp dataflow.cpp dcc.cpp     
disassem.cpp error.cpp fixwild.cpp frontend.cpp
graph.cpp  hlicode.cpp icode.cpp 
idioms.cpp locident.cpp parser.cpp        
perfhlib.cpp procs.cpp proplong.cpp reducible.cpp     
scanner.cpp symtab.cpp udm.cpp)
SET(dc_INCLUDES
${PROJECT_SOURCE_DIR}/include/ast.h
${PROJECT_SOURCE_DIR}/include/bundle.h
${PROJECT_SOURCE_DIR}/include/dcc.h
${PROJECT_SOURCE_DIR}/include/disassem.h
${PROJECT_SOURCE_DIR}/include/dosdcc.h
${PROJECT_SOURCE_DIR}/include/error.h
${PROJECT_SOURCE_DIR}/include/graph.h
${PROJECT_SOURCE_DIR}/include/hlicode.h
${PROJECT_SOURCE_DIR}/include/icode.h
${PROJECT_SOURCE_DIR}/include/locident.h
${PROJECT_SOURCE_DIR}/include/perfhlib.h
${PROJECT_SOURCE_DIR}/include/scanner.h
${PROJECT_SOURCE_DIR}/include/state.h
${PROJECT_SOURCE_DIR}/include/symtab.h
${PROJECT_SOURCE_DIR}/include/types.h
)
SOURCE_GROUP(Source FILES ${SOURCES})
SOURCE_GROUP(Headers FILES ${dc_INCLUDES}) 
ADD_EXECUTABLE(dcc_oo ${SOURCES} ${dc_INCLUDES})