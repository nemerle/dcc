MACRO(ADD_UNIT_TEST name)
        IF(NOT ${name}_TEST_VISITED)
                # add the loader as a dll
                ADD_EXECUTABLE(${name} ${ARGN})
                qt5_use_modules(${name} Core)
                MESSAGE(WARNING "Adding test " ${name} " " ${ARGN})
                TARGET_LINK_LIBRARIES(${name} ${UNIT_TEST_LIBS})
                ADD_TEST(NAME ${name} COMMAND ${name})
                set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT DCC_TEST_BASE=${PROJECT_SOURCE_DIR})
                SET(${name}_TEST_VISITED true)
        ENDIF()
ENDMACRO()

function(ADD_QTEST NAME)
  add_executable(${NAME} ${NAME}.cpp ${NAME}.h) #${PROTO_SRCS} ${PROTO_HDRS}
  target_link_libraries(${NAME} ${test_LIBRARIES})
  qt5_use_modules(${NAME} Core Test)

  add_test( NAME ${NAME} COMMAND $<TARGET_FILE:${NAME}>)
  set_property(TEST ${NAME} APPEND PROPERTY ENVIRONMENT DCC_TEST_BASE=${PROJECT_SOURCE_DIR})
endfunction()
