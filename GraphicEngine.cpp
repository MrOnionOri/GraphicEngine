#include "Sandbox/SandboxApplication.h"

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        SandboxApplication application;
        application.run();
    } catch (const std::exception& error) {
        std::cerr << "Error fatal: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
