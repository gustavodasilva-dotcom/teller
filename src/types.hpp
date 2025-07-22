#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class TokenType
{
    exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq
};

struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

struct NodeExprIntLit
{
    Token int_lit;
};

struct NodeExprIdent
{
    Token ident;
};

struct NodeExpr
{
    std::variant<NodeExprIntLit, NodeExprIdent> var;
};

struct NodeStmtExit
{
    NodeExpr expr;
};

struct NodeStmtLet
{
    Token ident;
    NodeExpr expr;
};

struct NodeStmt
{
    std::variant<NodeStmtExit, NodeStmtLet> var;
};

struct NodeProg
{
    std::vector<NodeStmt> stmts;
};
