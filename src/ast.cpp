#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "stdlib.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

bool Environment::hasVar(const std::string& name) const
{
    for (auto scope{scopes.rbegin()}; scope != scopes.rend(); ++scope)
    {
        if (scope->find(name) != scope->end())
            return true;
    }
    return false;
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

std::string valueTypeName(const Value& value)
{
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)               return "int";
        else if constexpr (std::is_same_v<T, double>)        return "dec";
        else if constexpr (std::is_same_v<T, std::string>)   return "str";
        else if constexpr (std::is_same_v<T, bool>)          return "bool";
        else if constexpr (std::is_same_v<T, ObjectPtr>)     return arg->className;
        else return "array";
    }, value);
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
        else if constexpr (std::is_same_v<T, ObjectPtr>)
            return "<" + arg->className + ">";
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
        else if constexpr (std::is_same_v<T, ObjectPtr>) return true;
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
    if (op == "!")
        return !valueToBool(right->eval(env));
    throw std::runtime_error("Line " + std::to_string(line) + ": unknown unary operator");
}

Value BinaryExpr::eval(Environment& env) const
{
    const Value a{left->eval(env)};
    if (op == "&&")
        return valueToBool(a) && valueToBool(right->eval(env));
    if (op == "||")
        return valueToBool(a) || valueToBool(right->eval(env));
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
    if (op == "%")
    {
        if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b))
        {
            const int intB{std::get<int>(b)};
            return intB == 0 ? 0 : std::get<int>(a) % intB;
        }
        const double numB{valueToNumber(b)};
        return numB == 0.0 ? 0.0 : std::fmod(valueToNumber(a), numB);
    }
    if (op == ">") return valueToNumber(a) > valueToNumber(b);
    if (op == "<") return valueToNumber(a) < valueToNumber(b);
    if (op == ">=") return valueToNumber(a) >= valueToNumber(b);
    if (op == "<=") return valueToNumber(a) <= valueToNumber(b);
    if (op == "==") return valueToString(a) == valueToString(b);
    if (op == "!=") return valueToString(a) != valueToString(b);
    throw std::runtime_error("Line " + std::to_string(line) + ": unknown binary operator");
}

