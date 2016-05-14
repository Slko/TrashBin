#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>
#include <stack>
#include <map>
#include <cmath>
#include <functional>

enum class TokenType
{
    None,
    Number,
    Ident,
    Operation,
    Comma,
    OpenBracket,
    CloseBracket,
};

struct Token
{
    TokenType type;

    std::string str;
    double number;

    Token(TokenType type, const std::string &s) : type(type), str(s)
    {
    }

    Token(TokenType type, double n) : type(type), number(n)
    {
    }

    Token(TokenType type) : type(type)
    {
    }

    std::string repr()
    {
        std::ostringstream ss;

        switch (type)
        {
        case TokenType::None:
            return "Token(None)";
        case TokenType::Number:
            ss << "Token(Number, " << number << ")";
            return ss.str();
        case TokenType::Ident:
            ss << "Token(Ident, \"" << str << "\")";
            return ss.str();
        case TokenType::Comma:
            return "Token(Comma)";
        case TokenType::OpenBracket:
            return "Token(OpenBracket)";
        case TokenType::CloseBracket:
            return "Token(CloseBracket)";
        case TokenType::Operation:
            ss << "Token(Operation, " << str << ")";
            return ss.str();
        default:
            return "Token(INVALID)";
        }
    }
};

struct Function
{
    std::function<double(std::vector<double>)> func;
    int n;

    Function() { }
    Function(std::function<double(std::vector<double>)> f, int n) : func(f), n(n) { }
};

std::map<std::string, int> operator_priority;
std::map<std::string, std::function<double(double, double)>> operators;
std::map<std::string, Function> functions;
bool silent = false;

std::vector<Token> tokenize(const std::string &expr)
{
    std::vector<Token> tokens;
    TokenType current_token = TokenType::None;
    size_t token_start = 0;
    bool hadPoint = false; // 4214.412412.412124 - bad, 2141244.424 - good

    for (size_t i = 0; i < expr.size() + 1; i++)
    {
        char c = expr[i];

        switch (current_token)
        {
        case TokenType::None:
            if (isdigit(c))
            {
                current_token = TokenType::Number;
                token_start = i;
            }
            else if (c == ',')
            {
                tokens.push_back(Token(TokenType::Comma));
            }
            else if (c == '(')
            {
                tokens.push_back(Token(TokenType::OpenBracket));
            }
            else if (c == ')')
            {
                tokens.push_back(Token(TokenType::CloseBracket));
            }
            else if (isalpha(c))
            {
                current_token = TokenType::Ident;
                token_start = i;
            }
            else if (c == ' ')
            {
                continue;
            }
            else if (c == '\0')
            {
                break;
            }
            else
            {
                char op[] = { c, 0 };
                tokens.push_back(Token(TokenType::Operation, std::string(op)));
            }
            break;
        case TokenType::Number:
            if (isdigit(c))
            {
                continue;
            }
            else if (c == '.')
            {
                if (hadPoint)
                {
                    std::cout << "Unexpected character: ." << std::endl;
                    exit(0);
                }
                else
                {
                    hadPoint = true;
                }
            }
            else
            {
                tokens.push_back(Token(TokenType::Number, std::stod(expr.substr(token_start, i - token_start))));
                i--;
                current_token = TokenType::None;
                hadPoint = false;
            }
            break;
        case TokenType::Ident:
            if (isalnum(c))
            {
                continue;
            }
            else
            {
                tokens.push_back(Token(TokenType::Ident, expr.substr(token_start, i - token_start)));
                i--;
                current_token = TokenType::None;
            }
            break;
        }
    }

    return tokens;
}

int check_priority(const std::string &op1, const std::string &op2)
{
    return operator_priority[op1] - operator_priority[op2];
}

