@echo off
echo Building LANDrop portable application...

REM Clean previous build
if exist build-release rmdir /s /q build-release
if exist dist rmdir /s /q dist

REM Create build directory
mkdir build-release
cd build-release

REM Configure with CMake (adjust Qt path as needed)
cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\mingw_64 -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" ..

REM Build the application
mingw32-make -j%NUMBER_OF_PROCESSORS%

REM Check if build succeeded
if not exist landropPlus.exe (
    echo Build failed!
    pause
    exit /b 1
)

REM Create dist directory for portable package
cd ..
mkdir dist

REM Copy executable
copy build-release\landropPlus.exe dist\

REM Deploy Qt dependencies for portable distribution
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw dist\landropPlus.exe

echo.
echo Build completed successfully!
echo Portable application created in 'dist' folder
echo.
pause