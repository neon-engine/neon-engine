# Possession, run without a window, see docs/possession.md. On the
# possession scene of the sandbox: the character walks onto the plate of the
# cart and possesses it, the view jumps to the camera on the cart, `move`
# drives the cart and nothing else, and `jump` possesses the character again
# where it stands; S drives the cart back, and A and D turn it. The default
# scene of the sandbox possesses nothing and draws its box all the same.
# Read from the log, from where the cart is in each frame, and pixel by
# pixel from what was saved. And on the blockout of the game of the tests
# and on the museum, whose players carry no Possessable and are possessed
# all the same, once, and walk. Needs a graphics card with a Vulkan driver.
# On a machine without one the tests are skipped, they do not fail.

neon_add_application_test(runtime-possession NeonEditor run.cmake
        PROJECT sandbox
        TOOLS
        pixel-probe
        CASES
        the-character-possesses-the-cart-and-gets-off
        the-cart-drives-back-and-turns
        the-sandbox-possesses-nothing
)

neon_add_application_test(runtime-possession-without-the-tag NeonEditor run.cmake
        PROJECT test-game
        CASES
        the-player-of-a-scene-without-the-tag-is-possessed
)

neon_add_application_test(runtime-possession-museum NeonEditor run.cmake
        PROJECT museum
        CASES
        the-museum-player-is-possessed-once-and-walks
)
