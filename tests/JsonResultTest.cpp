#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace
{
struct Json
{
    using Object = std::map<std::string, Json>;
    using Array = std::vector<Json>;
    using Value = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Value value;

    const Json& at(const std::string& key) const { return std::get<Object>(value).at(key); }
    const Json& at(std::size_t index) const { return std::get<Array>(value).at(index); }
    bool boolean() const { return std::get<bool>(value); }
    double number() const { return std::get<double>(value); }
    const std::string& string() const { return std::get<std::string>(value); }
};

class JsonParser
{
public:
    explicit JsonParser(std::string text) : _text(std::move(text)) {}

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
        if (consume('-')) {}
        while (_position < _text.size() && _text[_position] >= '0' && _text[_position] <= '9') ++_position;
        if (consume('.'))
        {
            while (_position < _text.size() && _text[_position] >= '0' && _text[_position] <= '9') ++_position;
        }
        if (_position < _text.size() && (_text[_position] == 'e' || _text[_position] == 'E'))
        {
            ++_position;
            if (_position < _text.size() && (_text[_position] == '+' || _text[_position] == '-')) ++_position;
            while (_position < _text.size() && _text[_position] >= '0' && _text[_position] <= '9') ++_position;
        }
        if (start == _position) fail("expected a number");
        return std::stod(_text.substr(start, _position - start));
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

void expect(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error("expectation failed: " + message);
}

void expectNear(double actual, double expected, double tolerance, const std::string& field)
{
    if (std::abs(actual - expected) > tolerance)
    {
        throw std::runtime_error(
            "expectation failed for " + field + ": expected " + std::to_string(expected) +
            ", actual " + std::to_string(actual));
    }
}

std::string quote(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: dop-gui-json-result-test <dop-gui executable>\n";
        return 2;
    }

    const auto outputPath = std::filesystem::current_path() / "json-result-test-output.json";
    const std::string command =
        quote(argv[1]) + " --query data.scene.object.bootstrap_cube.transform > " + quote(outputPath);

    try
    {
        const int status = std::system(command.c_str());
        expect(status == 0, "dop-gui query process should succeed");

        std::ifstream input(outputPath);
        expect(input.good(), "query output file should open");
        std::ostringstream buffer;
        buffer << input.rdbuf();

        const Json root = JsonParser(buffer.str()).parse();
        expect(root.at("ok").boolean(), "root.ok should be true");
        expect(
            root.at("query").string() == "data.scene.object.bootstrap_cube.transform",
            "root.query should identify the requested query");

        const Json& value = root.at("value");
        expect(value.at("id").string() == "bootstrap_cube", "value.id should be bootstrap_cube");
        expectNear(value.at("position").at(0).number(), -1.75, 0.000001, "position[0]");
        expectNear(value.at("position").at(1).number(), 0.0, 0.000001, "position[1]");
        expectNear(value.at("position").at(2).number(), 0.25, 0.000001, "position[2]");
        expectNear(value.at("scale").at(0).number(), 0.6, 0.000001, "scale[0]");

        std::filesystem::remove(outputPath);
        std::cout << "JSON query result assertions passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::filesystem::remove(outputPath);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
