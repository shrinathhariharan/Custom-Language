#ifndef LEXER_H
#define LEXER_H

#include <cstddef>
#include <string>
#include <vector>

enum class TokenType
{
    identifier,
    number,
    string_lit,
    symbol,
    keyword,
    eof_token,
};

struct Token
{
    TokenType type{};
    std::string text{};
    std::size_t line{};
};

std::vector<Token> tokenize(const std::string& source);

#endif // LEXER_H
