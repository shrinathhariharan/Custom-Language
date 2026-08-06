#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include <memory>
#include <stdexcept>
#include <vector>

class Parser
{
public:
    explicit Parser(std::vector<Token> toks);

    std::vector<std::unique_ptr<Stmt>> parse();

private:
    std::vector<Token> tokens{};
    std::size_t current{};

    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;

    bool check(const std::string& text) const;
    bool match(const std::string& text);
    const Token& advance();
    const Token& consume(const std::string& text, const std::string& message);
    Token consumeIdentifier(const std::string& message);
    std::runtime_error error(const std::string& message) const;
    void expectNoSemicolon();

    DataType parseType();
    bool isTypeKeyword() const;

    std::unique_ptr<Stmt> statement();
    std::unique_ptr<BlockStmt> block();
    std::unique_ptr<Stmt> declStatement(bool checkSemicolons);
    std::unique_ptr<Stmt> printStatement();
    std::unique_ptr<Stmt> ifStatement();
    std::unique_ptr<Stmt> whileStatement();
    std::unique_ptr<Stmt> forStatement();
    std::unique_ptr<Stmt> functionStatement();
    std::unique_ptr<Stmt> classStatement();
    std::unique_ptr<Stmt> returnStatement();
    std::unique_ptr<Stmt> importStatement();
    std::unique_ptr<Stmt> assignmentOrExpressionStatement(bool checkSemicolons);

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();
};

#endif // PARSER_H
