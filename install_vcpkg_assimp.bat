@echo off
SET VCPKG_PATH=C:\vcpkg

if exist "%VCPKG_PATH%" (
    echo vcpkg is already installed at %VCPKG_PATH%.
) else (
    git clone https://github.com/Microsoft/vcpkg.git "%VCPKG_PATH%"
    cd "%VCPKG_PATH%"
    call bootstrap-vcpkg.bat
	call vcpkg.exe integrate install
    setx PATH "%PATH%;%VCPKG_PATH%"
)

call vcpkg.exe install assimp
