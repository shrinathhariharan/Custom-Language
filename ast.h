#ifndef AST_H
#define AST_H

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

enum class DataType
{
    t_int,
    t_dec,
    t_str,
    t_bool,
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

    Value& get(const std::string& name);
    const Value& get(const std::string& name) const;
    void declare(const std::string& name, Value value, std::size_t line);
};

std::string typeName(DataType type);
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

struct ReturnException
{
    Value value{};
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
    std::unique_ptr<Expr> value{};
    std::unique_ptr<Expr> index{};

    AssignStmt(std::string varName, std::unique_ptr<Expr> val, std::unique_ptr<Expr> idx, std::size_t lineNum)
        : Stmt{lineNum}, name{std::move(varName)}, value{std::move(val)}, index{std::move(idx)} {}

    void exec(Environment& env) const override;
};

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
    std::unique_ptr<BlockStmt> elseBlock{};

    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> thenBody, std::unique_ptr<BlockStmt> elseBody, std::size_t lineNum)
        : Stmt{lineNum}, condition{std::move(cond)}, thenBlock{std::move(thenBody)}, elseBlock{std::move(elseBody)} {}

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

    FunctionDefStmt(std::string funcName, std::vector<std::string> parameters, std::unique_ptr<BlockStmt> funcBody, std::size_t lineNum)
        : Stmt{lineNum}, name{std::move(funcName)}, params{std::move(parameters)}, body{std::move(funcBody)} {}

    void exec(Environment& env) const override;
    Value call(Environment& env) const;
};

#endif // AST_H
