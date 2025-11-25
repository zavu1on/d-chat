@echo off
setlocal ENABLEDELAYEDEXPANSION

set "PROJECT_NAME=d-chat"
set "BUILD_DIR=build"
set "TOOLCHAIN_PATH=C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
set "GENERATOR=Ninja"

set "MINGW_PATH=C:\msys64\mingw64\bin"
set "CC=%MINGW_PATH%\gcc.exe"
set "CXX=%MINGW_PATH%\g++.exe"

if "%~1"=="" goto :use_default

set "ARG=%~1"
if "%ARG:~0,2%"=="--" set "ARG=%ARG:~2%"
if "%ARG:~0,1%"=="-"  set "ARG=%ARG:~1%"

if /I "%ARG%"=="debug"  goto :debug
if /I "%ARG%"=="release" goto :release

echo [WARN] Unknown argument "%~1". Using default (Release).
goto :use_default

:use_default
set "BUILD_TYPE=Release"
set "EXTRA_CMAKE_ARGS=-DDEBUG=OFF -DCMAKE_BUILD_TYPE=Release"
goto :configure_and_build

:debug
set "BUILD_TYPE=Debug"
set "EXTRA_CMAKE_ARGS=-DDEBUG=ON -DCMAKE_BUILD_TYPE=Debug"
goto :configure_and_build

:release
set "BUILD_TYPE=Release"
set "EXTRA_CMAKE_ARGS=-DDEBUG=OFF -DCMAKE_BUILD_TYPE=Release"
goto :configure_and_build

:configure_and_build
echo [INFO] Build type: %BUILD_TYPE%
echo [INFO] Extra CMake args: %EXTRA_CMAKE_ARGS%
echo.

if not exist "%BUILD_DIR%" (
  mkdir "%BUILD_DIR%"
  if errorlevel 1 (
    echo [ERROR] Cannot create build directory "%BUILD_DIR%".
    exit /b 1
  )
)

echo [STEP] Configuring with CMake (using MinGW)...
cmake -S . -B "%BUILD_DIR%" ^
  -G "%GENERATOR%" ^
  -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_PATH%" ^
  -DCMAKE_C_COMPILER="%CC%" ^
  -DCMAKE_CXX_COMPILER="%CXX%" ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static ^
  -DBUILD_TESTS=OFF ^
  %EXTRA_CMAKE_ARGS%
if errorlevel 1 (
  echo [ERROR] CMake configuration failed.
  exit /b 1
)


echo.
echo [STEP] Building...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

echo.
set "EXEC_PATH=%BUILD_DIR%\src\app\%PROJECT_NAME%_main.exe"
if exist "%EXEC_PATH%" (
  echo [STEP] Running "%EXEC_PATH%"
  echo.
  "%EXEC_PATH%"
  goto :end
)

echo [WARN] Executable not found. Looked for:
echo   %BUILD_DIR%\src\app\%PROJECT_NAME%_main.exe
echo   %BUILD_DIR%\src\app\%BUILD_TYPE%\%PROJECT_NAME%_main.exe
echo [INFO] Build succeeded but executable could not be located automatically.
goto :end

:end
echo.
echo [DONE]
endlocal
exit /b 0
