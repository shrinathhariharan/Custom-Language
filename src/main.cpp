#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string readWholeFile(const std::string& fileName)
{
    std::ifstream filePath{fileName};
    std::stringstream buffer{};
    buffer << filePath.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        std::cerr << "Error: No file specified. Usage: ./custom_language <file.txt>\n";
        return 1;
    }

    const std::string fileName{argv[1]};
    if (!std::filesystem::exists(fileName))
    {
        std::cerr << "Error: Could not find or open file: " << fileName << "\n";
        return 1;
    }

    try
    {
        Parser parser{tokenize(readWholeFile(fileName))};
        auto program{parser.parse()};

        Environment env{};
        for (const auto& statement : program)
            statement->exec(env);
    }
    catch (const std::exception& e)
    {
        std::cout << "COMPILER -- Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
