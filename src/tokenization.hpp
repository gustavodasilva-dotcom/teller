#pragma once

#include <vector>
#include <optional>
#include <string>

enum class TokenType
{
    exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq,
    plus,
    star,
    minus,
    fslash,
    open_curly,
    close_curly,
    if_cond
};

struct Token
{
    TokenType type;
    std::optional<std::string> value{};
};

inline std::optional<int> bin_prec(const TokenType type)
{
    switch (type)
    {
    case TokenType::plus:
    case TokenType::minus:
        return 0;
    case TokenType::star:
    case TokenType::fslash:
        return 1;
    default:
        return {};
    }
}

class Tokenizer
{
public:
    explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;

        std::string buf;

        while (peek().has_value())
        {
            if (std::isalpha(peek().value()))
            {
                buf.push_back(consume());

                while (peek().has_value() && std::isalnum(peek().value()))
                {
                    buf.push_back(consume());
                }

                if (buf == "exit")
                {
                    tokens.push_back({.type = TokenType::exit});
                    buf.clear();
                }
                else if (buf == "let")
                {
                    tokens.push_back({.type = TokenType::let});
                    buf.clear();
                }
                else if (buf == "if")
                {
                    tokens.push_back({.type = TokenType::if_cond});
                    buf.clear();
                }
                else
                {
                    tokens.push_back({.type = TokenType::ident, .value = buf});
                    buf.clear();
                }
            }
            else if (std::isdigit(peek().value()))
            {
                buf.push_back(consume());

                while (peek().has_value() && std::isdigit(peek().value()))
                {
                    buf.push_back(consume());
                }

                tokens.push_back({.type = TokenType::int_lit, .value = buf});
                buf.clear();
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/')
            {
                consume();
                consume();

                while (peek().has_value() && peek().value() != '\n')
                {
                    consume();
                }
            }
            else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*')
            {
                consume();
                consume();

                while (peek().has_value())
                {
                    if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/')
                    {
                        break;
                    }

                    consume();
                }

                consume();
                consume();
            }
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::open_paren});
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::close_paren});
            }
            else if (peek().value() == ';')
            {
                consume();
                tokens.push_back({.type = TokenType::semi});
            }
            else if (peek().value() == '=')
            {
                consume();
                tokens.push_back({.type = TokenType::eq});
            }
            else if (peek().value() == '+')
            {
                consume();
                tokens.push_back({.type = TokenType::plus});
            }
            else if (peek().value() == '*')
            {
                consume();
                tokens.push_back({.type = TokenType::star});
            }
            else if (peek().value() == '-')
            {
                consume();
                tokens.push_back({.type = TokenType::minus});
            }
            else if (peek().value() == '/')
            {
                consume();
                tokens.push_back({.type = TokenType::fslash});
            }
            else if (peek().value() == '{')
            {
                consume();
                tokens.push_back({.type = TokenType::open_curly});
            }
            else if (peek().value() == '}')
            {
                consume();
                tokens.push_back({.type = TokenType::close_curly});
            }
            else if (std::isspace(peek().value()))
            {
                consume();
            }
            else
            {
                std::cerr << "Unknown keyword." << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        m_index = 0;

        return tokens;
    }

private:
    [[nodiscard]] std::optional<char> peek(size_t offset = 0) const
    {
        if (m_index + offset >= m_src.length())
        {
            return {};
        }

        return m_src.at(m_index + offset);
    }

    char consume()
    {
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index = 0;
};
