# Cross-compile for Windows x64 from Linux or macOS using llvm-mingw
# (https://github.com/mstorsjo/llvm-mingw): clang + libc++ + mingw-w64 UCRT.
#
# Expects llvm-mingw's bin directory on PATH. Set LLVM_MINGW_ROOT in the
# environment if it is installed somewhere other than /opt/llvm-mingw.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(_triple x86_64-w64-mingw32)

if(DEFINED ENV{LLVM_MINGW_ROOT})
  set(_root "$ENV{LLVM_MINGW_ROOT}")
else()
  set(_root /opt/llvm-mingw)
endif()

set(CMAKE_C_COMPILER   ${_triple}-clang)
set(CMAKE_CXX_COMPILER ${_triple}-clang++)
set(CMAKE_RC_COMPILER  ${_triple}-windres)

# Only look for headers and libraries inside the mingw sysroot, never the host's.
set(CMAKE_FIND_ROOT_PATH "${_root}/${_triple}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Link the runtime (libc++, libunwind, winpthread) statically so the produced
# executables run without shipping llvm-mingw's DLLs next to them.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")

unset(_root)
unset(_triple)