Value ArrayMethodCallExpr::eval(Environment& env) const
{
    if (!env.hasVar(arrayName))
    {
        if (StandardLibrary::instance().hasModule(arrayName)) {
            auto moduleIt = StandardLibrary::instance().getModules().find(arrayName);
            if (moduleIt != StandardLibrary::instance().getModules().end()) {
                auto funcIt = moduleIt->second.find(method);
                if (funcIt != moduleIt->second.end()) {
                    std::vector<Value> values{};
                    for (const auto& arg : args) values.push_back(arg->eval(env));
                    return funcIt->second(values);
                }
            }
        }
    }

    Value& array{env.get(arrayName)};
    auto requireArgs = [this](std::size_t count) {
        if (args.size() != count)
            throw std::runtime_error("Line " + std::to_string(line) + ": " + method + " expects " + std::to_string(count) + " argument(s)");
    };

    if (std::holds_alternative<std::string>(array))
    {
        const std::string& value{std::get<std::string>(array)};
        if (method == "size" || method == "length")
        {
            requireArgs(0);
            return static_cast<int>(value.size());
        }
        if (method == "find")
        {
            requireArgs(1);
            const std::string substring{std::get<std::string>(castToType(DataType::t_str, args[0]->eval(env)))};
            const std::size_t position{value.find(substring)};
            return position == std::string::npos ? -1 : static_cast<int>(position);
        }
        if (method == "contains")
        {
            requireArgs(1);
            const std::string substring{std::get<std::string>(castToType(DataType::t_str, args[0]->eval(env)))};
            return value.find(substring) != std::string::npos;
        }
        if (method == "lower" || method == "upper")
        {
            requireArgs(0);
            std::string transformed{value};
            std::transform(transformed.begin(), transformed.end(), transformed.begin(), [this](unsigned char c) {
                return static_cast<char>(method == "lower" ? std::tolower(c) : std::toupper(c));
            });
            return transformed;
        }
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown string method: " + method);
    }

    return std::visit([&](auto& values) -> Value {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<double>> ||
                      std::is_same_v<T, std::vector<std::string>> || std::is_same_v<T, std::vector<bool>>)
        {
            using Element = typename T::value_type;
            if (method == "size")
            {
                requireArgs(0);
                return static_cast<int>(values.size());
            }
            if (method == "push")
            {
                requireArgs(1);
                if constexpr (std::is_same_v<Element, int>) values.push_back(std::get<int>(castToType(DataType::t_int, args[0]->eval(env))));
                else if constexpr (std::is_same_v<Element, double>) values.push_back(std::get<double>(castToType(DataType::t_dec, args[0]->eval(env))));
                else if constexpr (std::is_same_v<Element, std::string>) values.push_back(std::get<std::string>(castToType(DataType::t_str, args[0]->eval(env))));
                else values.push_back(std::get<bool>(castToType(DataType::t_bool, args[0]->eval(env))));
                return static_cast<int>(values.size());
            }
            if (method == "pop")
            {
                requireArgs(0);
                if (values.empty()) throw std::runtime_error("Line " + std::to_string(line) + ": cannot pop an empty array");
                const Value result{values.back()};
                values.pop_back();
                return result;
            }
            if (method == "insert")
            {
                requireArgs(2);
                const int index{static_cast<int>(valueToNumber(args[0]->eval(env)))};
                if (index < 0 || static_cast<std::size_t>(index) > values.size())
                    throw std::runtime_error("Line " + std::to_string(line) + ": array insertion index out of bounds");
                if constexpr (std::is_same_v<Element, int>) values.insert(values.begin() + index, std::get<int>(castToType(DataType::t_int, args[1]->eval(env))));
                else if constexpr (std::is_same_v<Element, double>) values.insert(values.begin() + index, std::get<double>(castToType(DataType::t_dec, args[1]->eval(env))));
                else if constexpr (std::is_same_v<Element, std::string>) values.insert(values.begin() + index, std::get<std::string>(castToType(DataType::t_str, args[1]->eval(env))));
                else values.insert(values.begin() + index, std::get<bool>(castToType(DataType::t_bool, args[1]->eval(env))));
                return static_cast<int>(values.size());
            }
            if (method == "remove")
            {
                requireArgs(1);
                Element target{};
                if constexpr (std::is_same_v<Element, int>) target = std::get<int>(castToType(DataType::t_int, args[0]->eval(env)));
                else if constexpr (std::is_same_v<Element, double>) target = std::get<double>(castToType(DataType::t_dec, args[0]->eval(env)));
                else if constexpr (std::is_same_v<Element, std::string>) target = std::get<std::string>(castToType(DataType::t_str, args[0]->eval(env)));
                else target = std::get<bool>(castToType(DataType::t_bool, args[0]->eval(env)));

                auto it = std::find(values.begin(), values.end(), target);
                if (it != values.end())
                    values.erase(it);
                return static_cast<int>(values.size());
            }
            throw std::runtime_error("Line " + std::to_string(line) + ": unknown array method: " + method);
        }
        else
            throw std::runtime_error("Line " + std::to_string(line) + ": variable is not an array");
    }, array);
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

    if (name == "toInt" || name == "toDec" || name == "toStr" || name == "toBool")
    {
        if (args.size() != 1)
            throw std::runtime_error("Line " + std::to_string(line) + ": " + name + " expects exactly 1 argument");
        const Value arg{args[0]->eval(env)};
        if (name == "toInt")  return static_cast<int>(valueToNumber(arg));
        if (name == "toDec")  return valueToNumber(arg);
        if (name == "toStr")  return valueToString(arg);
        if (name == "toBool") return valueToBool(arg);
    }

    if (name == "type")
    {
        if (args.size() != 1)
            throw std::runtime_error("Line " + std::to_string(line) + ": type() expects exactly 1 argument");
        return valueTypeName(args[0]->eval(env));
    }

    // Check for standard library functions (e.g., math.pow, math.abs)
    size_t dotPos = name.find('.');
    if (dotPos != std::string::npos) {
        std::string moduleName = name.substr(0, dotPos);
        std::string funcName = name.substr(dotPos + 1);

        if (StandardLibrary::instance().hasModule(moduleName)) {
            auto moduleIt = StandardLibrary::instance().getModules().find(moduleName);
            if (moduleIt != StandardLibrary::instance().getModules().end()) {
                auto funcIt = moduleIt->second.find(funcName);
                if (funcIt != moduleIt->second.end()) {
                    // Found the standard library function
                    std::vector<Value> values{};
                    for (const auto& arg : args) values.push_back(arg->eval(env));
                    return funcIt->second(values);
                }
            }
        }
    }

    std::vector<Value> values{}; for (const auto& arg : args) values.push_back(arg->eval(env));
    std::string lookupName = name;
    if (env.classes.find(lookupName) == env.classes.end() && env.functions.find(lookupName) == env.functions.end() && !env.currentModule.empty())
    {
        if (env.classes.find(env.currentModule + "." + name) != env.classes.end())
            lookupName = env.currentModule + "." + name;
        else if (env.functions.find(env.currentModule + "." + name) != env.functions.end())
            lookupName = env.currentModule + "." + name;
    }
    if (auto classFound{env.classes.find(lookupName)}; classFound != env.classes.end())
    {
        const auto* definition{classFound->second}; auto object{std::make_shared<Object>()}; object->className = lookupName;
        for (const auto& field : definition->fields) object->fields[field->name] = field->initializer ? castToType(field->type, field->initializer->eval(env)) : defaultValue(field->type);
        if (definition->constructor)
        {
            if (definition->constructor->params.size() != values.size() + 1) throw std::runtime_error("Line " + std::to_string(line) + ": wrong constructor argument count");
            env.scopes.push_back({}); env.declare(definition->constructor->params[0], object, line);
            for (std::size_t i{0}; i < values.size(); ++i) env.declare(definition->constructor->params[i + 1], values[i], line);
            definition->constructor->call(env);
        }
        return object;
    }
    auto found{env.functions.find(lookupName)};
    if (found == env.functions.end())
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown function: " + name);

    const FunctionDefStmt* function{found->second};
    if (function->params.size() != args.size())
        throw std::runtime_error("Line " + std::to_string(line) + ": wrong argument count for function: " + name);

    env.scopes.push_back({});
    for (std::size_t i{0}; i < args.size(); ++i)
        env.declare(function->params[i], values[i], line);
    return function->call(env);
}

