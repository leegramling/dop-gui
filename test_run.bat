@echo off
setlocal EnableExtensions

for %%I in ("%~dp0.") do set "ROOT_DIR=%%~fI"

if "%BUILD_DIR%"=="" set "BUILD_DIR=%ROOT_DIR%\build\dop-gui"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=Release"
if "%DOP_GUI_EXE%"=="" set "DOP_GUI_EXE=%BUILD_DIR%\%BUILD_TYPE%\dop-gui.exe"
if "%BUILD_JOBS%"=="" set "BUILD_JOBS=8"

set "DEFAULT_SCRIPT=%ROOT_DIR%\tests\desktop_bootstrap.json5"
set "MODE=%~1"
if "%MODE%"=="" set "MODE=script"

if /I "%MODE%"=="help" goto :usage
if /I "%MODE%"=="-h" goto :usage
if /I "%MODE%"=="--help" goto :usage

if /I "%MODE%"=="rebuild" (
    shift
    set "MODE=%~1"
    if "%MODE%"=="" set "MODE=script"
    call :build_app
    if errorlevel 1 exit /b 1
) else (
    call :ensure_app
    if errorlevel 1 exit /b 1
)

if /I "%MODE%"=="script" goto :run_script
if /I "%MODE%"=="command" goto :run_command
if /I "%MODE%"=="live-bg" goto :live_bg
if /I "%MODE%"=="live-grid-off" goto :live_grid_off
if /I "%MODE%"=="live-scene-cubes" goto :live_scene_cubes
if /I "%MODE%"=="live-scene-create" goto :live_scene_create
if /I "%MODE%"=="live-regression" goto :live_regression

echo Unknown mode: %MODE%
goto :usage_error

:build_app
echo Configuring dop-gui in "%BUILD_DIR%"...
cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 exit /b 1

echo Building dop-gui (%BUILD_TYPE%)...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %BUILD_JOBS%
if errorlevel 1 exit /b 1

if not exist "%DOP_GUI_EXE%" (
    echo Expected executable not found: "%DOP_GUI_EXE%"
    exit /b 1
)
exit /b 0

:ensure_app
if not exist "%DOP_GUI_EXE%" (
    call :build_app
    exit /b %ERRORLEVEL%
)
exit /b 0

:run_script
set "SCRIPT_FILE=%~2"
if "%SCRIPT_FILE%"=="" set "SCRIPT_FILE=%DEFAULT_SCRIPT%"
shift
shift
"%DOP_GUI_EXE%" --script "%SCRIPT_FILE%" --stay-open %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:run_command
set "COMMAND_TEXT=%~2"
if "%COMMAND_TEXT%"=="" set "COMMAND_TEXT=state.reset.bootstrap"
shift
shift
"%DOP_GUI_EXE%" --command "%COMMAND_TEXT%" --stay-open %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:live_bg
shift
"%DOP_GUI_EXE%" --script "%ROOT_DIR%\tests\live_ui_bg_blue.json5" --stay-open --startup-delay-ms 5000 %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:live_grid_off
shift
"%DOP_GUI_EXE%" --script "%ROOT_DIR%\tests\live_ui_grid_off.json5" --stay-open --startup-delay-ms 5000 %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:live_scene_cubes
shift
"%DOP_GUI_EXE%" --script "%ROOT_DIR%\tests\live_ui_scene_cubes.json5" --stay-open --startup-delay-ms 5000 %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:live_scene_create
shift
"%DOP_GUI_EXE%" --script "%ROOT_DIR%\tests\live_ui_scene_create.json5" --stay-open %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:live_regression
shift
"%DOP_GUI_EXE%" --script "%ROOT_DIR%\tests\live_regression.json5" --stay-open %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b %ERRORLEVEL%

:usage
echo Usage:
echo   test_run.bat script [script_file] [extra dop-gui args...]
echo   test_run.bat command [command_text] [extra dop-gui args...]
echo   test_run.bat rebuild ^<mode^> [mode args...]
echo   test_run.bat live-bg [extra dop-gui args...]
echo   test_run.bat live-grid-off [extra dop-gui args...]
echo   test_run.bat live-scene-cubes [extra dop-gui args...]
echo   test_run.bat live-scene-create [extra dop-gui args...]
echo   test_run.bat live-regression [extra dop-gui args...]
echo.
echo Environment overrides:
echo   BUILD_DIR
echo   BUILD_TYPE
echo   BUILD_JOBS
echo   DOP_GUI_EXE
echo.
echo Examples:
echo   test_run.bat
echo   test_run.bat script tests\desktop_bootstrap.json5
echo   test_run.bat script tests\smoke_cli.json5
echo   test_run.bat command state.reset.bootstrap
echo   test_run.bat rebuild live-regression
echo   test_run.bat live-scene-create
echo   test_run.bat live-regression
exit /b 0

:usage_error
call :usage
exit /b 1
