@echo off
rem ============================================================
rem GameGuru MAX - unit test gate
rem Builds and runs the test layers:
rem   1. GameGuruUnitTests    - product Lua script tests (doctest + Lua 5.2)
rem                             including Lua performance + memory suites
rem   2. WickedContractTests  - cross-repo integration contract tests
rem                             (compiles the real wickedcalls.cpp against
rem                              the sibling WickedRepo headers)
rem Both projects run in Release (speed budgets) AND Debug (CRT heap leak
rem detection for the engine suite). A non-zero exit code on any failure
rem aborts the deploy pipeline before publishing.
rem Usage: run_unit_tests.bat [Debug^|Release]   (default: Release)
rem        run_unit_tests.bat Debug   (also enables the leak suites)
rem ============================================================
setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

set "THIS_DIR=%~dp0"
set "MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD_PATH%" set "MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD_PATH%" (
    echo [run_unit_tests] MSBuild.exe not found - cannot run the test gate.
    exit /b 1
)

echo [run_unit_tests] Building GameGuruUnitTests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\GameGuruUnitTests\GameGuruUnitTests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 (
    echo [run_unit_tests] FAILED: GameGuruUnitTests did not build.
    exit /b 1
)

echo [run_unit_tests] Running product unit tests...
"%THIS_DIR%Tests\GameGuruUnitTests\bin\x64\%CONFIG%\GameGuruUnitTests.exe"
if errorlevel 1 (
    echo [run_unit_tests] FAILED: product unit tests reported failures.
    exit /b 1
)

echo [run_unit_tests] Building WickedContractTests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\WickedContractTests\WickedContractTests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 (
    echo [run_unit_tests] FAILED: WickedContractTests did not build.
    exit /b 1
)

echo [run_unit_tests] Running Wicked integration contract tests...
"%THIS_DIR%Tests\WickedContractTests\bin\x64\%CONFIG%\WickedContractTests.exe"
if errorlevel 1 (
    echo [run_unit_tests] FAILED: Wicked integration contract tests reported failures.
    exit /b 1
)

rem Engine suite with CRT leak detection. The engine project lives in the
rem sibling WickedRepo and uses its own solution; run only its Debug leak
rem layer when the sibling repository is present.
set "ENGINE_SLN=%THIS_DIR%..\..\WickedRepo\WickedEngine.sln"
if "%CONFIG%"=="Debug" if exist "%ENGINE_SLN%" goto ENGINE_LEAKS
goto GATE_DONE

:ENGINE_LEAKS
echo [run_unit_tests] Building engine UnitTests leak layer (Debug^|x64)...
"%MSBUILD_PATH%" "%ENGINE_SLN%" /t:UnitTests /p:Configuration=Debug /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 (
    echo [run_unit_tests] FAILED: engine UnitTests did not build.
    exit /b 1
)
echo [run_unit_tests] Running engine leak + performance suites...
"%THIS_DIR%..\..\WickedRepo\BUILD\x64\Debug\UnitTests\UnitTests.exe" -tc="*Leak*"
if errorlevel 1 (
    echo [run_unit_tests] FAILED: engine leak tests reported failures.
    exit /b 1
)

:GATE_DONE
echo [run_unit_tests] All unit tests passed.
endlocal
exit /b 0
