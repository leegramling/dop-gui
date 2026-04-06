#pragma once

#include <string>

/**
 * @brief Default ImGui theme helpers.
 */
class Theme
{
public:
    /**
     * @brief Apply the application's default ImGui style preset.
     * @param mode Theme mode name such as `"dark"` or `"light"`.
     */
    static void applyDefault(const std::string& mode = "dark");
};
