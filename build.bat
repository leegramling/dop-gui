@echo off
setlocal

for %%I in ("%~dp0.") do set "ROOT_DIR=%%~fI"
if "%BUILD_DIR%"=="" set "BUILD_DIR=%ROOT_DIR%\build\dop-gui"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
if "%BUILD_JOBS%"=="" set "BUILD_JOBS=8"
if "%Vulkan_ROOT%"=="" if not "%VULKAN_SDK%"=="" set "Vulkan_ROOT=%VULKAN_SDK%"
if not "%GLSLANG_VALIDATOR_DIR%"=="" set "PATH=%GLSLANG_VALIDATOR_DIR%;%PATH%"
set "CMAKE_ARGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%"
if not "%VSG_INSTALL_DIR%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_PREFIX_PATH=%VSG_INSTALL_DIR%"
if not "%Vulkan_ROOT%"=="" set "CMAKE_ARGS=%CMAKE_ARGS% -DVulkan_ROOT=%Vulkan_ROOT%"

echo Configuring dop-gui in "%BUILD_DIR%"...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" %CMAKE_ARGS%
if errorlevel 1 exit /b 1

echo Building dop-gui (%BUILD_TYPE%)...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %BUILD_JOBS%
if errorlevel 1 exit /b 1

echo Build complete.
endlocal
