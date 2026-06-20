#include "JsonTestSupport.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace dopgui::test
{
const Json& Json::at(const std::string& key) const
{
    return std::get<Object>(value).at(key);
}

const Json& Json::at(std::size_t index) const
{
    return std::get<Array>(value).at(index);
}

bool Json::boolean() const
{
    return std::get<bool>(value);
}

double Json::number() const
{
    return std::get<double>(value);
}

const std::string& Json::string() const
{
    return std::get<std::string>(value);
}

namespace
{
class Parser
{
public:
    explicit Parser(std::string text) : _text(std::move(text)) {}

    Json parse()
    {
        auto result = parseValue();
        skipWhitespace();
        if (_position != _text.size()) fail("unexpected trailing input");
        return result;
    }

private:
    Json parseValue()
    {
        skipWhitespace();
        if (_position >= _text.size()) fail("expected a value");

        switch (_text[_position])
        {
        case '{': return Json{parseObject()};
        case '[': return Json{parseArray()};
        case '"': return Json{parseString()};
        case 't': consumeLiteral("true"); return Json{true};
        case 'f': consumeLiteral("false"); return Json{false};
        case 'n': consumeLiteral("null"); return Json{nullptr};
        default: return Json{parseNumber()};
        }
    }

    Json::Object parseObject()
    {
        expect('{');
        Json::Object object;
        skipWhitespace();
        if (consume('}')) return object;

        while (true)
        {
            skipWhitespace();
            if (_position >= _text.size() || _text[_position] != '"') fail("expected an object key");
            auto key = parseString();
            skipWhitespace();
            expect(':');
            object.emplace(std::move(key), parseValue());
            skipWhitespace();
            if (consume('}')) return object;
            expect(',');
        }
    }

    Json::Array parseArray()
    {
        expect('[');
        Json::Array array;
        skipWhitespace();
        if (consume(']')) return array;

        while (true)
        {
            array.push_back(parseValue());
            skipWhitespace();
            if (consume(']')) return array;
            expect(',');
        }
    }

    std::string parseString()
    {
        expect('"');
        std::string result;
        while (_position < _text.size())
        {
            const char character = _text[_position++];
            if (character == '"') return result;
            if (character != '\\')
            {
                result.push_back(character);
                continue;
            }

            if (_position >= _text.size()) fail("unterminated string escape");
            switch (_text[_position++])
            {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            default: fail("unsupported string escape");
            }
        }
        fail("unterminated string");
    }

    double parseNumber()
    {
        const auto start = _position;
        consume('-');
        while (isDigit()) ++_position;
        if (consume('.')) while (isDigit()) ++_position;
        if (_position < _text.size() && (_text[_position] == 'e' || _text[_position] == 'E'))
        {
            ++_position;
            if (_position < _text.size() && (_text[_position] == '+' || _text[_position] == '-')) ++_position;
            while (isDigit()) ++_position;
        }
        if (start == _position) fail("expected a number");
        return std::stod(_text.substr(start, _position - start));
    }

    bool isDigit() const
    {
        return _position < _text.size() && _text[_position] >= '0' && _text[_position] <= '9';
    }

    void consumeLiteral(const std::string& literal)
    {
        if (_text.compare(_position, literal.size(), literal) != 0) fail("invalid literal");
        _position += literal.size();
    }

    void skipWhitespace()
    {
        while (_position < _text.size() &&
               (_text[_position] == ' ' || _text[_position] == '\n' ||
                _text[_position] == '\r' || _text[_position] == '\t'))
        {
            ++_position;
        }
    }

    bool consume(char expected)
    {
        if (_position < _text.size() && _text[_position] == expected)
        {
            ++_position;
            return true;
        }
        return false;
    }

    void expect(char expected)
    {
        if (!consume(expected)) fail(std::string("expected '") + expected + "'");
    }

    [[noreturn]] void fail(const std::string& message) const
    {
        throw std::runtime_error("JSON parse error at byte " + std::to_string(_position) + ": " + message);
    }

    std::string _text;
    std::size_t _position = 0;
};

std::string quote(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}
}

Json parseJson(std::string text)
{
    return Parser(std::move(text)).parse();
}

Json runDopGuiQuery(const std::filesystem::path& executable, const std::string& query)
{
    const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto outputPath = std::filesystem::temp_directory_path() /
        ("dop-gui-query-" + std::to_string(uniqueId) + ".json");
    const std::string command =
        quote(executable) + " --query " + query + " > " + quote(outputPath);

    const int status = std::system(command.c_str());
    if (status != 0)
    {
        std::filesystem::remove(outputPath);
        throw std::runtime_error("dop-gui query process failed: " + query);
    }

    std::ifstream input(outputPath);
    if (!input)
    {
        std::filesystem::remove(outputPath);
        throw std::runtime_error("failed to open dop-gui query output");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    input.close();
    std::filesystem::remove(outputPath);
    return parseJson(buffer.str());
}
}
