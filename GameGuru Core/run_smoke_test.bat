@echo off
rem ============================================================
rem GameGuru MAX - Automated Engine Smoke Test Gate
rem Launches the real GameGuruMAX.exe, verifies clean initialization,
rem loads the target project, confirms absence of crash logs,
rem and terminates gracefully.
rem Usage: run_smoke_test.bat [ProjectName] [TimeoutSeconds]
rem Examples:
rem   run_smoke_test.bat
rem   run_smoke_test.bat 3r4r4r4r4r 20
rem ============================================================
setlocal
set "PROJECT=%~1"
set "TIMEOUT=%~2"
if "%TIMEOUT%"=="" set TIMEOUT=15

set "THIS_DIR=%~dp0"
powershell -ExecutionPolicy Bypass -File "%THIS_DIR%Tests\test_engine_headless.ps1" -ProjectName "%PROJECT%" -TimeoutSeconds %TIMEOUT%
exit /b %ERRORLEVEL%
