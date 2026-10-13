@echo off
rem ============================================================
rem GameGuru MAX - Dedicated E2E & System Test Suite Runner
rem Covers Tiers 1-4: Feature Coverage, Boundary Cases,
rem Cross-Feature Combinations, and Real-World Scenarios.
rem
rem Usage: run_e2e_tests.bat [Release|Debug]   (default: Release)
rem ============================================================
setlocal
set "CONFIG=%~1"
if "%CONFIG%"=="" set CONFIG=Release
if /i not "%CONFIG%"=="Release" if /i not "%CONFIG%"=="Debug" (
    echo [test gate] Configuration must be Release or Debug.
    exit /b 1
)

set "THIS_DIR=%~dp0"
call "%THIS_DIR%Tests\find_msbuild.bat"
if errorlevel 1 exit /b 1

echo [run_e2e_tests] Building GameGuruE2ETests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\GameGuruE2ETests\GameGuruE2ETests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 (
    echo [run_e2e_tests] FAILED: GameGuruE2ETests compilation failed.
    exit /b 1
)

echo [run_e2e_tests] Running GameGuru MAX E2E and system test suite (Tiers 1-4)...
"%THIS_DIR%Tests\GameGuruE2ETests\bin\x64\%CONFIG%\GameGuruE2ETests.exe" < nul
if errorlevel 1 (
    echo [run_e2e_tests] FAILED: GameGuruE2ETests reported failures.
    exit /b 1
)

echo [run_e2e_tests] SUCCESS: All E2E test suites passed.
endlocal
exit /b 0
