```bat
@echo off
setlocal

echo ============================================
echo Generating Visual Studio 2022 project files...
echo ============================================

REM Change to project root
cd /d "%~dp0"

REM --------------------------------------------
REM Clean previous generated project files
REM --------------------------------------------

if exist "Miliastra++\CMakeCache.txt" del /q "Miliastra++\CMakeCache.txt"
if exist "Miliastra++\CMakeFiles" rmdir /s /q "Miliastra++\CMakeFiles"
if exist "Miliastra++\Miliastra++.sln" del /q "Miliastra++\Miliastra++.sln"
if exist "Miliastra++\Miliastra++.vcxproj" del /q "Miliastra++\Miliastra++.vcxproj"
if exist "Miliastra++\Miliastra++.vcxproj.filters" del /q "Miliastra++\Miliastra++.vcxproj.filters"
if exist "Miliastra++\ALL_BUILD.vcxproj" del /q "Miliastra++\ALL_BUILD.vcxproj"
if exist "Miliastra++\ALL_BUILD.vcxproj.filters" del /q "Miliastra++\ALL_BUILD.vcxproj.filters"
if exist "Miliastra++\ZERO_CHECK.vcxproj" del /q "Miliastra++\ZERO_CHECK.vcxproj"
if exist "Miliastra++\ZERO_CHECK.vcxproj.filters" del /q "Miliastra++\ZERO_CHECK.vcxproj.filters"
if exist "Miliastra++.sln" del /q "Miliastra++.sln"

REM --------------------------------------------
REM Generate Visual Studio project files
REM --------------------------------------------

cmake -S . -B "Miliastra++" -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake generation failed!
    pause
    exit /b 1
)

if not exist "Miliastra++\Miliastra++.sln" (
    echo.
    echo ERROR: Generated solution was not found!
    echo Expected:
    echo %CD%\Miliastra++\Miliastra++.sln
    pause
    exit /b 1
)

move /Y "Miliastra++\Miliastra++.sln" "Miliastra++.sln" >nul

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Failed to move solution to project root!
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "$path = 'Miliastra++.sln'; $s = Get-Content -LiteralPath $path -Raw; Get-ChildItem -LiteralPath 'Miliastra++' -Filter '*.vcxproj' | ForEach-Object { $name = $_.Name; $s = $s.Replace('"' + $name + '"', '"Miliastra++\' + $name + '"') }; [System.IO.File]::WriteAllText($path, $s, [System.Text.UTF8Encoding]::new($false))"

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Failed to fix solution project paths!
    pause
    exit /b 1
)

REM --------------------------------------------
REM Done
REM --------------------------------------------

pause
endlocal
```
