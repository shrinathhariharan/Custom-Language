#ifndef AST_H
#define AST_H

#include <cstddef>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

enum class DataType
{
    t_int,
    t_dec,
    t_str,
    t_bool,
};

struct NativeData
{
    virtual ~NativeData() = default;
};

struct FileHandle : NativeData
{
    std::string path{};
    std::string mode{};
    std::fstream stream{};
    bool isOpen{false};

    FileHandle(const std::string& p, const std::string& m)
        : path{p}, mode{m}
    {
        std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::out;
        if (mode == "w" || mode == "write")
            openMode = std::ios_base::out | std::ios_base::trunc;
        else if (mode == "a" || mode == "append")
            openMode = std::ios_base::out | std::ios_base::app;
        else if (mode == "r" || mode == "read")
            openMode = std::ios_base::in;

        stream.open(path, openMode);
        isOpen = stream.is_open();
    }

    ~FileHandle() override
    {
        if (stream.is_open())
            stream.close();
    }
};

struct Object;
using ObjectPtr = std::shared_ptr<Object>;

using Value = std::variant<int, double, std::string, bool, std::vector<int>, std::vector<double>, std::vector<std::string>, std::vector<bool>, ObjectPtr>;

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
struct ClassDefStmt;
struct Object {
    std::string className{};
    std::unordered_map<std::string, Value> fields{};
    std::shared_ptr<NativeData> nativeData{};
};

struct Environment
{
    std::vector<std::unordered_map<std::string, Value>> scopes{{}};
    std::unordered_map<std::string, const FunctionDefStmt*> functions{};
    std::unordered_map<std::string, const ClassDefStmt*> classes{};
    std::unordered_set<std::string> loadedFiles{};
    std::vector<std::vector<std::unique_ptr<Stmt>>> moduleASTs{};
    std::string currentModule{};

    Value& get(const std::string& name);
    const Value& get(const std::string& name) const;
    bool hasVar(const std::string& name) const;
    void declare(const std::string& name, Value value, std::size_t line);
};

std::string typeName(DataType type);
std::string valueTypeName(const Value& value);
std::string valueToString(const Value& value);
double valueToNumber(const Value& value);
bool valueToBool(const Value& value);
Value castToType(DataType type, const Value& value);
Value defaultValue(DataType type);

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
    Value eval(Environment& env) const override;
};

struct ArrayAccessExpr : Expr
{
    std::string name{};
    std::unique_ptr<Expr> index{};

    ArrayAccessExpr(std::string varName, std::unique_ptr<Expr> idx, std::size_t lineNum)
        : Expr{lineNum}, name{std::move(varName)}, index{std::move(idx)} {}

    Value eval(Environment& env) const override;
};

struct UnaryExpr : Expr
{
    std::string op{};
    std::unique_ptr<Expr> right{};

    UnaryExpr(std::string oper, std::unique_ptr<Expr> expr, std::size_t lineNum)
        : Expr{lineNum}, op{std::move(oper)}, right{std::move(expr)} {}

    Value eval(Environment& env) const override;
};

struct BinaryExpr : Expr
{
    std::unique_ptr<Expr> left{};
    std::string op{};
    std::unique_ptr<Expr> right{};

    BinaryExpr(std::unique_ptr<Expr> lhs, std::string oper, std::unique_ptr<Expr> rhs, std::size_t lineNum)
        : Expr{lineNum}, left{std::move(lhs)}, op{std::move(oper)}, right{std::move(rhs)} {}

    Value eval(Environment& env) const override;
};

struct FunctionCallExpr : Expr
{
    std::string name{};
    std::vector<std::unique_ptr<Expr>> args{};

    FunctionCallExpr(std::string funcName, std::vector<std::unique_ptr<Expr>> arguments, std::size_t lineNum)
        : Expr{lineNum}, name{std::move(funcName)}, args{std::move(arguments)} {}

    Value eval(Environment& env) const override;
};

struct ArrayMethodCallExpr : Expr
{
    std::string arrayName{};
    std::string method{};
    std::vector<std::unique_ptr<Expr>> args{};

    ArrayMethodCallExpr(std::string receiver, std::string methodName, std::vector<std::unique_ptr<Expr>> arguments, std::size_t lineNum)
        : Expr{lineNum}, arrayName{std::move(receiver)}, method{std::move(methodName)}, args{std::move(arguments)} {}

    Value eval(Environment& env) const override;
};
struct MemberExpr : Expr { std::string object{}, member{}; std::vector<std::unique_ptr<Expr>> args{}; bool isCall{}; MemberExpr(std::string o, std::string m, std::vector<std::unique_ptr<Expr>> a, bool call, std::size_t l): Expr{l}, object{std::move(o)}, member{std::move(m)}, args{std::move(a)}, isCall{call} {} Value eval(Environment&) const override; };

struct ReturnException
{
    Value value{};
};

struct BreakException {};
struct ContinueException {};

struct BreakStmt : Stmt
{
    explicit BreakStmt(std::size_t lineNum) : Stmt{lineNum} {}
    void exec(Environment&) const override { throw BreakException{}; }
};

struct ContinueStmt : Stmt
{
    explicit ContinueStmt(std::size_t lineNum) : Stmt{lineNum} {}
    void exec(Environment&) const override { throw ContinueException{}; }
};

struct BlockStmt : Stmt
{
    std::vector<std::unique_ptr<Stmt>> statements{};
    BlockStmt(std::vector<std::unique_ptr<Stmt>> body, std::size_t lineNum)
        : Stmt{lineNum}, statements{std::move(body)} {}

