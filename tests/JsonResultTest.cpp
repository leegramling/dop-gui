#include "JsonTestSupport.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void expect(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error("expectation failed: " + message);
}

void expectNear(double actual, double expected, const std::string& field)
{
    if (std::abs(actual - expected) > 0.000001)
    {
        throw std::runtime_error("expectation failed for " + field);
    }
}
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;

    try
    {
        const auto root = dopgui::test::runDopGuiQuery(
            argv[1], "data.scene.object.bootstrap_cube.transform");
        const auto& value = root.at("value");

        expect(root.at("ok").boolean(), "root.ok should be true");
        expect(value.at("id").string() == "bootstrap_cube", "value.id should be bootstrap_cube");
        expectNear(value.at("position").at(0).number(), -1.75, "position[0]");
        expectNear(value.at("position").at(1).number(), 0.0, "position[1]");
        expectNear(value.at("position").at(2).number(), 0.25, "position[2]");
        expectNear(value.at("scale").at(0).number(), 0.6, "scale[0]");

        std::cout << "JSON query result assertions passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
