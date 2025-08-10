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
    NodeExpr *expr{};
};

struct NodeScope;

struct NodeIfPred;

struct NodeIfPredElif
{
    NodeExpr *expr;
    NodeScope *scope;
    std::optional<NodeIfPred *> pred;
};

struct NodeIfPredElse
{
    NodeScope *scope;
};

struct NodeIfPred
{
    std::variant<NodeIfPredElse *, NodeIfPredElif *> var;
};

struct NodeStmtIf
{
    NodeExpr *expr;
    NodeScope *scope;
    std::optional<NodeIfPred *> pred;
};

struct NodeStmt;

struct NodeScope
{
    std::vector<NodeStmt *> stmts;
};

struct NodeStmtAssign
{
    Token ident;
    NodeExpr *expr{};
};

struct NodeStmt
{
    std::variant<NodeStmtExit *, NodeStmtLet *, NodeScope *, NodeStmtIf *, NodeStmtAssign *> var;
};

struct NodeProg
{
    std::vector<NodeStmt *> stmts;
};

class Parser
{
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_allocator(1024 * 1024 * 4) // 4 mb
    {
    }

    void error_expected_term(const std::string term) const
    {
        std::cerr << "Expected `" << term << "` on line " << peek(-1).value().line << "." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::optional<NodeTerm *> parse_term()
    {
        if (auto int_lit = try_consume(TokenType::int_lit))
        {
            auto term_int_lit = m_allocator.emplace<NodeTermIntLit>(int_lit.value());
            auto term = m_allocator.emplace<NodeTerm>(term_int_lit);

            return term;
        }

        if (auto ident = try_consume(TokenType::ident))
        {
            auto term_ident = m_allocator.emplace<NodeTermIdent>(ident.value());
            auto term = m_allocator.emplace<NodeTerm>(term_ident);

            return term;
        }

        if (const auto open_paren = try_consume(TokenType::open_paren))
        {
            auto expr = parse_expr();

            if (!expr.has_value())
            {
                std::cerr << "Expected expression on line " << open_paren.value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::close_paren);

            auto term_paren = m_allocator.emplace<NodeTermParen>(expr.value());
            auto term = m_allocator.emplace<NodeTerm>(term_paren);

            return term;
        }

        return {};
    }

    std::optional<NodeExpr *> parse_expr(const int min_prec = 0)
    {
        std::optional<NodeTerm *> term_lhs = parse_term();
        if (!term_lhs.has_value())
        {
            return {};
        }

        auto expr_lhs = m_allocator.emplace<NodeExpr>(term_lhs.value());

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

            const auto [type, line, value] = consume();

            const int next_min_rec = prec.value() + 1;
            auto expr_rhs = parse_expr(next_min_rec);

            if (!expr_rhs.has_value())
            {
                std::cerr << "Unable to parse expression on line " << line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator.emplace<NodeBinExpr>();

            auto expr_lhs2 = m_allocator.emplace<NodeExpr>();
            expr_lhs2->var = expr_lhs->var;

            if (type == TokenType::plus)
            {
                auto add = m_allocator.emplace<NodeBinExprAdd>(expr_lhs2, expr_rhs.value());
                expr->var = add;
            }
            else if (type == TokenType::minus)
            {
                auto sub = m_allocator.emplace<NodeBinExprSub>(expr_lhs2, expr_rhs.value());
                expr->var = sub;
            }
            else if (type == TokenType::star)
            {
                auto multi = m_allocator.emplace<NodeBinExprMulti>(expr_lhs2, expr_rhs.value());
                expr->var = multi;
            }
            else if (type == TokenType::fslash)
            {
                auto div = m_allocator.emplace<NodeBinExprDiv>(expr_lhs2, expr_rhs.value());
                expr->var = div;
            }

            expr_lhs->var = expr;
        }

        return expr_lhs;
    }

    std::optional<NodeScope *> parse_scope()
    {
        if (!try_consume(TokenType::open_curly).has_value())
        {
            return {};
        }

        auto scope = m_allocator.emplace<NodeScope>();

        while (auto stmt = parse_stmt())
        {
            scope->stmts.push_back(stmt.value());
        }

        try_consume_err(TokenType::close_curly);

        return scope;
    }

    std::optional<NodeIfPred *> parse_if_pred()
    {
        if (try_consume(TokenType::elif))
        {
            try_consume_err(TokenType::open_paren);

            const auto elif = m_allocator.emplace<NodeIfPredElif>();

            if (const auto expr = parse_expr())
            {
                elif->expr = expr.value();
            }
            else
            {
                std::cerr << "Expected expression on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::close_paren);

            if (const auto scope = parse_scope())
            {
                elif->scope = scope.value();
            }
            else
            {
                std::cerr << "Expected scope on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            elif->pred = parse_if_pred();

            auto pred = m_allocator.emplace<NodeIfPred>(elif);
            return pred;
        }

        if (try_consume(TokenType::else_cond))
        {
            const auto else_cond = m_allocator.emplace<NodeIfPredElse>();

            if (const auto scope = parse_scope())
            {
                else_cond->scope = scope.value();
            }
            else
            {
                std::cerr << "Expected scope on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            auto pred = m_allocator.emplace<NodeIfPred>(else_cond);
            return pred;
        }

        return {};
    }

    std::optional<NodeStmt *> parse_stmt()
    {
        if (peek().has_value() && peek().value().type == TokenType::exit)
        {
            if (peek(1).has_value() && peek(1).value().type != TokenType::open_paren)
            {
                std::cerr << "Missing `(` on line " << peek(1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            consume();
            consume();

            auto stmt_exit = m_allocator.emplace<NodeStmtExit>();

            if (const auto node_expr = parse_expr())
            {
                stmt_exit->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::close_paren);

            try_consume_err(TokenType::semi);

            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = stmt_exit;

            return stmt;
        }

        if (peek().has_value() && peek().value().type == TokenType::let)
        {
            if (peek(1).has_value() && peek(1).value().type != TokenType::ident)
            {
                std::cerr << "Missing variable identifier on line " << peek(1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            if (peek(2).has_value() && peek(2).value().type != TokenType::eq)
            {
                std::cerr << "Missing `=` on line " << peek(2).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            consume(); // Consume the 'let' token.

            auto stmt_let = m_allocator.emplace<NodeStmtLet>();
            stmt_let->ident = consume();

            consume(); // Consume the equal sign.

            if (const auto node_expr = parse_expr())
            {
                stmt_let->expr = node_expr.value();
            }
            else
            {
                std::cerr << "Invalid expression on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::semi);

            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = stmt_let;

            return stmt;
        }

        if (peek().has_value() && peek().value().type == TokenType::ident)
        {
            if (peek(1).has_value() && peek(1).value().type != TokenType::eq)
            {
                std::cerr << "Missing `=` on line " << peek(1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            const auto assign = m_allocator.emplace<NodeStmtAssign>();
            assign->ident = consume();

            consume(); // Consume the equal sign.

            if (const auto expr = parse_expr())
            {
                assign->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::semi);

            auto stmt = m_allocator.emplace<NodeStmt>();
            stmt->var = assign;

            return stmt;
        }

        if (peek().has_value() && peek().value().type == TokenType::open_curly)
        {
            if (const auto scope = parse_scope())
            {
                auto stmt = m_allocator.emplace<NodeStmt>(scope.value());
                return stmt;
            }

            std::cerr << "Invalid scope on line " << peek(-1).value().line << "." << std::endl;
            exit(EXIT_FAILURE);
        }

        if (auto if_cond = try_consume(TokenType::if_cond))
        {
            try_consume_err(TokenType::open_paren);

            auto stmt_if = m_allocator.emplace<NodeStmtIf>();

            if (const auto expr = parse_expr())
            {
                stmt_if->expr = expr.value();
            }
            else
            {
                std::cerr << "Invalid expression on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            try_consume_err(TokenType::close_paren);

            if (auto scope = parse_scope())
            {
                stmt_if->scope = scope.value();
            }
            else
            {
                std::cerr << "Invalid scope on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }

            stmt_if->pred = parse_if_pred();

            auto stmt = m_allocator.emplace<NodeStmt>(stmt_if);
            return stmt;
        }

        return {};
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
                std::cerr << "Invalid statement on line " << peek(-1).value().line << "." << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        return prog;
    }

private:
    [[nodiscard]] std::optional<Token> peek(const int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        }

        return m_tokens.at(m_index + offset);
    }

    Token consume()
    {
        return m_tokens.at(m_index++);
    }

    std::optional<Token> try_consume(const TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }

        return {};
    }

    Token try_consume_err(TokenType type)
    {
        if (auto token = try_consume(type))
        {
            return token.value();
        }

        error_expected_term(to_string(type));
        return {};
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};
