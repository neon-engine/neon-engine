neon_add_functional_test(extensions
        SOURCES extensions.cpp
        LIBRARIES neon-core neon-ryml neon-sdl2 neon-flecs
)

# extensions:// is the folder `extensions` next to the executable, which for
# the test is in the build tree. The extensions the test starts are built
# into it, each a library of its own.
set(TEST_EXTENSIONS "${NEON_TESTS_DIRECTORY}/extensions.test/extensions")

# The recipe of an extension of the test that names the library of this
# platform alone. hello has one that is written by hand, with every platform.
function(neon_add_test_extension NAME SOURCE)
  set(RECIPE "${CMAKE_CURRENT_BINARY_DIR}/${NAME}/extension.yml")
  file(WRITE "${RECIPE}"
          "version: 1\n"
          "name: ${NAME}\n"
          "\n"
          "libraries:\n"
          "  ${NEON_PLATFORM}: ${NAME}-${NEON_PLATFORM}${NEON_EXTENSION_SUFFIX}\n"
  )

  neon_add_extension(${NAME}
          SOURCES ${NAME}/${SOURCE}
          RECIPE "${RECIPE}"
          DIRECTORY "${TEST_EXTENSIONS}"
          EXCLUDE_FROM_ALL
  )
  add_dependencies(extensions.test ${NAME}-extension)
endfunction()

neon_add_extension(hello
        SOURCES hello/hello.c
        RECIPE "${CMAKE_CURRENT_SOURCE_DIR}/hello/extension.yml"
        DIRECTORY "${TEST_EXTENSIONS}"
        EXCLUDE_FROM_ALL
)
add_dependencies(extensions.test hello-extension)

neon_add_test_extension(polite polite.cpp)
neon_add_test_extension(refuses refuses.c)
neon_add_test_extension(from-the-future from-the-future.c)
neon_add_test_extension(no-entry no-entry.c)
neon_add_test_extension(spinner spinner.c)
neon_add_test_extension(old old.c)
neon_add_test_extension(mislaid mislaid.c)
neon_add_test_extension(eager eager.c)
neon_add_test_extension(tumbler tumbler.cpp)
neon_add_test_extension(visitor visitor.cpp)
neon_add_test_extension(painter painter.cpp)
neon_add_test_extension(keeper keeper.cpp)
