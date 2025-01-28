
// hello.cpp
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 2) { // Check if there is exactly one command-line argument
        std::cerr << "Usage: " << argv[0] << " <number>" << std::endl;
        return 1;
    }

    std::cout << "Hello world " << argv[1] << std::endl; // Convert argument to integer
    return 0;
}