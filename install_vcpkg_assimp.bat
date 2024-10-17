@echo off
SET VCPKG_PATH=C:\vcpkg

REM Check if vcpkg is already installed
if exist "%VCPKG_PATH%" (
    echo vcpkg is already installed at %VCPKG_PATH%.
    exit /b
)

REM Prompt the user for installation
set /p response="Do you want to install vcpkg to %VCPKG_PATH%? (Y/N): "

if /I "%response%"=="Y" (
    REM Clone vcpkg repository
    git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_PATH%"

    REM Change directory to vcpkg
    cd "%VCPKG_PATH%"

    REM Bootstrap vcpkg
    call bootstrap-vcpkg.bat

    REM Install assimp using vcpkg
    call vcpkg.exe install assimp

    REM Add vcpkg to the system PATH
    setx PATH "%PATH%;%VCPKG_PATH%"

    echo vcpkg and assimp have been installed and added to your PATH. Please restart your terminal.
) else (
    echo Installation cancelled.
)