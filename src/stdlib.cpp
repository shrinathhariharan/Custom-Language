#include "stdlib.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
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

    // IO module functions
    lib.registerFunction("io", "write", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.write() expects a file path");
        }
        std::string path = valueToString(args[0]);
        auto obj = std::make_shared<Object>();
        obj->className = "File";
        auto handle = std::make_shared<FileHandle>(path, "w");
        if (!handle->isOpen) {
            throw std::runtime_error("io.write(): could not open file: " + path);
        }
        obj->nativeData = handle;
        obj->fields["path"] = path;
        obj->fields["mode"] = "w";

        if (args.size() > 1) {
            std::string content = valueToString(args[1]);
            handle->stream << content;
        }
        return Value{obj};
    });

    lib.registerFunction("io", "read", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.read() expects a file path");
        }
        std::string path = valueToString(args[0]);
        auto obj = std::make_shared<Object>();
        obj->className = "File";
        auto handle = std::make_shared<FileHandle>(path, "r");
        if (!handle->isOpen) {
            throw std::runtime_error("io.read(): could not open file: " + path);
        }
        obj->nativeData = handle;
        obj->fields["path"] = path;
        obj->fields["mode"] = "r";
        return Value{obj};
    });

    lib.registerFunction("io", "append", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.append() expects a file path");
        }
        std::string path = valueToString(args[0]);
        auto obj = std::make_shared<Object>();
        obj->className = "File";
        auto handle = std::make_shared<FileHandle>(path, "a");
        if (!handle->isOpen) {
            throw std::runtime_error("io.append(): could not open file: " + path);
        }
        obj->nativeData = handle;
        obj->fields["path"] = path;
        obj->fields["mode"] = "a";
        return Value{obj};
    });

    lib.registerFunction("io", "open", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.open() expects at least a file path");
        }
        std::string path = valueToString(args[0]);
        std::string mode = args.size() > 1 ? valueToString(args[1]) : "r";
        auto obj = std::make_shared<Object>();
        obj->className = "File";
        auto handle = std::make_shared<FileHandle>(path, mode);
        if (!handle->isOpen) {
            throw std::runtime_error("io.open(): could not open file: " + path);
        }
        obj->nativeData = handle;
        obj->fields["path"] = path;
        obj->fields["mode"] = mode;
        return Value{obj};
    });

    lib.registerFunction("io", "exists", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.exists() expects a file path");
        }
        std::string path = valueToString(args[0]);
        return Value{std::filesystem::exists(path)};
    });

    lib.registerFunction("io", "remove", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.remove() expects a file path");
        }
        std::string path = valueToString(args[0]);
        return Value{std::filesystem::remove(path)};
    });

    lib.registerFunction("io", "readAll", [](const std::vector<Value>& args) -> Value {
        if (args.empty()) {
            throw std::runtime_error("io.readAll() expects a file path");
        }
        std::string path = valueToString(args[0]);
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("io.readAll(): could not open file: " + path);
        }
        std::stringstream ss;
        ss << file.rdbuf();
        return Value{ss.str()};
    });

    lib.registerFunction("io", "writeAll", [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) {
            throw std::runtime_error("io.writeAll() expects file path and content");
        }
        std::string path = valueToString(args[0]);
        std::string content = valueToString(args[1]);
        std::ofstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("io.writeAll(): could not open file: " + path);
        }
        file << content;
        return Value{true};
    });
}

void StandardLibrary::loadModule(const std::string& moduleName, Environment& env) const {
    (void)env;
    if (!hasModule(moduleName)) {
        throw std::runtime_error("Module not found: " + moduleName);
    }
}
