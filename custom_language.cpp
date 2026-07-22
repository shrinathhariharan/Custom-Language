#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

enum class DataType
{
    t_int,
    t_dec,
    t_str,
    t_bool,
};

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

using Value = std::variant<int, double, std::string, bool, std::vector<int>, std::vector<double>, std::vector<std::string>, std::vector<bool>>;

struct Environment;

struct Expr
{
    std::size_t line{};
    explicit Expr(std::size_t lineNum) : line{lineNum} {}
    virtual ~Expr() = default;
    virtual Value eval(Environment& env) const = 0;
};

struct Stmt
{
    std::size_t line{};
    explicit Stmt(std::size_t lineNum) : line{lineNum} {}
    virtual ~Stmt() = default;
    virtual void exec(Environment& env) const = 0;
};

struct FunctionDefStmt;

struct Environment
{
    std::vector<std::unordered_map<std::string, Value>> scopes{{}};
    std::unordered_map<std::string, const FunctionDefStmt*> functions{};

    Value& get(const std::string& name)
    {
        for (auto scope{scopes.rbegin()}; scope != scopes.rend(); ++scope)
        {
            auto found{scope->find(name)};
            if (found != scope->end())
                return found->second;
        }
        throw std::runtime_error("Unknown variable: " + name);
    }

    const Value& get(const std::string& name) const
    {
        for (auto scope{scopes.rbegin()}; scope != scopes.rend(); ++scope)
        {
            auto found{scope->find(name)};
            if (found != scope->end())
                return found->second;
        }
        throw std::runtime_error("Unknown variable: " + name);
    }

    void declare(const std::string& name, Value value, std::size_t line)
    {
        if (scopes.back().find(name) != scopes.back().end())
            throw std::runtime_error("Line " + std::to_string(line) + ": variable already declared: " + name);
        scopes.back().emplace(name, std::move(value));
    }
};

std::string getName(std::string_view statement = "")
{
    std::cout << statement;
    std::string name{};
    std::getline(std::cin, name);
    return name;
}

std::string getValidFile()
{
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

std::string typeName(DataType type)
{
    switch (type)
    {
    case DataType::t_int: return "int";
    case DataType::t_dec: return "dec";
    case DataType::t_str: return "str";
    case DataType::t_bool: return "bool";
    }
    return "unknown";
}

std::string valueToString(const Value& value)
{
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::string>)
            return arg;
        else if constexpr (std::is_same_v<T, bool>)
            return arg ? "true" : "false";
        else if constexpr (std::is_same_v<T, int>)
            return std::to_string(arg);
        else if constexpr (std::is_same_v<T, double>)
        {
            std::string result{std::to_string(arg)};
            while (result.find('.') != std::string::npos && result.back() == '0')
                result.pop_back();
            if (!result.empty() && result.back() == '.')
                result.pop_back();
            return result;
        }
        else
        {
            std::string result{"["};
            for (std::size_t i{0}; i < arg.size(); ++i)
            {
                if (i > 0)
                    result += ", ";
                result += valueToString(arg[i]);
            }
            result += "]";
            return result;
        }
    }, value);
}

double valueToNumber(const Value& value)
{
    return std::visit([](auto&& arg) -> double {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>)
            return static_cast<double>(arg);
        else if constexpr (std::is_same_v<T, bool>)
            return arg ? 1.0 : 0.0;
        else if constexpr (std::is_same_v<T, std::string>)
        {
            try { return std::stod(arg); }
            catch (...) { throw std::runtime_error("Cannot convert string to number: " + arg); }
        }
        else
            throw std::runtime_error("Array cannot be used as a number");
    }, value);
}

bool valueToBool(const Value& value)
{
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)
            return arg;
        else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>)
            return arg != 0;
        else if constexpr (std::is_same_v<T, std::string>)
            return !arg.empty();
        else
            return !arg.empty();
    }, value);
}

Value castToType(DataType type, const Value& value)
{
    switch (type)
    {
    case DataType::t_int: return static_cast<int>(valueToNumber(value));
    case DataType::t_dec: return valueToNumber(value);
    case DataType::t_str: return valueToString(value);
    case DataType::t_bool: return valueToBool(value);
    }
    throw std::runtime_error("Unknown data type");
}