Value MemberExpr::eval(Environment& env) const
{
    if (env.hasVar(object))
    {
        const Value& receiver{env.get(object)}; if (!std::holds_alternative<ObjectPtr>(receiver)) throw std::runtime_error("Line " + std::to_string(line) + ": member access requires an object");
        const auto instance{std::get<ObjectPtr>(receiver)}; if (!member.empty() && member.front() == '_' && object != "self") throw std::runtime_error("Line " + std::to_string(line) + ": private field: " + member);

        if (instance->className == "File" || instance->nativeData)
        {
            auto fileHandle = std::dynamic_pointer_cast<FileHandle>(instance->nativeData);
            if (!fileHandle)
                throw std::runtime_error("Line " + std::to_string(line) + ": invalid file handle");

            if (member == "write")
            {
                if (args.empty())
                    throw std::runtime_error("Line " + std::to_string(line) + ": write() expects at least 1 argument");
                if (!fileHandle->stream.is_open())
                    throw std::runtime_error("Line " + std::to_string(line) + ": file is not open");
                std::string text = valueToString(args[0]->eval(env));
                fileHandle->stream << text;
                return static_cast<int>(text.size());
            }
            if (member == "writeLine" || member == "writeline")
            {
                if (!fileHandle->stream.is_open())
                    throw std::runtime_error("Line " + std::to_string(line) + ": file is not open");
                std::string text = args.empty() ? "" : valueToString(args[0]->eval(env));
                fileHandle->stream << text << '\n';
                return static_cast<int>(text.size() + 1);
            }
            if (member == "flush")
            {
                if (fileHandle->stream.is_open())
                    fileHandle->stream.flush();
                return true;
            }
            if (member == "close")
            {
                if (fileHandle->stream.is_open())
                {
                    fileHandle->stream.close();
                    fileHandle->isOpen = false;
                }
                return true;
            }
            if (member == "read")
            {
                if (!fileHandle->stream.is_open())
                    throw std::runtime_error("Line " + std::to_string(line) + ": file is not open");
                if (args.empty())
                {
                    std::stringstream ss;
                    ss << fileHandle->stream.rdbuf();
                    return ss.str();
                }
                else
                {
                    int count = static_cast<int>(valueToNumber(args[0]->eval(env)));
                    if (count <= 0) return std::string{};
                    std::string buf(count, '\0');
                    fileHandle->stream.read(&buf[0], count);
                    std::streamsize bytesRead = fileHandle->stream.gcount();
                    buf.resize(bytesRead);
                    return buf;
                }
            }
            if (member == "readLine" || member == "readline")
            {
                if (!fileHandle->stream.is_open())
                    throw std::runtime_error("Line " + std::to_string(line) + ": file is not open");
                std::string lineStr{};
                if (std::getline(fileHandle->stream, lineStr))
                {
                    if (!lineStr.empty() && lineStr.back() == '\r')
                        lineStr.pop_back();
                    return lineStr;
                }
                return std::string{};
            }
            if (member == "readLines" || member == "readlines")
            {
                if (!fileHandle->stream.is_open())
                    throw std::runtime_error("Line " + std::to_string(line) + ": file is not open");
                std::vector<std::string> lines{};
                std::string lineStr{};
                while (std::getline(fileHandle->stream, lineStr))
                {
                    if (!lineStr.empty() && lineStr.back() == '\r')
                        lineStr.pop_back();
                    lines.push_back(lineStr);
                }
                return lines;
            }
            if (member == "eof" || member == "isEof")
            {
                return fileHandle->stream.eof() || fileHandle->stream.peek() == EOF;
            }
            if (member == "isOpen" || member == "is_open")
            {
                return fileHandle->isOpen && fileHandle->stream.is_open();
            }
            throw std::runtime_error("Line " + std::to_string(line) + ": unknown File method: " + member);
        }

        if (!isCall) { auto field{instance->fields.find(member)}; if (field == instance->fields.end()) throw std::runtime_error("unknown field: " + member); return field->second; }
        const auto* definition{env.classes.at(instance->className)}; auto method{definition->methods.find(member)}; if (method == definition->methods.end()) throw std::runtime_error("unknown method: " + member);
        if (method->second->params.size() != args.size()) throw std::runtime_error("wrong argument count for method: " + member);
        std::vector<Value> values{}; for (const auto& arg: args) values.push_back(arg->eval(env)); env.scopes.push_back({}); env.declare("self", instance, line); for (std::size_t i{}; i < values.size(); ++i) env.declare(method->second->params[i], values[i], line); return method->second->call(env);
    }

    const std::string fullName{object + "." + member};
    if (isCall)
    {
        auto funcIt{env.functions.find(fullName)};
        if (funcIt != env.functions.end())
        {
            const FunctionDefStmt* function{funcIt->second};
            if (function->params.size() != args.size())
                throw std::runtime_error("Line " + std::to_string(line) + ": wrong argument count for function: " + fullName);
            std::vector<Value> values{};
            for (const auto& arg : args) values.push_back(arg->eval(env));
            env.scopes.push_back({});
            for (std::size_t i{0}; i < args.size(); ++i)
                env.declare(function->params[i], values[i], line);
            return function->call(env);
        }

        // Check if it's a standard library function (e.g., math.pow)
        if (StandardLibrary::instance().hasModule(object)) {
            auto moduleIt = StandardLibrary::instance().getModules().find(object);
            if (moduleIt != StandardLibrary::instance().getModules().end()) {
                auto funcIt = moduleIt->second.find(member);
                if (funcIt != moduleIt->second.end()) {
                    // Found the standard library function
                    std::vector<Value> values{};
                    for (const auto& arg : args) values.push_back(arg->eval(env));
                    return funcIt->second(values);
                }
            }
        }

        auto classIt{env.classes.find(fullName)};
        if (classIt != env.classes.end())
        {
            const auto* definition{classIt->second};
            auto obj{std::make_shared<Object>()};
            obj->className = fullName;
            for (const auto& field : definition->fields)
                obj->fields[field->name] = field->initializer ? castToType(field->type, field->initializer->eval(env)) : defaultValue(field->type);
            std::vector<Value> values{};
            for (const auto& arg : args) values.push_back(arg->eval(env));
            if (definition->constructor)
            {
                if (definition->constructor->params.size() != values.size() + 1)
                    throw std::runtime_error("Line " + std::to_string(line) + ": wrong constructor argument count");
                env.scopes.push_back({});
                env.declare(definition->constructor->params[0], obj, line);
                for (std::size_t i{0}; i < values.size(); ++i)
                    env.declare(definition->constructor->params[i + 1], values[i], line);
                definition->constructor->call(env);
            }
            return obj;
        }
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown function or class: " + fullName);
    }
    else
    {
        if (env.hasVar(fullName))
            return env.get(fullName);
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown variable or field: " + fullName);
    }
}

