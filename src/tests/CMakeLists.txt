set(TESTS
    ProjectTests
    LoaderTests
    MemoryChunkTests
    MemorySegmentCoordinatorTests
)


include(DCC_Macros)

set(target_INCLUDE_DIR
    ..
)
include_directories(${target_INCLUDE_DIR}
    ../../frontend/sparc
    ../../frontend/pentium
)

set(test_LIBRARIES
dcc_lib dcc_hash disasm_s
)
foreach(t ${TESTS})
    ADD_QTEST(${t})
endforeach()
