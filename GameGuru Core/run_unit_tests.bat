@echo off
rem ============================================================
rem GameGuru MAX - unit test gate
rem Builds and runs the production DBO decoder, object compatibility,
rem string/Lua API, gameplay script, and Wicked contract test layers.
rem The test projects run in Release (speed budgets) AND Debug (CRT heap leak
rem detection for the engine suite). A non-zero exit code on any failure
rem aborts the deploy pipeline before publishing.
rem Usage: run_unit_tests.bat [Debug^|Release]   (default: Release)
rem        run_unit_tests.bat Debug   (also enables the leak suites)
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

echo [run_unit_tests] Building DBOModelTests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\DBOModelTests\DBOModelTests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 exit /b 1
"%THIS_DIR%Tests\DBOModelTests\bin\%CONFIG%\DBOModelTests.exe"
if errorlevel 1 exit /b 1

echo [run_unit_tests] Building DBOCompatibilityTests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\DBOCompatibilityTests\DBOCompatibilityTests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 exit /b 1
"%THIS_DIR%Tests\DBOCompatibilityTests\bin\%CONFIG%\DBOCompatibilityTests.exe"
if errorlevel 1 exit /b 1

echo [run_unit_tests] Building GameGuruE2ETests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\GameGuruE2ETests\GameGuruE2ETests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 (
    echo [run_unit_tests] FAILED: GameGuruE2ETests did not build.
    exit /b 1
)

echo [run_unit_tests] Running GameGuru MAX E2E and system test suite (Tiers 1-4)...
"%THIS_DIR%Tests\GameGuruE2ETests\bin\x64\%CONFIG%\GameGuruE2ETests.exe" < nul
if errorlevel 1 (
    echo [run_unit_tests] FAILED: GameGuruE2ETests reported failures.
    exit /b 1
)

echo [run_unit_tests] Building GameGuruUnitTests (%CONFIG%^|x64)...
"%MSBUILD_PATH%" "%THIS_DIR%Tests\GameGuruUnitTests\GameGuruUnitTests.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /verbosity:minimal /nologo
if errorlevel 1 (
    echo [run_unit_tests] FAILED: GameGuruUnitTests did not build.
    exit /b 1
)

echo [run_unit_tests] Running product unit tests...
"%THIS_DIR%Tests\GameGuruUnitTests\bin\x64\%CONFIG%\GameGuruUnitTests.exe" < nul
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
