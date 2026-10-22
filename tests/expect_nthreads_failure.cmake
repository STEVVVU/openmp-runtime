if(NOT DEFINED PROGRAM)
    message(FATAL_ERROR "PROGRAM must name the unsupported-team-size fixture")
endif()

execute_process(
    COMMAND "${PROGRAM}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr)

if(result STREQUAL "0")
    message(FATAL_ERROR "test_nthreads unexpectedly accepted changing team size")
endif()

set(output "${stdout}${stderr}")
string(FIND "${output}" "Altering the number of threads is not implemented. (Changing from 1 to 2)" expected_failure)
if(expected_failure EQUAL -1)
    message(FATAL_ERROR "test_nthreads failed for an unexpected reason: ${output}")
endif()

message(STATUS "test_nthreads confirmed the documented unsupported 1-to-2 team-size change")