    void exec(Environment& env) const override;
};

struct DeclStmt : Stmt
{
    DataType type{};
    std::string name{};
    bool isDynamic{false};
    std::unique_ptr<Expr> initializer{};
    std::vector<std::unique_ptr<Expr>> arrayItems{};

    DeclStmt(DataType dataType, std::string varName, std::size_t lineNum)
        : Stmt{lineNum}, type{dataType}, name{std::move(varName)} {}

    void exec(Environment& env) const override;
};

struct ObjectDeclStmt : Stmt
{
    std::string className{};
    std::string name{};
    std::unique_ptr<Expr> initializer{};
    ObjectDeclStmt(std::string typeName, std::string varName, std::unique_ptr<Expr> value, std::size_t lineNum)
        : Stmt{lineNum}, className{std::move(typeName)}, name{std::move(varName)}, initializer{std::move(value)} {}
    void exec(Environment& env) const override;
};

struct ReturnStmt : Stmt
{
    std::unique_ptr<Expr> value{};
    ReturnStmt(std::unique_ptr<Expr> val, std::size_t lineNum)
        : Stmt{lineNum}, value{std::move(val)} {}

    void exec(Environment& env) const override;
};

struct AssignStmt : Stmt
{
    std::string name{};
    std::string op{};
    std::unique_ptr<Expr> value{};
    std::unique_ptr<Expr> index{};

    AssignStmt(std::string varName, std::string assignmentOp, std::unique_ptr<Expr> val, std::unique_ptr<Expr> idx, std::size_t lineNum)
        : Stmt{lineNum}, name{std::move(varName)}, op{std::move(assignmentOp)}, value{std::move(val)}, index{std::move(idx)} {}

    void exec(Environment& env) const override;
};
struct MemberAssignStmt : Stmt { std::string object{}, member{}, op{}; std::unique_ptr<Expr> value{}; MemberAssignStmt(std::string o,std::string m,std::string p,std::unique_ptr<Expr> v,std::size_t l):Stmt{l},object{std::move(o)},member{std::move(m)},op{std::move(p)},value{std::move(v)}{} void exec(Environment&) const override; };

struct PrintStmt : Stmt
{
    std::unique_ptr<Expr> expr{};
    PrintStmt(std::unique_ptr<Expr> valueExpr, std::size_t lineNum) : Stmt{lineNum}, expr{std::move(valueExpr)} {}
    void exec(Environment& env) const override;
};

struct ExprStmt : Stmt
{
    std::unique_ptr<Expr> expr{};
    ExprStmt(std::unique_ptr<Expr> valueExpr, std::size_t lineNum) : Stmt{lineNum}, expr{std::move(valueExpr)} {}
    void exec(Environment& env) const override;
};

struct IfStmt : Stmt
{
    std::unique_ptr<Expr> condition{};
    std::unique_ptr<BlockStmt> thenBlock{};
    std::unique_ptr<Stmt> elseBranch{};

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> thenBody, std::unique_ptr<Stmt> elseBody, std::size_t lineNum)
        : Stmt{lineNum}, condition{std::move(cond)}, thenBlock{std::move(thenBody)}, elseBranch{std::move(elseBody)} {}

    void exec(Environment& env) const override;
};

struct WhileStmt : Stmt
{
    std::unique_ptr<Expr> condition{};
    std::unique_ptr<BlockStmt> body{};

    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> loopBody, std::size_t lineNum)
        : Stmt{lineNum}, condition{std::move(cond)}, body{std::move(loopBody)} {}

    void exec(Environment& env) const override;
};

struct ForStmt : Stmt
{
    std::unique_ptr<Stmt> initializer{};
    std::unique_ptr<Expr> condition{};
    std::unique_ptr<Stmt> increment{};
    std::unique_ptr<BlockStmt> body{};

    ForStmt(std::unique_ptr<Stmt> init, std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> inc, std::unique_ptr<BlockStmt> loopBody, std::size_t lineNum)
        : Stmt{lineNum}, initializer{std::move(init)}, condition{std::move(cond)}, increment{std::move(inc)}, body{std::move(loopBody)} {}

    void exec(Environment& env) const override;
};

struct FunctionDefStmt : Stmt
{
    std::string name{};
    std::vector<std::string> params{};
    std::unique_ptr<BlockStmt> body{};
    bool isVoid{};

    FunctionDefStmt(std::string funcName, std::vector<std::string> parameters, std::unique_ptr<BlockStmt> funcBody, std::size_t lineNum, bool returnsVoid = false)
        : Stmt{lineNum}, name{std::move(funcName)}, params{std::move(parameters)}, body{std::move(funcBody)}, isVoid{returnsVoid} {}

    void exec(Environment& env) const override;
    Value call(Environment& env) const;
};
struct ClassDefStmt : Stmt { std::string name{}; std::vector<std::unique_ptr<DeclStmt>> fields{}; std::unique_ptr<FunctionDefStmt> constructor{}; std::unordered_map<std::string,std::unique_ptr<FunctionDefStmt>> methods{}; ClassDefStmt(std::string n,std::size_t l):Stmt{l},name{std::move(n)}{} void exec(Environment&) const override; };

struct ImportStmt : Stmt
{
    std::string fileName{};
    ImportStmt(std::string file, std::size_t lineNum)
        : Stmt{lineNum}, fileName{std::move(file)} {}

    void exec(Environment& env) const override;
};

#endif // AST_H