std::vector<Token> to_rpn(std::vector<Token> infix)
{
    std::vector<Token> result;
    std::stack<Token> stack;

    for (size_t i = 0; i < infix.size(); i++)
    {
        switch (infix[i].type)
        {
        case TokenType::Number:
            result.push_back(infix[i]);
            break;
        case TokenType::Ident:
        case TokenType::OpenBracket:
            stack.push(infix[i]);
            break;
        case TokenType::CloseBracket:
            while (true)
            {
                Token top = stack.top();
                if (top.type == TokenType::OpenBracket)
                {
                    stack.pop();
                    break;
                }
                result.push_back(top);
                stack.pop();
            }
            if (!stack.empty() && stack.top().type == TokenType::Ident)
            {
                result.push_back(stack.top());
                stack.pop();
            }
            break;
        case TokenType::Operation:
            while (true)
            {
                if (stack.empty())
                {
                    break;
                }

                if (check_priority(infix[i].str, stack.top().str) <= 0)
                {
                    result.push_back(stack.top());
                    stack.pop();
                }
                else
                {
                    break;
                }
            }
            stack.push(infix[i]);
            break;
        }
    }

    while (!stack.empty())
    {
        result.push_back(stack.top());
        stack.pop();
    }

    return result;
}

double eval(const std::string &expr)
{
    std::vector<Token> tokens = tokenize(expr);

    if (!silent)
    {
        std::cout << "Infix:" << std::endl;
        for (size_t i = 0; i < tokens.size(); i++)
        {
            std::cout << tokens[i].repr() << std::endl;
        }
        std::cout << std::endl;
    }

    tokens = to_rpn(tokens);

    if (!silent)
    {
        std::cout << "Reverse Polish notation:" << std::endl;
        for (size_t i = 0; i < tokens.size(); i++)
        {
            std::cout << tokens[i].repr() << std::endl;
        }
        std::cout << std::endl;
    }

    std::stack<double> stack;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        switch (tokens[i].type)
        {
        case TokenType::Number:
            stack.push(tokens[i].number);
            break;
        case TokenType::Ident:
            if (functions.find(tokens[i].str) == functions.end())
            {
                std::cout << "Unknown function: " << tokens[i].str << std::endl;
                return 0;
            }
            else
            {
                std::vector<double> args;
                Function f = functions[tokens[i].str];
                for (int i = 0; i < f.n; i++)
                {
                    args.push_back(stack.top());
                    stack.pop();
                }
                std::reverse(args.begin(), args.end());
                stack.push(f.func(args));
            }
            break;
        case TokenType::Operation:
            if (operators.find(tokens[i].str) == operators.end())
            {
                std::cout << "Unknown operation: " << tokens[i].str << std::endl;
                return 0;
            }
            else
            {
                double b = stack.top(); stack.pop();
                double a = stack.top(); stack.pop();
                stack.push(operators[tokens[i].str](a, b));
            }
            break;
        }
    }

    if (stack.size() != 1)
    {
        std::cout << "Bad expression" << std::endl;
        return 0;
    }

    return stack.top();
}

void init_expr()
{
    operator_priority["+"] = 1;
    operator_priority["-"] = 1;
    operator_priority["*"] = 2;
    operator_priority["/"] = 2;
    operator_priority["^"] = 3;

    operators["+"] = [](double a, double b) { return a + b; };
    operators["-"] = [](double a, double b) { return a - b; };
    operators["*"] = [](double a, double b) { return a * b; };
    operators["/"] = [](double a, double b) { return a / b; };
    operators["^"] = [](double a, double b) { return pow(a, b); };

    functions["sin"] = Function([](std::vector<double> v) { return sin(v[0]); }, 1);
    functions["cos"] = Function([](std::vector<double> v) { return cos(v[0]); }, 1);
    functions["pow"] = Function([](std::vector<double> v) { return pow(v[0], v[1]); }, 2);
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        if (strcmp("/q", argv[1]) == 0)
        {
            silent = true;
        }
    }

    init_expr();

    if (!silent) std::cout << "Expression: ";
    std::string expr;
    std::getline(std::cin, expr);
    if (!silent) std::cout << std::endl;

    if (!silent) std::cout << "Result: ";
    std::cout << eval(expr);
    if (!silent) std::cout << std::endl;

    return 0;
}
