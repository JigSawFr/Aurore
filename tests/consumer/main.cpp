// Built against an installed Aurore: the headers, the library and its
// dependencies come from find_package(Aurore)
#include <aurore.hpp>

#include <cstdio>

int main()
{
    std::printf("Aurore %s\n", AURORE_VERSION);
    brls::Logger::info("linked with {}", "Aurore::aurore");
    return 0;
}