Value defaultValue(DataType type)
{
    switch (type)
    {
    case DataType::t_int: return 0;
    case DataType::t_dec: return 0.0;
    case DataType::t_str: return std::string{};
    case DataType::t_bool: return false;
    }
    throw std::runtime_error("Unknown data type");
}

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
                "else", "loop", "while", "for", "func", "return", "input"
            };
            tokens.push_back({std::find(keywords.begin(), keywords.end(), text) == keywords.end() ? TokenType::identifier : TokenType::keyword, text, line});
        }
        else if (std::isdigit(static_cast<unsigned char>(c)) || c == '.')
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
                if (two == "==" || two == "!=" || two == ">=" || two == "<=")
                    text = two;
            }
            tokens.push_back({TokenType::symbol, text, line});
            i += text.size();
        }
    }

    tokens.push_back({TokenType::eof_token, "", line});
    return tokens;
}

struct LiteralExpr : Expr
{
    Value value{};
    LiteralExpr(Value v, std::size_t lineNum) : Expr{lineNum}, value{std::move(v)} {}
    Value eval(Environment&) const override { return value; }
};

struct VariableExpr : Expr
{
    std::string name{};
    VariableExpr(std::string varName, std::size_t lineNum) : Expr{lineNum}, name{std::move(varName)} {}
    Value eval(Environment& env) const override { return env.get(name); }
};

struct ArrayAccessExpr : Expr
{
    std::string name{};
    std::unique_ptr<Expr> index{};

    ArrayAccessExpr(std::string varName, std::unique_ptr<Expr> idx, std::size_t lineNum)
        : Expr{lineNum}, name{std::move(varName)}, index{std::move(idx)} {}

    Value eval(Environment& env) const override
    {
        const int idx{static_cast<int>(valueToNumber(index->eval(env)))};
        if (idx < 0)
            throw std::runtime_error("Line " + std::to_string(line) + ": negative array index");

        return std::visit([this, idx](auto&& arg) -> Value {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<double>> ||
                          std::is_same_v<T, std::vector<std::string>> || std::is_same_v<T, std::vector<bool>>)
            {
                if (static_cast<std::size_t>(idx) >= arg.size())
                    throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
                return arg[static_cast<std::size_t>(idx)];
            }
            else
                throw std::runtime_error("Line " + std::to_string(line) + ": variable is not an array");
        }, env.get(name));
    }
};

struct UnaryExpr : Expr
{
    std::string op{};
    std::unique_ptr<Expr> right{};

    UnaryExpr(std::string oper, std::unique_ptr<Expr> expr, std::size_t lineNum)
        : Expr{lineNum}, op{std::move(oper)}, right{std::move(expr)} {}

    Value eval(Environment& env) const override
    {
        if (op == "-")
            return -valueToNumber(right->eval(env));
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown unary operator");
    }
};

struct BinaryExpr : Expr
{
    std::unique_ptr<Expr> left{};
    std::string op{};
    std::unique_ptr<Expr> right{};

    BinaryExpr(std::unique_ptr<Expr> lhs, std::string oper, std::unique_ptr<Expr> rhs, std::size_t lineNum)
        : Expr{lineNum}, left{std::move(lhs)}, op{std::move(oper)}, right{std::move(rhs)} {}

    Value eval(Environment& env) const override
    {
        const Value a{left->eval(env)};
        const Value b{right->eval(env)};

        if (op == "+")
        {
            if (std::holds_alternative<std::string>(a) || std::holds_alternative<std::string>(b))
                return valueToString(a) + valueToString(b);
            const double result{valueToNumber(a) + valueToNumber(b)};
            return (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) ? Value{static_cast<int>(result)} : Value{result};
        }
        if (op == "-") return valueToNumber(a) - valueToNumber(b);
        if (op == "*") return valueToNumber(a) * valueToNumber(b);
        if (op == "/") return valueToNumber(b) == 0 ? 0.0 : valueToNumber(a) / valueToNumber(b);
        if (op == ">") return valueToNumber(a) > valueToNumber(b);
        if (op == "<") return valueToNumber(a) < valueToNumber(b);
        if (op == ">=") return valueToNumber(a) >= valueToNumber(b);
        if (op == "<=") return valueToNumber(a) <= valueToNumber(b);
        if (op == "==") return valueToString(a) == valueToString(b);
        if (op == "!=") return valueToString(a) != valueToString(b);
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown binary operator");
    }
};

