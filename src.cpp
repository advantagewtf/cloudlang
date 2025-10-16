#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "parser.h"
// todo: probably fix this looking very ugly xd
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    system("cls");

    std::string filename = argv[1];
    std::ifstream file(filename);
    std::stringstream buffer;
    
    buffer << file.rdbuf();
    std::string cld_program = buffer.str();
    
    parser cld;
    
    cld.run(cld_program);

    // std::cin.get();
    return 0;
}
