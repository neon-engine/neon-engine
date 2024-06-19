
#include <iostream>
#include <stdexcept>
#include <core/application.hpp>

int main()
{
    core::Application app{};

    try
    {
        app.Run();
    } catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
