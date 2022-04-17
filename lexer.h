#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace parse {

namespace token_type {
struct Number {  // Лексема «число»
    int value;   // число
};

struct Id {             // Лексема «идентификатор»
    std::string value;  // Имя идентификатора
};

struct Char {    // Лексема «символ»
    char value;  // код символа
};

struct String {  // Лексема «строковая константа»
    std::string value;
};

struct Class {};    // Лексема «class»
struct Return {};   // Лексема «return»
struct If {};       // Лексема «if»
struct Else {};     // Лексема «else»
struct Def {};      // Лексема «def»
struct Newline {};  // Лексема «конец строки»
struct Print {};    // Лексема «print»
struct Indent {};  // Лексема «увеличение отступа», соответствует двум пробелам
struct Dedent {};  // Лексема «уменьшение отступа»
struct Eof {};     // Лексема «конец файла»
struct And {};     // Лексема «and»
struct Or {};      // Лексема «or»
struct Not {};     // Лексема «not»
struct Eq {};      // Лексема «==»
struct NotEq {};   // Лексема «!=»
struct LessOrEq {};     // Лексема «<=»
struct GreaterOrEq {};  // Лексема «>=»
struct None {};         // Лексема «None»
struct True {};         // Лексема «True»
struct False {};        // Лексема «False»
}  // namespace token_type

using TokenBase
    = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
                   token_type::Class, token_type::Return, token_type::If, token_type::Else,
                   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                   token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T& As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T* TryAs() const {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Lexer {
public:
    explicit Lexer(std::istream& input);

    // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
    [[nodiscard]] const Token& CurrentToken() const;

    // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
    Token NextToken();

    // Если текущий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& Expect() const {
        using namespace std::literals;
        if (currentToken.Is<T>()) {
            return currentToken.As<T>();
        } else {
            throw LexerError("Not implemented"s);
        }
    }

    // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void Expect(const U& value) const {
        using namespace std::literals;
        if (currentToken.Is<T>()) {
            auto p = currentToken.TryAs<T>();
            if (p && (static_cast<U>(p->value) == value)) {
                return;
            } else {
                throw LexerError("Not implemented"s);
            }
        } else {
            throw LexerError("Not implemented"s);
        }
    }

    // Если следующий токен имеет тип T, метод возвращает ссылку на него.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T>
    const T& ExpectNext() {
        using namespace std::literals;
        NextToken();
        if (currentToken.Is<T>()) {
            return currentToken.As<T>();
        } else {
            throw LexerError("Not implemented"s);
        }
    }

    // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
    // В противном случае метод выбрасывает исключение LexerError
    template <typename T, typename U>
    void ExpectNext(const U& value) {
        using namespace std::literals;
        NextToken();
        if (currentToken.Is<T>()) {
            auto p = currentToken.TryAs<T>();
            if (p && (static_cast<U>(p->value) == value)) {
                return;
            } else {
                throw LexerError("Not implemented"s);
            }
        } else {
            throw LexerError("Not implemented"s);
        }
    }

private:
    Token currentToken = token_type::Eof();
    std::istream& in;
    bool emptyLine = true;
    int newOffset, offsetSpace = 0;

    // чтение токена из потока
    Token ReadToken(std::istream& input);
    
    // чтение двойных символов (сравнение)
    Token ReadMultiplyChar(char c, std::istream& input);
    
    // чтение одиночных символов (сравнение)
    Token ReadSingleChar(char c);
    
    // чтение чисел
    Token ReadNumber(char first, std::istream& input);
    
    //проверка на цифровой символ
    bool IsDigit(char c);
    
    //проверка на буквеннный символ
    bool IsAplhabetic(char c);
    
    //проверка на буквеннно-цифровой символ
    bool IsAplhabeticOrDigit(char c);
    
    // чтение строк 
    Token ReadString(char first, std::istream& input);
    
    // чтение начального отступа
    int ReadOffset(std::istream& input);
    
    // чтение символьного токена (идентификатор или служебное слово)
    Token ReadSymbolToken(char first, std::istream& input);
    
    // пропуск комментария
    void SkipComment(std::istream& input);
    
};

}  // namespace parse