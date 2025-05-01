set(_glad_SourceDir ${CMAKE_SOURCE_DIR}/external/glad)

if (NOT TARGET glad)
  add_library(glad STATIC
          ${_glad_SourceDir}/include/glad/gl.h
          ${_glad_SourceDir}/include/glad/gles2.h
          ${_glad_SourceDir}/include/KHR/khrplatform.h
          ${_glad_SourceDir}/src/gl.c
          ${_glad_SourceDir}/src/gles2.c
  )
  target_include_directories(glad PUBLIC
          ${_glad_SourceDir}/include
  )
endif ()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)

find_package_handle_standard_args(
        glad
        REQUIRED_VARS
        _glad_SourceDir
)

unset(_glad_SourceDir)
