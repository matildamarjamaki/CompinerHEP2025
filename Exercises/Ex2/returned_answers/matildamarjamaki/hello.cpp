
// hello.cpp
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <number>" << std::endl;
        return 1;
    }

    std::cout << "Hello world " << argv[1] << std::endl;
    return 0;
}