struct FunctionCallExpr : Expr
{
    std::string name{};
    std::vector<std::unique_ptr<Expr>> args{};

    FunctionCallExpr(std::string funcName, std::vector<std::unique_ptr<Expr>> arguments, std::size_t lineNum)
        : Expr{lineNum}, name{std::move(funcName)}, args{std::move(arguments)} {}

    Value eval(Environment& env) const override;
};

// Exception thrown by 'return' statements to unwind the call stack
struct ReturnException
{
    Value value{};
};

struct BlockStmt : Stmt
{
    std::vector<std::unique_ptr<Stmt>> statements{};
    BlockStmt(std::vector<std::unique_ptr<Stmt>> body, std::size_t lineNum)
        : Stmt{lineNum}, statements{std::move(body)} {}

    void exec(Environment& env) const override
    {
        env.scopes.push_back({});
        for (const auto& statement : statements)
            statement->exec(env);
        env.scopes.pop_back();
    }
};

struct DeclStmt : Stmt
{
    DataType type{};
    std::string name{};
    bool isDynamic{false};                       // true  => int x[] = {…}
    std::unique_ptr<Expr> initializer{};          // scalar init
    std::vector<std::unique_ptr<Expr>> arrayItems{}; // dynamic-array items

    DeclStmt(DataType dataType, std::string varName, std::size_t lineNum)
        : Stmt{lineNum}, type{dataType}, name{std::move(varName)} {}

    void exec(Environment& env) const override
    {
        if (!isDynamic)
        {
            // Scalar declaration: int x = expr  (or  int x  with default)
            env.declare(name, castToType(type, initializer ? initializer->eval(env) : defaultValue(type)), line);
            return;
        }

        // Dynamic array: int x[] = {e1, e2, …}
        auto buildVec = [&](auto sample) -> Value {
            using T = decltype(sample);
            std::vector<T> values{};
            values.reserve(arrayItems.size());
            for (const auto& item : arrayItems)
                values.push_back(std::get<T>(castToType(type, item->eval(env))));
            return values;
        };

        switch (type)
        {
        case DataType::t_int:  env.declare(name, buildVec(int{}),         line); break;
        case DataType::t_dec:  env.declare(name, buildVec(double{}),      line); break;
        case DataType::t_str:  env.declare(name, buildVec(std::string{}), line); break;
        case DataType::t_bool: env.declare(name, buildVec(bool{}),        line); break;
        }
    }
};

struct ReturnStmt : Stmt
{
    std::unique_ptr<Expr> value{};
    ReturnStmt(std::unique_ptr<Expr> val, std::size_t lineNum)
        : Stmt{lineNum}, value{std::move(val)} {}

    void exec(Environment& env) const override
    {
        throw ReturnException{value ? value->eval(env) : Value{0}};
    }
};

struct AssignStmt : Stmt
{
    std::string name{};
    std::unique_ptr<Expr> value{};
    std::unique_ptr<Expr> index{};

    AssignStmt(std::string varName, std::unique_ptr<Expr> val, std::unique_ptr<Expr> idx, std::size_t lineNum)
        : Stmt{lineNum}, name{std::move(varName)}, value{std::move(val)}, index{std::move(idx)} {}

