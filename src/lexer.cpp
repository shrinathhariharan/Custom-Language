#include "lexer.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

std::vector<Token> tokenize(const std::string& source)
{
    std::vector<Token> tokens{};
    std::size_t line{1};

    for (std::size_t i{0}; i < source.size();)
    {
        const char c{source[i]};
        if (c == '\n')
        {
            ++line;
            ++i;
        }
        else if (std::isspace(static_cast<unsigned char>(c)))
            ++i;
        else if (c == '/' && i + 1 < source.size() && source[i + 1] == '/')
        {
            while (i < source.size() && source[i] != '\n')
                ++i;
        }
        else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            std::string text{};
            while (i < source.size() && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_'))
                text += source[i++];

            static const std::vector<std::string> keywords{
                "int", "dec", "str", "bool", "true", "false", "print", "if",
                "else", "loop", "while", "for", "func", "return", "input", "break", "continue", "void", "class", "import"
            };
            tokens.push_back({std::find(keywords.begin(), keywords.end(), text) == keywords.end() ? TokenType::identifier : TokenType::keyword, text, line});
        }
        else if (std::isdigit(static_cast<unsigned char>(c)) ||
                 (c == '.' && i + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[i + 1]))))
        {
            std::string text{};
            bool sawDot{false};
            while (i < source.size() && (std::isdigit(static_cast<unsigned char>(source[i])) || source[i] == '.'))
            {
                if (source[i] == '.')
                {
                    if (sawDot)
                        throw std::runtime_error("Line " + std::to_string(line) + ": malformed number");
                    sawDot = true;
                }
                text += source[i++];
            }
            tokens.push_back({TokenType::number, text, line});
        }
        else if (c == '"')
        {
            std::string text{};
            ++i;
            while (i < source.size() && source[i] != '"')
            {
                if (source[i] == '\n')
                    ++line;
                if (source[i] == '\\' && i + 1 < source.size())
                {
                    switch (source[i + 1])
                    {
                    case 'n': text += '\n'; break;
                    case 't': text += '\t'; break;
                    case '"': text += '"'; break;
                    case '\\': text += '\\'; break;
                    default: text += source[i + 1]; break;
                    }
                    i += 2;
                }
                else
                    text += source[i++];
            }
            if (i >= source.size())
                throw std::runtime_error("Line " + std::to_string(line) + ": unterminated string");
            ++i;
            tokens.push_back({TokenType::string_lit, text, line});
        }
        else
        {
            std::string text{c};
            if (i + 1 < source.size())
            {
                const std::string two{source.substr(i, 2)};
                if (two == "==" || two == "!=" || two == ">=" || two == "<=" ||
                    two == "+=" || two == "-=" || two == "*=" || two == "/=" ||
                    two == "&&" || two == "||")
                    text = two;
            }
            tokens.push_back({TokenType::symbol, text, line});
            i += text.size();
        }
    }

    tokens.push_back({TokenType::eof_token, "", line});
    return tokens;
}
