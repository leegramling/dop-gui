@echo off
setlocal

set "ROOT_DIR=%~dp0"
if "%BUILD_DIR%"=="" set "BUILD_DIR=%ROOT_DIR%build\dop-gui"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
if "%BUILD_JOBS%"=="" set "BUILD_JOBS=8"

echo Configuring dop-gui in "%BUILD_DIR%"...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 exit /b 1

echo Building dop-gui (%BUILD_TYPE%)...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %BUILD_JOBS%
if errorlevel 1 exit /b 1

echo Build complete.
endlocal