    void exec(Environment& env) const override
    {
        Value& target{env.get(name)};
        if (!index)
        {
            target = value->eval(env);
            return;
        }

        const int idx{static_cast<int>(valueToNumber(index->eval(env)))};
        if (idx < 0)
            throw std::runtime_error("Line " + std::to_string(line) + ": negative array index");

        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::vector<int>>)
            {
                if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
                arg[static_cast<std::size_t>(idx)] = std::get<int>(castToType(DataType::t_int, value->eval(env)));
            }
            else if constexpr (std::is_same_v<T, std::vector<double>>)
            {
                if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
                arg[static_cast<std::size_t>(idx)] = std::get<double>(castToType(DataType::t_dec, value->eval(env)));
            }
            else if constexpr (std::is_same_v<T, std::vector<std::string>>)
            {
                if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
                arg[static_cast<std::size_t>(idx)] = std::get<std::string>(castToType(DataType::t_str, value->eval(env)));
            }
            else if constexpr (std::is_same_v<T, std::vector<bool>>)
            {
                if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
                arg[static_cast<std::size_t>(idx)] = std::get<bool>(castToType(DataType::t_bool, value->eval(env)));
            }
            else
                throw std::runtime_error("Line " + std::to_string(line) + ": variable is not an array");
        }, target);
    }
};

struct PrintStmt : Stmt
{
    std::unique_ptr<Expr> expr{};
    PrintStmt(std::unique_ptr<Expr> valueExpr, std::size_t lineNum) : Stmt{lineNum}, expr{std::move(valueExpr)} {}
    void exec(Environment& env) const override { std::cout << valueToString(expr->eval(env)); }
};

struct ExprStmt : Stmt
{
    std::unique_ptr<Expr> expr{};
    ExprStmt(std::unique_ptr<Expr> valueExpr, std::size_t lineNum) : Stmt{lineNum}, expr{std::move(valueExpr)} {}
    void exec(Environment& env) const override { (void)expr->eval(env); }
};

struct IfStmt : Stmt
{
    std::unique_ptr<Expr> condition{};
    std::unique_ptr<BlockStmt> thenBlock{};
    std::unique_ptr<BlockStmt> elseBlock{};

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> thenBody, std::unique_ptr<BlockStmt> elseBody, std::size_t lineNum)
        : Stmt{lineNum}, condition{std::move(cond)}, thenBlock{std::move(thenBody)}, elseBlock{std::move(elseBody)} {}

    void exec(Environment& env) const override
    {
        if (valueToBool(condition->eval(env)))
            thenBlock->exec(env);
        else if (elseBlock)
            elseBlock->exec(env);
    }
};

struct WhileStmt : Stmt
{
    std::unique_ptr<Expr> condition{};
    std::unique_ptr<BlockStmt> body{};

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> loopBody, std::size_t lineNum)
        : Stmt{lineNum}, condition{std::move(cond)}, body{std::move(loopBody)} {}

    void exec(Environment& env) const override
    {
        while (valueToBool(condition->eval(env)))
            body->exec(env);
    }
};

struct ForStmt : Stmt
{
    std::unique_ptr<Stmt> initializer{};
    std::unique_ptr<Expr> condition{};
    std::unique_ptr<Stmt> increment{};
    std::unique_ptr<BlockStmt> body{};

    ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> inc, std::unique_ptr<BlockStmt> loopBody, std::size_t lineNum)
        : Stmt{lineNum}, initializer{std::move(init)}, condition{std::move(cond)}, increment{std::move(inc)}, body{std::move(loopBody)} {}

    void exec(Environment& env) const override
    {
        env.scopes.push_back({});
        initializer->exec(env);
        while (valueToBool(condition->eval(env)))
        {
            body->exec(env);
            increment->exec(env);
        }
        env.scopes.pop_back();
    }
};

struct FunctionDefStmt : Stmt
{
    std::string name{};
    std::vector<std::string> params{};
    std::unique_ptr<BlockStmt> body{};

    FunctionDefStmt(std::string funcName, std::vector<std::string> parameters, std::unique_ptr<BlockStmt> funcBody, std::size_t lineNum)
        : Stmt{lineNum}, name{std::move(funcName)}, params{std::move(parameters)}, body{std::move(funcBody)} {}

    void exec(Environment& env) const override { env.functions[name] = this; }

    Value call(Environment& env) const
    {
        try
        {
            body->exec(env);
        }
        catch (const ReturnException& ret)
        {
            env.scopes.pop_back();
            return ret.value;
        }
        env.scopes.pop_back();
        return 0;
    }
};

