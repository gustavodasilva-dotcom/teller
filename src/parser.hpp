#pragma once

#include <vector>

#include "./arena.hpp"

struct NodeExpr;

struct NodeTermIntLit
{
    Token int_lit;
};

struct NodeTermIdent
{
    Token ident;
};

struct NodeTermParen
{
    NodeExpr *expr;
};

struct NodeBinExprAdd
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprSub
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprMulti
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExprDiv
{
    NodeExpr *lhs;
    NodeExpr *rhs;
};

struct NodeBinExpr
{
    std::variant<NodeBinExprAdd *, NodeBinExprSub *, NodeBinExprMulti *, NodeBinExprDiv *> var;
};

struct NodeTerm
{
    std::variant<NodeTermIntLit *, NodeTermIdent *, NodeTermParen *> var;
};

struct NodeExpr
{
    std::variant<NodeTerm *, NodeBinExpr *> var;
};

struct NodeStmtExit
{
    NodeExpr *expr;
};

struct NodeStmtLet
{
    Token ident;
    NodeExpr *expr;
};

struct NodeStmt
{
    std::variant<NodeStmtExit *, NodeStmtLet *> var;
};

struct NodeProg
{
    std::vector<NodeStmt *> stmts;
};

class Parser
{
public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_allocator(1024 * 1024 * 4) // 4 mb
    {
    }

    std::optional<NodeTerm *> parse_term()
    {
        if (auto int_lit = try_consume(TokenType::int_lit))
        {
            auto *term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            term_int_lit->int_lit = int_lit.value();

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_int_lit;

            return term;
        }
        else if (auto ident = try_consume(TokenType::ident))
        {
            auto term_ident = m_allocator.alloc<NodeTermIdent>();
            term_ident->ident = ident.value();

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;

            return term;
        }
        else if (auto open_paren = try_consume(TokenType::open_paren))
        {
            auto expr = parse_expr();

            if (!expr.has_value())
            {
                std::cerr << "Expected expression." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::close_paren, "Expected `)`.");

            auto term_paren = m_allocator.alloc<NodeTermParen>();
            term_paren->expr = expr.value();

            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_paren;

            return term;
        }
        else
        {
            return {};
        }
    }

    std::optional<NodeExpr *> parse_expr(int min_prec = 0)
    {
        std::optional<NodeTerm *> term_lhs = parse_term();
        if (!term_lhs.has_value())
        {
            return {};
        }

        auto expr_lhs = m_allocator.alloc<NodeExpr>();
        expr_lhs->var = term_lhs.value();

        while (true)
        {
            std::optional<int> prec;

            std::optional<Token> curr_tok = peek();

            if (curr_tok.has_value())
            {
                prec = bin_prec(curr_tok->type);

                if (!prec.has_value() || prec < min_prec)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            Token op = consume();

            int next_min_rec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_rec);

            if (!expr_rhs.has_value())
            {
                std::cerr << "Unable to parse expression." << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator.alloc<NodeBinExpr>();

            auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
            expr_lhs2->var = expr_lhs->var;

            if (op.type == TokenType::plus)
            {
                auto add = m_allocator.alloc<NodeBinExprAdd>();
                add->lhs = expr_lhs2;
                add->rhs = expr_rhs.value();

                expr->var = add;
            }
            else if (op.type == TokenType::sub)
            {
                auto sub = m_allocator.alloc<NodeBinExprSub>();
                sub->lhs = expr_lhs2;
                sub->rhs = expr_rhs.value();

                expr->var = sub;
            }
            else if (op.type == TokenType::star)
            {
                auto multi = m_allocator.alloc<NodeBinExprMulti>();
                multi->lhs = expr_lhs2;
                multi->rhs = expr_rhs.value();

                expr->var = multi;
            }
            else if (op.type == TokenType::div)
            {
                auto div = m_allocator.alloc<NodeBinExprDiv>();
                div->lhs = expr_lhs2;
                div->rhs = expr_rhs.value();

                expr->var = div;
            }

            expr_lhs->var = expr;
        }

        return expr_lhs;
    }

    std::optional<NodeStmt *> parse_stmt()
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

            auto stmt_exit = m_allocator.alloc<NodeStmtExit>();

            if (auto node_expr = parse_expr())
            {
                stmt_exit->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::close_paren, "Missing `)`.");

            try_consume(TokenType::semi, "Missing `;`.");

            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_exit;

            return stmt;
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
                std::cerr << "Missing `=`." << std::endl;
                exit(EXIT_FAILURE);
            }

            consume();

            auto stmt_let = m_allocator.alloc<NodeStmtLet>();
            stmt_let->ident = consume();

            consume();

            if (auto node_expr = parse_expr())
            {
                stmt_let->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::semi, "Expected `;`.");

            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = stmt_let;

            return stmt;
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

    inline std::optional<Token> try_consume(TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            return {};
        }
    }

    inline Token try_consume(TokenType type, const std::string &err_msg)
    {
        if (auto token = try_consume(type))
        {
            return token.value();
        }
        else
        {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};
