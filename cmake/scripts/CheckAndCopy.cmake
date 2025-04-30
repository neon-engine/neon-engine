# Usage:
#   cmake -D SRC="path/to/source.file" -D DST="path/to/dest.file" -P CheckAndCopy.cmake

if (NOT DEFINED SRC)
  message(FATAL_ERROR "SRC variable not defined!")
endif ()
if (NOT DEFINED DST)
  message(FATAL_ERROR "DST variable not defined!")
endif ()

if ((NOT EXISTS "${SRC}") AND (NOT EXISTS "${DST}"))
  message(STATUS "Both source ${SRC} and destination ${DST} do not exist. Skipping.")
  return()
endif ()

if (NOT EXISTS "${DST}")
  message(STATUS "Copying ${SRC} -> ${DST} (destination not found)")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${SRC}" "${DST}")
  return()
endif ()

file(SHA256 "${SRC}" SRC_HASH)
file(SHA256 "${DST}" DST_HASH)

if (NOT "${SRC_HASH}" STREQUAL "${DST_HASH}")
  message(STATUS "Copying ${SRC} -> ${DST} (hash mismatch)")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${SRC}" "${DST}")
endif ()
