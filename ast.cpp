#include "ast.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>

Value& Environment::get(const std::string& name)
{
    for (auto scope{scopes.rbegin()}; scope != scopes.rend(); ++scope)
    {
        auto found{scope->find(name)};
        if (found != scope->end())
            return found->second;
    }
    throw std::runtime_error("Unknown variable: " + name);
}

const Value& Environment::get(const std::string& name) const
{
    for (auto scope{scopes.rbegin()}; scope != scopes.rend(); ++scope)
    {
        auto found{scope->find(name)};
        if (found != scope->end())
            return found->second;
    }
    throw std::runtime_error("Unknown variable: " + name);
}

void Environment::declare(const std::string& name, Value value, std::size_t line)
{
    if (scopes.back().find(name) != scopes.back().end())
        throw std::runtime_error("Line " + std::to_string(line) + ": variable already declared: " + name);
    scopes.back().emplace(name, std::move(value));
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

Value VariableExpr::eval(Environment& env) const
{
    return env.get(name);
}

Value ArrayAccessExpr::eval(Environment& env) const
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

Value UnaryExpr::eval(Environment& env) const
{
    if (op == "-")
        return -valueToNumber(right->eval(env));
    throw std::runtime_error("Line " + std::to_string(line) + ": unknown unary operator");
}

Value BinaryExpr::eval(Environment& env) const
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
    return function->call(env);
}

void BlockStmt::exec(Environment& env) const
{
    env.scopes.push_back({});
    for (const auto& statement : statements)
        statement->exec(env);
    env.scopes.pop_back();
}

void DeclStmt::exec(Environment& env) const
{
    if (!isDynamic)
    {
        env.declare(name, castToType(type, initializer ? initializer->eval(env) : defaultValue(type)), line);
        return;
    }

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

void ReturnStmt::exec(Environment& env) const
{
    throw ReturnException{value ? value->eval(env) : Value{0}};
}

void AssignStmt::exec(Environment& env) const
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

void PrintStmt::exec(Environment& env) const
{
    std::cout << valueToString(expr->eval(env));
}

void ExprStmt::exec(Environment& env) const
{
    (void)expr->eval(env);
}

void IfStmt::exec(Environment& env) const
{
    if (valueToBool(condition->eval(env)))
        thenBlock->exec(env);
    else if (elseBlock)
        elseBlock->exec(env);
}

void WhileStmt::exec(Environment& env) const
{
    while (valueToBool(condition->eval(env)))
        body->exec(env);
}

void ForStmt::exec(Environment& env) const
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

void FunctionDefStmt::exec(Environment& env) const
{
    env.functions[name] = this;
}

Value FunctionDefStmt::call(Environment& env) const
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
