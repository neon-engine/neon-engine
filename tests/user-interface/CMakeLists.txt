# The user interface as an application puts it together: files in YAML, the
# layout engine, and the system that shows them. Drawing goes to a renderer
# that keeps what it is asked to draw, and text to a font whose characters
# are all the same box, so that nothing here needs a graphics card.
neon_add_functional_test(user-interface
        SOURCES
        ui-fixture.hpp
        ui-binding.cpp
        ui-drawing.cpp
        ui-events.cpp
        ui-files.cpp
        ui-input.cpp
        ui-layout.cpp
        ui-scenes.cpp
        LIBRARIES neon-core neon-ryml
)
