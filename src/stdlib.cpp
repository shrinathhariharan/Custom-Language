#include "stdlib.h"
#include <cmath>
#include <stdexcept>

StandardLibrary& StandardLibrary::instance() {
    static StandardLibrary instance;
    return instance;
}

StandardLibrary::StandardLibrary() {
    setup();
}

void StandardLibrary::registerFunction(const std::string& moduleName, const std::string& funcName, NativeFunction func) {
    modules[moduleName][funcName] = func;
}

bool StandardLibrary::hasModule(const std::string& moduleName) const {
    return modules.find(moduleName) != modules.end();
}

void StandardLibrary::setup() {
    auto& lib = *this;

    // Math module functions
    lib.registerFunction("math", "pow", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 2) {
            throw std::runtime_error("pow() expects exactly 2 arguments");
        }
        double base = valueToNumber(args[0]);
        double exp = valueToNumber(args[1]);
        return Value{std::pow(base, exp)};
    });

    lib.registerFunction("math", "abs", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("abs() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::abs(val)};
    });

    lib.registerFunction("math", "sqrt", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("sqrt() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        if (val < 0) {
            throw std::runtime_error("sqrt() argument cannot be negative");
        }
        return Value{std::sqrt(val)};
    });

    lib.registerFunction("math", "sin", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("sin() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::sin(val)};
    });

    lib.registerFunction("math", "cos", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("cos() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::cos(val)};
    });

    lib.registerFunction("math", "tan", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("tan() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::tan(val)};
    });

    lib.registerFunction("math", "log", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("log() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        if (val <= 0) {
            throw std::runtime_error("log() argument must be positive");
        }
        return Value{std::log(val)};
    });

    lib.registerFunction("math", "log10", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("log10() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        if (val <= 0) {
            throw std::runtime_error("log10() argument must be positive");
        }
        return Value{std::log10(val)};
    });

    lib.registerFunction("math", "exp", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("exp() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::exp(val)};
    });

    lib.registerFunction("math", "ceil", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("ceil() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::ceil(val)};
    });

    lib.registerFunction("math", "floor", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("floor() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::floor(val)};
    });

    lib.registerFunction("math", "round", [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1) {
            throw std::runtime_error("round() expects exactly 1 argument");
        }
        double val = valueToNumber(args[0]);
        return Value{std::round(val)};
    });
}

void StandardLibrary::loadModule(const std::string& moduleName, Environment& env) const {
    (void)env;
    if (!hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
}