void MemberAssignStmt::exec(Environment& env) const
{
    if (env.hasVar(object))
    {
        Value& receiver{env.get(object)}; if (!std::holds_alternative<ObjectPtr>(receiver)) throw std::runtime_error("Line " + std::to_string(line) + ": member assignment requires an object"); if (!member.empty() && member.front() == '_' && object != "self") throw std::runtime_error("Line " + std::to_string(line) + ": private field: " + member);
        Value& target{std::get<ObjectPtr>(receiver)->fields.at(member)}; const Value assigned{value->eval(env)}; if (op == "=") { target = assigned; return; } target = BinaryExpr{std::make_unique<LiteralExpr>(target,line),op.substr(0,op.size() - 1),std::make_unique<LiteralExpr>(assigned,line),line}.eval(env);
        return;
    }

    const std::string fullName{object + "." + member};
    if (env.hasVar(fullName))
    {
        Value& target{env.get(fullName)};
        const Value assigned{value->eval(env)};
        if (op == "=") { target = assigned; return; }
        target = BinaryExpr{std::make_unique<LiteralExpr>(target, line), op.substr(0, op.size() - 1),
                            std::make_unique<LiteralExpr>(assigned, line), line}.eval(env);
        return;
    }

    throw std::runtime_error("Line " + std::to_string(line) + ": unknown variable or field: " + fullName);
}

