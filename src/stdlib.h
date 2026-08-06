#ifndef STDLIB_H
#define STDLIB_H

#include "ast.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using NativeFunction = std::function<Value(const std::vector<Value>& args)>;

class StandardLibrary
{
public:
    static StandardLibrary& instance();

    void registerFunction(const std::string& moduleName, const std::string& funcName, NativeFunction func);
    bool hasModule(const std::string& moduleName) const;
    void loadModule(const std::string& moduleName, Environment& env) const;

    std::unordered_map<std::string, std::unordered_map<std::string, NativeFunction>>& getModules() { return modules; }

private:
    StandardLibrary();
    void setup();

    std::unordered_map<std::string, std::unordered_map<std::string, NativeFunction>> modules;
};

#endif // STDLIB_H
