@echo off
setlocal

git --version >nul 2>&1
if errorlevel 1 (
    echo "Git is not installed. Please install Git and try again."
    exit /b 1
)

set "vcpkgPath=C:\vcpkg"

if not exist "%vcpkgPath%" (
    echo "Cloning vcpkg repository from GitHub..."
    git clone https://github.com/microsoft/vcpkg.git "%vcpkgPath%"
)

cd "%vcpkgPath%"
echo "Setting up vcpkg..."
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install assimp

pause
endlocal