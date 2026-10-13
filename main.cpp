#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <utility>
#include <climits>
using namespace std;

// ---------- 1. Lexer ----------
enum class TokenType {
    NUMBER, PLUS, MINUS, MUL, DIV,
    LPAREN, RPAREN, END
};

struct Token {
    TokenType type;
    long long value = 0;
};

vector<Token> tokenize(const string& input) {
    vector<Token> tokens;

    for (size_t i = 0; i < input.size();) {
        char c = input[i];

        if (isspace(static_cast<unsigned char>(c))) {
            i++;
        }
        else if (isdigit(static_cast<unsigned char>(c))) {
            long long value = 0;

            while (i < input.size() &&
                   isdigit(static_cast<unsigned char>(input[i]))) {
                int digit = input[i] - '0';

                if (value > (LLONG_MAX - digit) / 10)
                    throw runtime_error("Integer literal overflow");

                value = value * 10 + digit;
                i++;
            }

            tokens.push_back({TokenType::NUMBER, value});
        }
        else {
            TokenType type;

            switch (c) {
                case '+': type = TokenType::PLUS; break;
                case '-': type = TokenType::MINUS; break;
                case '*': type = TokenType::MUL; break;
                case '/': type = TokenType::DIV; break;
                case '(': type = TokenType::LPAREN; break;
                case ')': type = TokenType::RPAREN; break;
                default:
                    throw runtime_error(
                        string("Invalid character: ") + c
                    );
            }

            tokens.push_back({type, 0});
            i++;
        }
    }

    tokens.push_back({TokenType::END, 0});
    return tokens;
}

// ---------- 2. AST ----------
struct Node {
    enum class Kind { NUMBER, UNARY, BINARY };

    Kind kind;
    long long value = 0;
    char op = '\0';
    unique_ptr<Node> left, right;

    explicit Node(long long v) : kind(Kind::NUMBER), value(v) {}

    Node(char operation, unique_ptr<Node> child)
        : kind(Kind::UNARY), op(operation),
          right(move(child)) {}

    Node(char operation, unique_ptr<Node> lhs,
         unique_ptr<Node> rhs)
        : kind(Kind::BINARY), op(operation),
          left(move(lhs)), right(move(rhs)) {}
};

// ---------- 3. Parser ----------
class Parser {
    vector<Token> tokens;
    size_t pos = 0;

    Token current() const {
        return tokens[pos];
    }

    Token consume() {
        return tokens[pos++];
    }

    bool match(TokenType type) {
        if (current().type == type) {
            consume();
            return true;
        }
        return false;
    }

    void expect(TokenType type) {
        if (!match(type))
            throw runtime_error("Unexpected token");
    }

    // expression = term (('+' | '-') term)*
    unique_ptr<Node> expression() {
        auto node = term();

        while (current().type == TokenType::PLUS ||
               current().type == TokenType::MINUS) {
            char op = consume().type == TokenType::PLUS
                      ? '+' : '-';

            auto rhs = term();

            node = make_unique<Node>(
                op, move(node), move(rhs)
            );
        }

        return node;
    }

    // term = unary (('*' | '/') unary)*
    unique_ptr<Node> term() {
        auto node = unary();

        while (current().type == TokenType::MUL ||
               current().type == TokenType::DIV) {
            char op = consume().type == TokenType::MUL
                      ? '*' : '/';

            auto rhs = unary();

            node = make_unique<Node>(
                op, move(node), move(rhs)
            );
        }

        return node;
    }

    // unary = ('+' | '-') unary | primary
    unique_ptr<Node> unary() {
        if (match(TokenType::PLUS))
            return make_unique<Node>('+', unary());

        if (match(TokenType::MINUS))
            return make_unique<Node>('-', unary());

        return primary();
    }

    // primary = NUMBER | '(' expression ')'
    unique_ptr<Node> primary() {
        if (current().type == TokenType::NUMBER) {
            return make_unique<Node>(consume().value);
        }

        if (match(TokenType::LPAREN)) {
            auto node = expression();
            expect(TokenType::RPAREN);
            return node;
        }

        throw runtime_error("Expected a number or '('");
    }

public:
    explicit Parser(vector<Token> t) : tokens(move(t)) {}

    unique_ptr<Node> parse() {
        auto root = expression();

        if (current().type != TokenType::END)
            throw runtime_error("Unexpected trailing input");

        return root;
    }
};

// ---------- 4. AST Evaluation ----------
long long evaluate(const Node* node) {
    if (node->kind == Node::Kind::NUMBER)
        return node->value;

    if (node->kind == Node::Kind::UNARY) {
        long long value = evaluate(node->right.get());

        if (node->op == '-')
            return -value;

        return value;
    }

    long long a = evaluate(node->left.get());
    long long b = evaluate(node->right.get());

    switch (node->op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;

        case '/':
            if (b == 0)
                throw runtime_error("Division by zero");

            if (a == LLONG_MIN && b == -1)
                throw runtime_error("Integer overflow");

            return a / b;
    }

    throw runtime_error("Unknown operator");
}

// ---------- 5. Main ----------
int main() {
    string input;

    cout << "Enter expression: ";
    getline(cin, input);

    try {
        auto tokens = tokenize(input);
        Parser parser(move(tokens));
        auto ast = parser.parse();

        cout << "Result: " << evaluate(ast.get()) << '\n';
    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << '\n';
    }

    return 0;
}