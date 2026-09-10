@echo off
setlocal

echo ============================================
echo Generating Visual Studio 2022 project files...
echo ============================================

cd /d "%~dp0"

rem PowerShell can pass PATH and Path as distinct environment keys. MSBuild's
rem process launcher treats those keys as duplicates, so normalize the key
rem before CMake performs compiler detection or starts a build.
set "NormalizedMiliastraPath=%PATH%"
set "Path="
set "PATH=%NormalizedMiliastraPath%"
set "NormalizedMiliastraPath="

echo.
echo Cleaning previous generated project files...

rem The solution is regenerated in the build tree and then moved to the
rem repository root. Remove Visual Studio's cached per-solution launch state
rem so a previous ALL_BUILD selection cannot survive regeneration.
if exist ".vs\Miliastra++\v17\.suo" (
del /q ".vs\Miliastra++\v17\.suo"
)

if exist "Miliastra++.sln" (
del /q "Miliastra++.sln"
)

if exist "Miliastra++\CMakeCache.txt" (
del /q "Miliastra++\CMakeCache.txt"
)

if exist "Miliastra++\CMakeFiles" (
rmdir /s /q "Miliastra++\CMakeFiles"
)

rem Remove CMake's generated build metadata and target intermediates only.
rem Source, Vendor, and Tests are intentionally outside this cleanup scope.
if exist "Miliastra++\Miliastra++.dir" (
rmdir /s /q "Miliastra++\Miliastra++.dir"
)

if exist "Miliastra++\Phase1Tests.dir" (
rmdir /s /q "Miliastra++\Phase1Tests.dir"
)

if exist "Miliastra++\Testing" (
rmdir /s /q "Miliastra++\Testing"
)

if exist "Miliastra++\cmake_install.cmake" del /q "Miliastra++\cmake_install.cmake"
if exist "Miliastra++\CTestTestfile.cmake" del /q "Miliastra++\CTestTestfile.cmake"

if exist "Miliastra++\Miliastra++.sln" del /q "Miliastra++\Miliastra++.sln"

if exist "Miliastra++\Miliastra++.vcxproj" (
del /q "Miliastra++\Miliastra++.vcxproj"
)

if exist "Miliastra++\Miliastra++.vcxproj.filters" (
del /q "Miliastra++\Miliastra++.vcxproj.filters"
)

if exist "Miliastra++\ALL_BUILD.vcxproj" (
del /q "Miliastra++\ALL_BUILD.vcxproj"
)

if exist "Miliastra++\ALL_BUILD.vcxproj.filters" (
del /q "Miliastra++\ALL_BUILD.vcxproj.filters"
)

if exist "Miliastra++\ZERO_CHECK.vcxproj" (
del /q "Miliastra++\ZERO_CHECK.vcxproj"
)

if exist "Miliastra++\ZERO_CHECK.vcxproj.filters" (
del /q "Miliastra++\ZERO_CHECK.vcxproj.filters"
)

if exist "Miliastra++\Phase1Tests.vcxproj" (
del /q "Miliastra++\Phase1Tests.vcxproj"
)

if exist "Miliastra++\Phase1Tests.vcxproj.filters" (
del /q "Miliastra++\Phase1Tests.vcxproj.filters"
)

if exist "Miliastra++\RUN_TESTS.vcxproj" (
del /q "Miliastra++\RUN_TESTS.vcxproj"
)

if exist "Miliastra++\RUN_TESTS.vcxproj.filters" (
del /q "Miliastra++\RUN_TESTS.vcxproj.filters"
)

if exist "Miliastra++\Miliastra++.vcxproj.user" (
del /q "Miliastra++\Miliastra++.vcxproj.user"
)

if exist "Miliastra++\ALL_BUILD.vcxproj.user" (
del /q "Miliastra++\ALL_BUILD.vcxproj.user"
)

if exist "Int\ProjectFiles" (
rmdir /s /q "Int\ProjectFiles"
)

rem A project-file regeneration is a full generated-state reset. Remove all
rem configuration outputs and intermediates so Bin and Int are recreated only
rem by a subsequent build.
if exist "Bin" (
rmdir /s /q "Bin"
)

if exist "Int" (
rmdir /s /q "Int"
)

if exist "x64" (
rmdir /s /q "x64"
)

echo.
echo Generating CMake project files...

set "ProjectFilesDirectory=Miliastra++"

cmake -S . -B "%ProjectFilesDirectory%" -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
echo.
echo ERROR: CMake generation failed!
pause
exit /b 1
)

if not exist "%ProjectFilesDirectory%\Miliastra++.sln" (
echo.
echo ERROR: Generated solution was not found!
echo Expected:
echo %CD%\%ProjectFilesDirectory%\Miliastra++.sln
pause
exit /b 1
)

echo.
echo Moving solution to solution root...

move /Y "%ProjectFilesDirectory%\Miliastra++.sln" "Miliastra++.sln" >nul

if %ERRORLEVEL% neq 0 (
echo.
echo ERROR: Failed to move solution to solution root!
pause
exit /b 1
)

echo.
echo Fixing project paths inside solution...

powershell -NoProfile -ExecutionPolicy Bypass -Command "$path = 'Miliastra++.sln'; $s = Get-Content -LiteralPath $path -Raw; $q = [char]34; Get-ChildItem -LiteralPath 'Miliastra++' -Filter '*.vcxproj' | ForEach-Object { $file = $_.Name; $s = $s.Replace(', ' + $q + $file + $q, ', ' + $q + 'Miliastra++\' + $file + $q) }; [System.IO.File]::WriteAllText($path, $s, [System.Text.UTF8Encoding]::new($false))"

if %ERRORLEVEL% neq 0 (
echo.
echo ERROR: Failed to fix solution project paths!
pause
exit /b 1
)

echo.
echo ============================================
echo Project generation completed successfully.
echo ============================================
echo.
echo Solution:
echo %CD%\Miliastra++.sln
echo.
echo Generated project files:
echo %CD%\Miliastra++
echo.
echo Build outputs:
echo %CD%\Bin
echo.
echo Build intermediates:
echo %CD%\Int
echo.

if exist "x64" (
echo WARNING: Root-level x64 directory still exists!
) else (
echo Root-level x64 directory: NOT PRESENT
)

echo.

pause
endlocal
