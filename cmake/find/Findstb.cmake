set(_stb_srcdir ${CMAKE_SOURCE_DIR}/external/stb)

if (NOT TARGET stb)
    add_library(stb INTERFACE)
    # As a system header, so that the warnings of the engine stop at it: the
    # implementation of stb is compiled into the file that includes it.
    target_include_directories(stb SYSTEM INTERFACE ${_stb_srcdir})
endif ()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)

find_package_handle_standard_args(
        stb
        REQUIRED_VARS
        _stb_srcdir
)

unset(_stb_srcdir)
