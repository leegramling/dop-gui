#include "App.h"

/**
 * @brief Program entry point.
 * @param argc Argument count from the process invocation.
 * @param argv Argument vector from the process invocation.
 * @return Process-style exit code returned by the application.
 */
int main(int argc, char** argv)
{
    App app{argc, argv};
    return app.run();
}
