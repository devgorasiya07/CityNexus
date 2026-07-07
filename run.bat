@echo off
echo =========================================
echo       Compiling CityNexus Project...
echo =========================================
cmake --build build

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build Failed! Please check the errors above.
    exit /b %errorlevel%
)

echo.
echo [SUCCESS] Build completed! Starting application...
echo =========================================
echo.

rundll32.exe .\build\bin\citynexus.dll,RunApp
