execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --target ${TARGET}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

set(diagnostic "${output}\n${error}")
if(result EQUAL 0)
    message(FATAL_ERROR "${TARGET} compiled successfully")
endif()
if(NOT diagnostic MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR "${TARGET} did not emit the expected diagnostic:\n${diagnostic}")
endif()
if(diagnostic MATCHES "${REJECTED_REGEX}")
    message(FATAL_ERROR "${TARGET} continued instantiating after validation:\n${diagnostic}")
endif()
