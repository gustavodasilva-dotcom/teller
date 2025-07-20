#pragma once

#include <optional>
#include <string>

enum class TokenType
{
    exit,
    int_lit,
    semi
};

struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

struct NodeExpr
{
    Token int_lit;
};

struct NodeExit
{
    NodeExpr expr;
};