void BlockStmt::exec(Environment& env) const
{
    env.scopes.push_back({});
    try
    {
        for (const auto& statement : statements)
            statement->exec(env);
    }
    catch (...)
    {
        env.scopes.pop_back();
        throw;
    }
    env.scopes.pop_back();
}

void DeclStmt::exec(Environment& env) const
{
    if (!isDynamic)
    {
        env.declare(name, castToType(type, initializer ? initializer->eval(env) : defaultValue(type)), line);
        if (!env.currentModule.empty() && env.scopes.size() == 1)
        {
            env.declare(env.currentModule + "." + name, env.get(name), line);
        }
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

    if (!env.currentModule.empty() && env.scopes.size() == 1)
    {
        env.declare(env.currentModule + "." + name, env.get(name), line);
    }
}

void ObjectDeclStmt::exec(Environment& env) const
{
    if (env.classes.find(className) == env.classes.end())
        throw std::runtime_error("Line " + std::to_string(line) + ": unknown class: " + className);
    Value object{initializer->eval(env)};
    if (!std::holds_alternative<ObjectPtr>(object) || std::get<ObjectPtr>(object)->className != className)
        throw std::runtime_error("Line " + std::to_string(line) + ": expected a " + className + " object");
    env.declare(name, std::move(object), line);
}

void ReturnStmt::exec(Environment& env) const
{
    throw ReturnException{value ? value->eval(env) : Value{0}};
}

void AssignStmt::exec(Environment& env) const
{
    if (!index && op == "=" && !env.hasVar(name))
    {
        env.declare(name, value->eval(env), line);
        return;
    }
    Value& target{env.get(name)};
    const Value assigned{value->eval(env)};
    auto apply = [&](const Value& current) -> Value {
        if (op == "=") return assigned;
        const std::string binaryOp{op.substr(0, op.size() - 1)};
        return BinaryExpr{std::make_unique<LiteralExpr>(current, line), binaryOp,
                          std::make_unique<LiteralExpr>(assigned, line), line}.eval(env);
    };
    if (!index)
    {
        target = apply(target);
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
            arg[static_cast<std::size_t>(idx)] = std::get<int>(castToType(DataType::t_int, apply(arg[static_cast<std::size_t>(idx)])));
        }
        else if constexpr (std::is_same_v<T, std::vector<double>>)
        {
            if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
            arg[static_cast<std::size_t>(idx)] = std::get<double>(castToType(DataType::t_dec, apply(arg[static_cast<std::size_t>(idx)])));
        }
        else if constexpr (std::is_same_v<T, std::vector<std::string>>)
        {
            if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
            arg[static_cast<std::size_t>(idx)] = std::get<std::string>(castToType(DataType::t_str, apply(arg[static_cast<std::size_t>(idx)])));
        }
        else if constexpr (std::is_same_v<T, std::vector<bool>>)
        {
            if (static_cast<std::size_t>(idx) >= arg.size()) throw std::runtime_error("Line " + std::to_string(line) + ": array index out of bounds");
            arg[static_cast<std::size_t>(idx)] = std::get<bool>(castToType(DataType::t_bool, apply(arg[static_cast<std::size_t>(idx)])));
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
    else if (elseBranch)
        elseBranch->exec(env);
}

void WhileStmt::exec(Environment& env) const
{
    while (valueToBool(condition->eval(env)))
    {
        try { body->exec(env); }
        catch (const ContinueException&) { continue; }
        catch (const BreakException&) { break; }
    }
}

void ForStmt::exec(Environment& env) const
{
    env.scopes.push_back({});
    initializer->exec(env);
    while (valueToBool(condition->eval(env)))
    {
        try { body->exec(env); }
        catch (const ContinueException&) { increment->exec(env); continue; }
        catch (const BreakException&) { break; }
        increment->exec(env);
    }
    env.scopes.pop_back();
}

void FunctionDefStmt::exec(Environment& env) const
{
    env.functions[name] = this;
    if (!env.currentModule.empty())
    {
        env.functions[env.currentModule + "." + name] = this;
    }
}

void ClassDefStmt::exec(Environment& env) const
{
    env.classes[name] = this;
    if (!env.currentModule.empty())
    {
        env.classes[env.currentModule + "." + name] = this;
    }
}

void ImportStmt::exec(Environment& env) const
{
    // Check if this is a standard library module
    if (StandardLibrary::instance().hasModule(fileName)) {
        if (env.loadedFiles.find(fileName) != env.loadedFiles.end())
            return;
        env.loadedFiles.insert(fileName);

        StandardLibrary::instance().loadModule(fileName, env);
        return;
    }

    std::filesystem::path path{fileName};
    if (env.loadedFiles.find(path.string()) != env.loadedFiles.end())
        return;
    env.loadedFiles.insert(path.string());

    if (!std::filesystem::exists(path))
        throw std::runtime_error("Line " + std::to_string(line) + ": could not find imported file: " + fileName);

    std::ifstream file{path};
    if (!file.is_open())
        throw std::runtime_error("Line " + std::to_string(line) + ": could not open imported file: " + fileName);

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    const std::string moduleName = path.stem().string();
    std::vector<Token> tokens = tokenize(buffer.str());
    Parser parser{tokens};
    auto statements = parser.parse();

    env.moduleASTs.push_back(std::move(statements));
    const auto& storedStatements = env.moduleASTs.back();

    std::string previousModule = env.currentModule;
    env.currentModule = moduleName;

    try
    {
        for (const auto& stmt : storedStatements)
            stmt->exec(env);
    }
    catch (...)
    {
        env.currentModule = previousModule;
        throw;
    }

    env.currentModule = previousModule;
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
