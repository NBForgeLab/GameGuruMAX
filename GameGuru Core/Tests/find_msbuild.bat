@echo off
rem Resolve the installed C++ build tools for test gates without a user-specific path.
set "MSBUILD_PATH="
set "TEST_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%TEST_VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%TEST_VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
        if exist "%%I\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD_PATH=%%I\MSBuild\Current\Bin\MSBuild.exe"
    )
)
if defined MSBUILD_PATH exit /b 0
for %%I in (
    "%ProgramFiles%\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
) do if exist "%%~I" set "MSBUILD_PATH=%%~I"
if defined MSBUILD_PATH exit /b 0
echo [test gate] MSBuild with C++ tools was not found.
exit /b 1
