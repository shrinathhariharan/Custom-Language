#include "parser.h"
#include <stdexcept>
#include <utility>

Parser::Parser(std::vector<Token> toks) : tokens{std::move(toks)} {}

std::vector<std::unique_ptr<Stmt>> Parser::parse()
{
    std::vector<std::unique_ptr<Stmt>> statements{};
    while (!isAtEnd())
        statements.push_back(statement());
    return statements;
}

const Token& Parser::peek() const { return tokens[current]; }
const Token& Parser::previous() const { return tokens[current - 1]; }
bool Parser::isAtEnd() const { return peek().type == TokenType::eof_token; }

bool Parser::check(const std::string& text) const { return !isAtEnd() && peek().text == text; }

bool Parser::match(const std::string& text)
{
    if (!check(text))
        return false;
    ++current;
    return true;
}

const Token& Parser::advance()
{
    if (!isAtEnd())
        ++current;
    return previous();
}

const Token& Parser::consume(const std::string& text, const std::string& message)
{
    if (check(text))
        return advance();
    throw error(message);
}

Token Parser::consumeIdentifier(const std::string& message)
{
    if (peek().type == TokenType::identifier)
        return advance();
    throw error(message);
}

std::runtime_error Parser::error(const std::string& message) const
{
    return std::runtime_error("Line " + std::to_string(peek().line) + ": " + message);
}

void Parser::expectNoSemicolon()
{
    if (check(";"))
        throw error("semicolons are not allowed");
}

DataType Parser::parseType()
{
    if (match("int")) return DataType::t_int;
    if (match("dec")) return DataType::t_dec;
    if (match("str")) return DataType::t_str;
    if (match("bool")) return DataType::t_bool;
    throw error("expected type keyword");
}

bool Parser::isTypeKeyword() const
{
    const std::string& t{peek().text};
    return (peek().type == TokenType::keyword) &&
           (t == "int" || t == "dec" || t == "str" || t == "bool");
}

std::unique_ptr<Stmt> Parser::statement()
{
    if (isTypeKeyword())  return declStatement(true);
    if (peek().type == TokenType::identifier && current + 1 < tokens.size() && tokens[current + 1].type == TokenType::identifier)
    {
        const Token className{advance()};
        const Token name{consumeIdentifier("expected variable name after class name")};
        consume("=", "expected '=' after object variable name");
        auto initializer{expression()};
        expectNoSemicolon();
        return std::make_unique<ObjectDeclStmt>(className.text, name.text, std::move(initializer), className.line);
    }
    if (check("print"))   return printStatement();
    if (check("if"))      return ifStatement();
    if (check("while"))   return whileStatement();
    if (check("for"))     return forStatement();
    if (check("func"))    return functionStatement();
    if (check("class"))   return classStatement();
    if (check("return"))  return returnStatement();
    if (check("import"))  return importStatement();
    if (match("break")) { expectNoSemicolon(); return std::make_unique<BreakStmt>(previous().line); }
    if (match("continue")) { expectNoSemicolon(); return std::make_unique<ContinueStmt>(previous().line); }
    if (check("{"))       return block();
    return assignmentOrExpressionStatement(true);
}

std::unique_ptr<BlockStmt> Parser::block()
{
    const std::size_t line{consume("{", "expected '{'").line};
    std::vector<std::unique_ptr<Stmt>> statements{};
    while (!check("}") && !isAtEnd())
        statements.push_back(statement());
    consume("}", "expected '}' after block");
    return std::make_unique<BlockStmt>(std::move(statements), line);
}

std::unique_ptr<Stmt> Parser::declStatement(bool checkSemicolons)
{
    const std::size_t line{peek().line};
    DataType type{parseType()};

    const std::string name{consumeIdentifier("expected variable name after type").text};
    auto stmt{std::make_unique<DeclStmt>(type, name, line)};

    if (match("["))
    {
        consume("]", "expected ']' after '[' in array declaration");
        stmt->isDynamic = true;
        consume("=", "expected '=' after '[]'");
        consume("{", "expected '{' for array initializer");
        if (!check("}"))
        {
            do { stmt->arrayItems.push_back(expression()); }
            while (match(","));
        }
        consume("}", "expected '}' after array initializer");
    }
    else
    {
        if (match("="))
            stmt->initializer = expression();
    }

    if (checkSemicolons)
        expectNoSemicolon();
    return stmt;
}

std::unique_ptr<Stmt> Parser::printStatement()
{
    const std::size_t line{consume("print", "expected 'print'").line};
    consume("(", "expected '(' after print");
    auto expr{expression()};
    consume(")", "expected ')' after print expression");
    expectNoSemicolon();
    return std::make_unique<PrintStmt>(std::move(expr), line);
}

