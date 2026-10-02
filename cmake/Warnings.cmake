# Warnings of the engine's own code.
#
# neon_warnings(<target>) makes the compiler report everything that -Wall and
# -Wextra cover, and stops the build on any of it. The flags are set on each
# target of the engine, as PRIVATE options, and never on anything under
# external/: those libraries are built as they come, and their warnings are not
# ours to fix. A header of theirs that the engine includes is marked as a
# system header where it is declared, which keeps its warnings out as well.
# Every toolchain of the project is clang, so the flags are the same on every
# platform.
#
# Two categories of -Wextra are off:
#
# -Wunused-parameter: the interfaces of neon-core give their methods a default
# body that does nothing, and the names of the parameters there are the
# documentation of the method. Backends and tests do the same when an argument
# does not matter to them. clang-tidy's readability-named-parameter is off for
# the same reason.
#
# -Wmissing-designated-field-initializers: a designated initializer leaves out
# the fields that keep their default on purpose, that is what it is for.
function(neon_warnings TARGET)
  target_compile_options(${TARGET} PRIVATE
          -Wall
          -Wextra
          -Werror
          -Wno-unused-parameter
          -Wno-missing-designated-field-initializers
  )
endfunction()
