#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <iostream>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input):in(input) {
    emptyLine = true;
    newOffset = 0;
    offsetSpace = 0;
    NextToken();
}

const Token& Lexer::CurrentToken() const {
    return currentToken;
}

Token Lexer::NextToken() {
    currentToken = ReadToken(in);
    return currentToken;
}
    
Token Lexer::ReadToken(std::istream& input) {
    // выравниваем смещение
    if (newOffset > offsetSpace) {
        offsetSpace += 2;
        return Token(token_type::Indent());
    }
    
    if (newOffset < offsetSpace) {
        offsetSpace -= 2;
        return Token(token_type::Dedent());
    }
    
    // проверка на конец потока
    if (input.eof()) {
        if (!emptyLine) {
            emptyLine = true;
            return Token(token_type::Newline());
        }
        return Token(token_type::Eof());
    }
    
    // если пустая строка - считываем смещение
    if (emptyLine) {
        
        int tmpOffset = ReadOffset(input);
        if (input.eof()) {
            //if (offsetSpace > 0) {}
            newOffset = tmpOffset;
            return ReadToken(input);
        }
        // если строка пустая - пропускаем
        if (input.peek()=='\n') {
            input.get();
            return ReadToken(input);
        }
        newOffset = tmpOffset;
        
        // пропуск комментария
        if (input.peek() == '#') {
            SkipComment(input);
        } else {
            emptyLine = false;
        }
        
        return ReadToken(input);
    }
    
    // конец не пустой строки
    if ((input.peek()=='\n') && (!emptyLine)) {
        input.get();
        emptyLine = true;
        return Token(token_type::Newline());
    }
    
    char c = input.get();
    switch (c) {
        case '=':
        case '>':
        case '<':
        case '!':
            emptyLine = false;
            if (input.peek() == '=') {
                return ReadMultiplyChar(c, input);
            } else {
                return ReadSingleChar(c);
            }
            //break;
        case '.':
        case ',':
        case '(':
        case ')':
        case ':':    
        case '+':
        case '-':
        case '*':
        case '/':
            emptyLine = false;
            return ReadSingleChar(c);
        case '\'':
        case '\"':
            emptyLine = false;
            return ReadString(c, input);
        case '#':
            SkipComment(input);
            return ReadToken(input);
        default:
            if (IsDigit(c)) {
                emptyLine = false;
                return ReadNumber(c, input);
            }
            
            if (IsAplhabetic(c)) {
                emptyLine = false;
                return ReadSymbolToken(c, input);
            }
    }
    
    return ReadToken(input);
}    

Token Lexer::ReadMultiplyChar(char c, std::istream& input) {
    input.get();
    switch (c) {
        case '=':
            return Token(token_type::Eq());
        case '>':
            return Token(token_type::GreaterOrEq());
        case '<':
            return Token(token_type::LessOrEq());
        case '!':
            return Token(token_type::NotEq());
    }
    
    return Token(token_type::None());
}

bool Lexer::IsDigit(char c) {
    return (c >= '0') && (c <= '9');
}
    
bool Lexer::IsAplhabetic(char c) {
    return ((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')) ||
        (c == '_');
}
    
bool Lexer::IsAplhabeticOrDigit(char c) {
    return IsDigit(c) || IsAplhabetic(c);
}    
    
Token Lexer::ReadSingleChar(char c) {
    return Token(token_type::Char{c});
}

Token Lexer::ReadNumber(char first, std::istream& input) {
    string s;
    s.push_back(first);
    while (IsDigit(input.peek())) {
        s.push_back(input.get());
    }
    return Token(token_type::Number{stoi(s)});
}    

Token Lexer::ReadString(char first, std::istream& input) {
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw LexerError("String parsing error");
        }
        const char ch = *it;
        if (ch == first) {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw LexerError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\'':
                    s.push_back('\'');
                    break;    
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw LexerError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw LexerError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it;
    }
    
    return Token(token_type::String{s});
}
    
int Lexer::ReadOffset(std::istream& input) {
    int offset = 0;
    while ((!input.eof()) && (input.peek() == ' ')) {
        input.get();
        offset++;
    }
    return offset;
}
    
Token Lexer::ReadSymbolToken(char first, std::istream& input) {
    string s;
    s.push_back(first);
    while ((!input.eof()) && IsAplhabeticOrDigit(input.peek())) {
        char c = input.get();
        s.push_back(c);
    }
    
    using namespace std::literals;
    if (s == "class"s) {
        return Token(token_type::Class()); 
    }
    
    if (s == "return"s) {
        return Token(token_type::Return()); 
    }
    
    if (s == "if"s) {
        return Token(token_type::If()); 
    }
    
    if (s == "else"s) {
        return Token(token_type::Else()); 
    }
    
    if (s == "def"s) {
        return Token(token_type::Def()); 
    }
    
    if (s == "print"s) {
        return Token(token_type::Print()); 
    }
    
    if (s == "and"s) {
        return Token(token_type::And()); 
    }
    
    if (s == "or"s) {
        return Token(token_type::Or()); 
    }
    
    if (s == "not"s) {
        return Token(token_type::Not()); 
    }
    
    if (s == "None"s) {
        return Token(token_type::None()); 
    }
    
    if (s == "True"s) {
        return Token(token_type::True()); 
    }
    
    if (s == "False"s) {
        return Token(token_type::False()); 
    }
    
    return Token(token_type::Id{s});    
}
    
void Lexer::SkipComment(std::istream& input) {
    while ((!input.eof()) && (input.peek() != '\n')) {
        input.get();
    }
}
    
}  // namespace parse