std::unique_ptr<Stmt> Parser::ifStatement()
{
    const std::size_t line{consume("if", "expected 'if'").line};
    consume("(", "expected '(' after if");
    auto condition{expression()};
    consume(")", "expected ')' after if condition");
    auto thenBlock{block()};
    std::unique_ptr<Stmt> elseBranch{};
    if (match("else"))
        elseBranch = check("if") ? ifStatement() : block();
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBranch), line);
}

std::unique_ptr<Stmt> Parser::whileStatement()
{
    const std::size_t line{consume("while", "expected 'while'").line};
    consume("(", "expected '(' after while");
    auto condition{expression()};
    consume(")", "expected ')' after while condition");
    return std::make_unique<WhileStmt>(std::move(condition), block(), line);
}

std::unique_ptr<Stmt> Parser::forStatement()
{
    const std::size_t line{consume("for", "expected 'for'").line};
    consume("(", "expected '(' after for");
    auto initializer{isTypeKeyword() ? declStatement(false) : assignmentOrExpressionStatement(false)};
    consume(",", "expected ',' after for initializer");
    auto condition{expression()};
    consume(",", "expected ',' after for condition");
    auto increment{assignmentOrExpressionStatement(false)};
    consume(")", "expected ')' after for increment");
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), block(), line);
}

std::unique_ptr<Stmt> Parser::functionStatement()
{
    const std::size_t line{consume("func", "expected 'func'").line};
    const bool returnsVoid{match("void")};
    if (!returnsVoid && isTypeKeyword())
        parseType();
    const std::string name{consumeIdentifier("expected function name after 'func'").text};
    consume("(", "expected '(' after function name");

    std::vector<std::string> params{};
    if (!check(")"))
    {
        do
        {
            if (isTypeKeyword())
                parseType();
            params.push_back(consumeIdentifier("expected parameter name").text);
        }
        while (match(","));
    }
    consume(")", "expected ')' after function parameters");
    return std::make_unique<FunctionDefStmt>(name, std::move(params), block(), line, returnsVoid);
}

std::unique_ptr<Stmt> Parser::classStatement()
{
    const std::size_t line{consume("class", "expected 'class'").line};
    const std::string name{consumeIdentifier("expected class name").text};
    consume("{", "expected '{' after class name");
    auto result{std::make_unique<ClassDefStmt>(name, line)};
    while (!check("}") && !isAtEnd())
    {
        if (isTypeKeyword()) { result->fields.emplace_back(static_cast<DeclStmt*>(declStatement(true).release())); continue; }
        if (check("func")) { auto method{static_cast<FunctionDefStmt*>(functionStatement().release())}; result->methods.emplace(method->name, std::unique_ptr<FunctionDefStmt>{method}); continue; }
        if (check(name))
        {
            advance(); consume("(", "expected '(' after constructor name"); std::vector<std::string> params{};
            if (!check(")")) do { params.push_back(consumeIdentifier("expected constructor parameter").text); } while (match(","));
            consume(")", "expected ')' after constructor parameters"); result->constructor = std::make_unique<FunctionDefStmt>(name, std::move(params), block(), line, true); continue;
        }
        throw error("expected a field, constructor, or method in class body");
    }
    consume("}", "expected '}' after class body"); return result;
}

std::unique_ptr<Stmt> Parser::returnStatement()
{
    const std::size_t line{consume("return", "expected 'return'").line};
    std::unique_ptr<Expr> val{};
    if (check(";"))
        throw error("semicolons are not allowed");
    if (!check("}") && !isAtEnd())
        val = expression();
    expectNoSemicolon();
    return std::make_unique<ReturnStmt>(std::move(val), line);
}

std::unique_ptr<Stmt> Parser::importStatement()
{
    const std::size_t line{consume("import", "expected 'import'").line};
    std::string fileName{};
    if (peek().type == TokenType::string_lit || peek().type == TokenType::identifier)
        fileName = advance().text;
    else
        throw error("expected file path or module name after 'import'");
    expectNoSemicolon();
    return std::make_unique<ImportStmt>(fileName, line);
}

