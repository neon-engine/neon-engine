# What the pbr shader makes of light, read pixel by pixel from what the
# runtime saved. Needs a graphics card with a Vulkan driver. On a machine
# without one the tests are skipped, they do not fail.
neon_add_application_test(runtime-pbr NeonRuntime run.cmake
        TOOLS
        pixel-probe
        CASES
        a-matte-dielectric-scatters-nearly-all-the-light
        a-matte-metal-spreads-the-light-and-scatters-none
        a-red-dielectric-reflects-white
)