Value FunctionCallExpr::eval(Environment& env) const
{
    if (name == "input")
    {
        if (!args.empty())
            std::cout << valueToString(args.front()->eval(env));
        std::string input{};
        std::getline(std::cin, input);
        return input;
    }

    auto found{env.functions.find(name)};
    if (found == env.functions.end())
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown function: " + name);

    const FunctionDefStmt* function{found->second};
    if (function->params.size() != args.size())
        throw std::runtime_error("Line " + std::to_string(line) + ": wrong argument count for function: " + name);

    env.scopes.push_back({});
    for (std::size_t i{0}; i < args.size(); ++i)
        env.declare(function->params[i], args[i]->eval(env), line);
    return function->call(env);  // call() handles ReturnException and scope pop
}

class Parser
{
public:
    explicit Parser(std::vector<Token> toks) : tokens{std::move(toks)} {}

    std::vector<std::unique_ptr<Stmt>> parse()
    {
        std::vector<std::unique_ptr<Stmt>> statements{};
        while (!isAtEnd())
            statements.push_back(statement());
        return statements;
    }

private:
    std::vector<Token> tokens{};
    std::size_t current{};

    const Token& peek() const { return tokens[current]; }
    const Token& previous() const { return tokens[current - 1]; }
    bool isAtEnd() const { return peek().type == TokenType::eof_token; }

    bool check(const std::string& text) const { return !isAtEnd() && peek().text == text; }
    bool match(const std::string& text)
    {
        if (!check(text))
            return false;
        ++current;
        return true;
    }

    const Token& advance()
    {
        if (!isAtEnd())
            ++current;
        return previous();
    }

    const Token& consume(const std::string& text, const std::string& message)
    {
        if (check(text))
            return advance();
        throw error(message);
    }

    Token consumeIdentifier(const std::string& message)
    {
        if (peek().type == TokenType::identifier)
            return advance();
        throw error(message);
    }

    std::runtime_error error(const std::string& message) const
    {
        return std::runtime_error("Line " + std::to_string(peek().line) + ": " + message);
    }

    void optionalSemicolon() { match(";"); }

    DataType parseType()
    {
        if (match("int")) return DataType::t_int;
        if (match("dec")) return DataType::t_dec;
        if (match("str")) return DataType::t_str;
        if (match("bool")) return DataType::t_bool;
        throw error("expected type keyword");
    }

    bool isTypeKeyword() const
    {
        const std::string& t{peek().text};
        return (peek().type == TokenType::keyword) &&
               (t == "int" || t == "dec" || t == "str" || t == "bool");
    }

    std::unique_ptr<Stmt> statement()
    {
        if (isTypeKeyword())  return declStatement(true);
        if (check("print"))   return printStatement();
        if (check("if"))      return ifStatement();
        if (check("while"))   return whileStatement();
        if (check("for"))     return forStatement();
        if (check("func"))    return functionStatement();
        if (check("return"))  return returnStatement();
        if (check("{"))       return block();
        return assignmentOrExpressionStatement(true);
    }

    std::unique_ptr<BlockStmt> block()
    {
        const std::size_t line{consume("{", "expected '{'").line};
        std::vector<std::unique_ptr<Stmt>> statements{};
        while (!check("}") && !isAtEnd())
            statements.push_back(statement());
        consume("}", "expected '}' after block");
        return std::make_unique<BlockStmt>(std::move(statements), line);
    }

    // Parses:  int x = expr;
    //          int x[] = {e1, e2, …};
    std::unique_ptr<Stmt> declStatement(bool allowSemicolon)
    {
        const std::size_t line{peek().line};
        DataType type{parseType()};

        const std::string name{consumeIdentifier("expected variable name after type").text};
        auto stmt{std::make_unique<DeclStmt>(type, name, line)};

        if (match("["))
        {
            // Dynamic array: int x[] = {…}
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
            // Scalar: int x = expr  or  int x  (default value)
            if (match("="))
                stmt->initializer = expression();
            // No initializer is also valid — will use defaultValue(type)
        }

        if (allowSemicolon)
            optionalSemicolon();
        return stmt;
    }

    std::unique_ptr<Stmt> printStatement()
    {
        const std::size_t line{consume("print", "expected 'print'").line};
        consume("(", "expected '(' after print");
        auto expr{expression()};
        consume(")", "expected ')' after print expression");
        optionalSemicolon();
        return std::make_unique<PrintStmt>(std::move(expr), line);
    }