std::unique_ptr<Stmt> Parser::assignmentOrExpressionStatement(bool checkSemicolons)
{
    if (peek().type == TokenType::identifier)
    {
        const std::size_t saved{current};
        const Token name{advance()};
        std::unique_ptr<Expr> index{};
        if (match("["))
        {
            index = expression();
            consume("]", "expected ']' after array index");
        }
        if (match("."))
        {
            const Token member{consumeIdentifier("expected member name after '.'")};
            if (match("=") || match("+=") || match("-=") || match("*=") || match("/=") || match("%=")) { const std::string op{previous().text}; auto value{expression()}; if (checkSemicolons) expectNoSemicolon(); return std::make_unique<MemberAssignStmt>(name.text, member.text, op, std::move(value), name.line); }
        }
        if (match("=") || match("+=") || match("-=") || match("*=") || match("/=") || match("%="))
        {
            const std::string op{previous().text};
            auto value{expression()};
            if (checkSemicolons)
                expectNoSemicolon();
            return std::make_unique<AssignStmt>(name.text, op, std::move(value), std::move(index), name.line);
        }
        current = saved;
    }

    const std::size_t line{peek().line};
    auto expr{expression()};
    if (checkSemicolons)
        expectNoSemicolon();
    return std::make_unique<ExprStmt>(std::move(expr), line);
}

std::unique_ptr<Expr> Parser::expression() { return logicalOr(); }

std::unique_ptr<Expr> Parser::logicalOr()
{
    auto expr{logicalAnd()};
    while (match("||"))
    {
        const std::string op{previous().text};
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, logicalAnd(), previous().line);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd()
{
    auto expr{equality()};
    while (match("&&"))
    {
        const std::string op{previous().text};
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, equality(), previous().line);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::equality()
{
    auto expr{comparison()};
    while (match("==") || match("!="))
    {
        const std::string op{previous().text};
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, comparison(), previous().line);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::comparison()
{
    auto expr{term()};
    while (match(">") || match(">=") || match("<") || match("<="))
    {
        const std::string op{previous().text};
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, term(), previous().line);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::term()
{
    auto expr{factor()};
    while (match("+") || match("-"))
    {
        const std::string op{previous().text};
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, factor(), previous().line);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::factor()
{
    auto expr{unary()};
    while (match("*") || match("/") || match("%"))
    {
        const std::string op{previous().text};
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, unary(), previous().line);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::unary()
{
    if (match("-") || match("!"))
    {
        const Token op{previous()};
        return std::make_unique<UnaryExpr>(op.text, unary(), op.line);
    }
    return primary();
}

std::unique_ptr<Expr> Parser::primary()
{
    if (match("true")) return std::make_unique<LiteralExpr>(true, previous().line);
    if (match("false")) return std::make_unique<LiteralExpr>(false, previous().line);
    if (peek().type == TokenType::number)
    {
        const Token token{advance()};
        if (token.text.find('.') != std::string::npos)
            return std::make_unique<LiteralExpr>(std::stod(token.text), token.line);
        return std::make_unique<LiteralExpr>(std::stoi(token.text), token.line);
    }
    if (peek().type == TokenType::string_lit)
    {
        const Token token{advance()};
        return std::make_unique<LiteralExpr>(token.text, token.line);
    }
    if (peek().type == TokenType::identifier || check("input"))
    {
        const Token token{advance()};
        if (match("("))
        {
            std::vector<std::unique_ptr<Expr>> args{};
            if (!check(")"))
            {
                do { args.push_back(expression()); }
                while (match(","));
            }
            consume(")", "expected ')' after arguments");
            return std::make_unique<FunctionCallExpr>(token.text, std::move(args), token.line);
        }
        if (match("["))
        {
            auto index{expression()};
            consume("]", "expected ']' after array index");
            return std::make_unique<ArrayAccessExpr>(token.text, std::move(index), token.line);
        }
        if (match("."))
        {
            const Token member{consumeIdentifier("expected member name after '.'")};
            std::vector<std::unique_ptr<Expr>> args{};
            const bool isCall{match("(")};
            if (isCall) { if (!check(")")) do { args.push_back(expression()); } while (match(",")); consume(")", "expected ')' after method arguments"); }
            if (member.text == "size" || member.text == "push" || member.text == "pop" || member.text == "insert" || member.text == "remove" ||
                member.text == "length" || member.text == "find" || member.text == "contains" || member.text == "lower" || member.text == "upper")
            {
                if (!isCall)
                    throw error("member function requires '()'");
                return std::make_unique<ArrayMethodCallExpr>(token.text, member.text, std::move(args), token.line);
            }
            return std::make_unique<MemberExpr>(token.text, member.text, std::move(args), isCall, token.line);
        }
        return std::make_unique<VariableExpr>(token.text, token.line);
    }
    if (match("("))
    {
        auto expr{expression()};
        consume(")", "expected ')' after expression");
        return expr;
    }
    throw error("expected expression");
}
