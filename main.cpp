#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string getName(std::string_view statement = "")
{
    std::cout << statement;
    std::string name{};
    std::getline(std::cin, name);
    return name;
}

std::string getValidFile(int argc, char** argv)
{
    if (argc > 1)
    {
        std::string fileName{argv[1]};
        std::ifstream filePath{fileName};
        if (filePath.is_open())
            return fileName;
        std::cerr << "Error: Could not open file specified in arguments: " << fileName << "\n";
    }

    while (true)
    {
        std::string fileName{getName("Enter your file path for cus language here (.txt): ")};
        std::ifstream filePath{fileName};

        if (!filePath.is_open())
            std::cerr << "Error: Could not open the file\n";
        else
            return fileName;
    }
}

std::string readWholeFile(const std::string& fileName)
{
    std::ifstream filePath{fileName};
    std::stringstream buffer{};
    buffer << filePath.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv)
{
    try
    {
        const std::string fileName{getValidFile(argc, argv)};
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