    std::unique_ptr<Stmt> ifStatement()
    {
        const std::size_t line{consume("if", "expected 'if'").line};
        consume("(", "expected '(' after if");
        auto condition{expression()};
        consume(")", "expected ')' after if condition");
        auto thenBlock{block()};
        std::unique_ptr<BlockStmt> elseBlock{};
        if (match("else"))
            elseBlock = block();
        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBlock), line);
    }

    std::unique_ptr<Stmt> whileStatement()
    {
        const std::size_t line{consume("while", "expected 'while'").line};
        consume("(", "expected '(' after while");
        auto condition{expression()};
        consume(")", "expected ')' after while condition");
        return std::make_unique<WhileStmt>(std::move(condition), block(), line);
    }

    std::unique_ptr<Stmt> forStatement()
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

    std::unique_ptr<Stmt> functionStatement()
    {
        // Syntax: func <returnType> <name>(<params>) { … }
        const std::size_t line{consume("func", "expected 'func'").line};
        // Optional return-type annotation (ignored at runtime but parsed for correctness)
        if (isTypeKeyword())
            parseType();
        const std::string name{consumeIdentifier("expected function name after 'func'").text};
        consume("(", "expected '(' after function name");

        std::vector<std::string> params{};
        if (!check(")"))
        {
            do
            {
                // Optional type annotation on parameters
                if (isTypeKeyword())
                    parseType();
                params.push_back(consumeIdentifier("expected parameter name").text);
            }
            while (match(","));
        }
        consume(")", "expected ')' after function parameters");
        return std::make_unique<FunctionDefStmt>(name, std::move(params), block(), line);
    }

    std::unique_ptr<Stmt> returnStatement()
    {
        const std::size_t line{consume("return", "expected 'return'").line};
        std::unique_ptr<Expr> val{};
        // If the next token is NOT ';' or '}' or EOF, parse an expression
        if (!check(";") && !check("}") && !isAtEnd())
            val = expression();
        optionalSemicolon();
        return std::make_unique<ReturnStmt>(std::move(val), line);
    }

    std::unique_ptr<Stmt> assignmentOrExpressionStatement(bool allowSemicolon)
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
            if (match("="))
            {
                auto value{expression()};
                if (allowSemicolon)
                    optionalSemicolon();
                return std::make_unique<AssignStmt>(name.text, std::move(value), std::move(index), name.line);
            }
            current = saved;
        }

        const std::size_t line{peek().line};
        auto expr{expression()};
        if (allowSemicolon)
            optionalSemicolon();
        return std::make_unique<ExprStmt>(std::move(expr), line);
    }

    std::unique_ptr<Expr> expression() { return equality(); }

    std::unique_ptr<Expr> equality()
    {
        auto expr{comparison()};
        while (match("==") || match("!="))
        {
            const std::string op{previous().text};
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, comparison(), previous().line);
        }
        return expr;
    }

    std::unique_ptr<Expr> comparison()
    {
        auto expr{term()};
        while (match(">") || match(">=") || match("<") || match("<="))
        {
            const std::string op{previous().text};
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, term(), previous().line);
        }
        return expr;
    }

    std::unique_ptr<Expr> term()
    {
        auto expr{factor()};
        while (match("+") || match("-"))
        {
            const std::string op{previous().text};
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, factor(), previous().line);
        }
        return expr;
    }

    std::unique_ptr<Expr> factor()
    {
        auto expr{unary()};
        while (match("*") || match("/"))
        {
            const std::string op{previous().text};
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, unary(), previous().line);
        }
        return expr;
    }

    std::unique_ptr<Expr> unary()
    {
        if (match("-"))
            return std::make_unique<UnaryExpr>("-", unary(), previous().line);
        return primary();
    }

    std::unique_ptr<Expr> primary()
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
};

std::string readWholeFile(const std::string& fileName)
{
    std::ifstream filePath{fileName};
    std::stringstream buffer{};
    buffer << filePath.rdbuf();
    return buffer.str();
}

int main()
{
    try
    {
        const std::string fileName{getValidFile()};
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
