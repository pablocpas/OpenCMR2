# Applies PATCH to the current directory (FetchContent's patch step) unless it
# is already applied, as after a repeated configure of the same source.
execute_process(COMMAND git apply --ignore-whitespace --reverse --check "${PATCH}" RESULT_VARIABLE applied OUTPUT_QUIET ERROR_QUIET)
if(applied EQUAL 0)
    return()
endif()
execute_process(COMMAND git apply --ignore-whitespace --whitespace=nowarn "${PATCH}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Could not apply ${PATCH}")
endif()
