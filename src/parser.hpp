#pragma once

#include <vector>

#include "./types.hpp"

class Parser
{
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens))
    {
    }

    std::optional<NodeExpr> parse_expr()
    {
        if (peek().has_value() && peek().value().type == TokenType::int_lit)
        {
            return NodeExpr{.var = NodeExprIntLit{.int_lit = consume()}};
        }
        else if (peek().has_value() && peek().value().type == TokenType::ident)
        {
            return NodeExpr{.var = NodeExprIdent{.ident = consume()}};
        }
        else
        {
            return {};
        }
    }

    std::optional<NodeStmt> parse_stmt()
    {
        if (peek().value().type == TokenType::exit)
        {
            if (peek(1).has_value() && peek(1).value().type != TokenType::open_paren)
            {
                std::cerr << "Missing `(`." << std::endl;
                exit(EXIT_FAILURE);
            }

            consume();
            consume();

            NodeStmtExit exit_stmt;

            if (auto node_expr = parse_expr())
            {
                exit_stmt = {.expr = node_expr.value()};
            }
            else
            {
                std::cerr << "Invalid expression." << std::endl;
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() && peek().value().type == TokenType::close_paren)
            {
                consume();
            }
            else
            {
                std::cerr << "Missing `)`." << std::endl;
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() && peek().value().type == TokenType::semi)
            {
                consume();
            }
            else
            {
                std::cerr << "Missing `;`." << std::endl;
                exit(EXIT_FAILURE);
            }

            return NodeStmt{.var = exit_stmt};
        }
        else if (peek().has_value() && peek().value().type == TokenType::let)
        {
            if (peek(1).has_value() && peek(1).value().type != TokenType::ident)
            {
                std::cerr << "Missing variable identifier." << std::endl;
                exit(EXIT_FAILURE);
            }

            if (peek(2).has_value() && peek(2).value().type != TokenType::eq)
            {
                std::cerr << "Missing equal sign." << std::endl;
                exit(EXIT_FAILURE);
            }

            consume();

            auto stmt_let = NodeStmtLet{.ident = consume()};

            consume();

            if (auto node_expr = parse_expr())
            {
                stmt_let.expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression." << std::endl;
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() && peek().value().type == TokenType::semi)
            {
                consume();
            }
            else
            {
                std::cerr << "Expected `;`." << std::endl;
                exit(EXIT_FAILURE);
            }

            return NodeStmt{.var = stmt_let};
        }
        else
        {
            return {};
        }
    }

    std::optional<NodeProg> parse_prog()
    {
        NodeProg prog;

        while (peek().has_value())
        {
            if (auto stmt = parse_stmt())
            {
                prog.stmts.push_back(stmt.value());
            }
            else
            {
                std::cerr << "Invalid statement." << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        return prog;
    }

private:
    [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        }
        else
        {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume()
    {
        return m_tokens.at(m_index++);
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
};
