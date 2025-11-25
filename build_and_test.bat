@echo off
setlocal ENABLEDELAYEDEXPANSION

set "PROJECT_NAME=d-chat"
set "BUILD_DIR=build_test"
set "TOOLCHAIN_PATH=C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
set "GENERATOR=Ninja"

set "MINGW_PATH=C:\msys64\mingw64\bin"
set "CC=%MINGW_PATH%\gcc.exe"
set "CXX=%MINGW_PATH%\g++.exe"

set "BUILD_TYPE=Release"
set "EXTRA_CMAKE_ARGS=-DDEBUG=OFF -DCMAKE_BUILD_TYPE=Release"
set "TEST_LEVEL=all"
set "TEST_FILTER="
set "SKIP_BUILD=0"

:parse_args
if "%~1"=="" goto :done_parsing

set "ARG=%~1"
if "%ARG:~0,2%"=="--" set "ARG=%ARG:~2%"
if "%ARG:~0,1%"=="-"  set "ARG=%ARG:~1%"

if /I "%ARG%"=="debug" (
  set "BUILD_TYPE=Debug"
  set "EXTRA_CMAKE_ARGS=-DDEBUG=ON -DCMAKE_BUILD_TYPE=Debug"
  shift
  goto :parse_args
)

if /I "%ARG%"=="release" (
  set "BUILD_TYPE=Release"
  set "EXTRA_CMAKE_ARGS=-DDEBUG=OFF -DCMAKE_BUILD_TYPE=Release"
  shift
  goto :parse_args
)

if /I "%ARG%"=="unit" (
  set "TEST_LEVEL=unit"
  shift
  goto :parse_args
)

if /I "%ARG%"=="integration" (
  set "TEST_LEVEL=integration"
  shift
  goto :parse_args
)

if /I "%ARG%"=="e2e" (
  set "TEST_LEVEL=e2e"
  shift
  goto :parse_args
)

if /I "%ARG%"=="test" (
  set "TEST_FILTER=%~2"
  shift
  shift
  goto :parse_args
)

if /I "%ARG%"=="filter" (
  set "TEST_FILTER=%~2"
  shift
  shift
  goto :parse_args
)

if /I "%ARG%"=="skip-build" (
  set "SKIP_BUILD=1"
  shift
  goto :parse_args
)

if /I "%ARG%"=="help" goto :show_help
if /I "%ARG%"=="h" goto :show_help
if /I "%ARG%"=="-help" goto :show_help
if /I "%ARG%"=="?" goto :show_help

echo [WARN] Unknown argument "%~1". Use --help for usage information.
shift
goto :parse_args

:done_parsing

if "%SKIP_BUILD%"=="1" (
  echo [INFO] Skipping build as requested
  goto :run_tests
)

echo [INFO] Build type: %BUILD_TYPE%
echo [INFO] Extra CMake args: %EXTRA_CMAKE_ARGS%
echo [INFO] Building with tests enabled
echo.

if exist "%BUILD_DIR%" (
  echo [INFO] Using existing build directory "%BUILD_DIR%"...
) else (
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
  -DBUILD_TESTS=ON ^
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

:run_tests
echo.
echo [STEP] Running tests...
echo [INFO] Test level: %TEST_LEVEL%
if not "%TEST_FILTER%"=="" (
  echo [INFO] Test filter: %TEST_FILTER%
)
echo.

cd "%BUILD_DIR%"

set "TOTAL_FAILURES=0"

if /I "%TEST_LEVEL%"=="unit" goto :run_unit
if /I "%TEST_LEVEL%"=="integration" goto :run_integration
if /I "%TEST_LEVEL%"=="e2e" goto :run_e2e
if /I "%TEST_LEVEL%"=="all" goto :run_all

echo [ERROR] Unknown test level: %TEST_LEVEL%
cd ..
exit /b 1

:run_unit
call :execute_test_suite "Unit" "unit_tests"
goto :test_summary

:run_integration
call :execute_test_suite "Integration" "integration_tests"
goto :test_summary

:run_e2e
call :execute_test_suite "E2E" "e2e_tests"
goto :test_summary

:run_all
call :execute_test_suite "Unit" "unit_tests"
call :execute_test_suite "Integration" "integration_tests"
call :execute_test_suite "E2E" "e2e_tests"
goto :test_summary

:execute_test_suite
setlocal
set "SUITE_NAME=%~1"
set "EXECUTABLE=%~2"
set "EXEC_PATH=tests\%EXECUTABLE%.exe"

echo.
echo ========================================
echo Running %SUITE_NAME% Tests
echo ========================================

if not exist "%EXEC_PATH%" (
  echo [WARN] %SUITE_NAME% tests executable not found: %EXEC_PATH%
  endlocal
  exit /b 0
)

if not "%TEST_FILTER%"=="" (
  echo [INFO] Running with filter: %TEST_FILTER%
  "%EXEC_PATH%" --gtest_filter=%TEST_FILTER%
) else (
  "%EXEC_PATH%"
)

if errorlevel 1 (
  echo [ERROR] %SUITE_NAME% tests failed.
  set /a "TOTAL_FAILURES+=1"
)

endlocal & set "TOTAL_FAILURES=%TOTAL_FAILURES%"
exit /b 0

:test_summary
cd ..

echo.
echo ========================================
if "%TOTAL_FAILURES%"=="0" (
  echo [SUCCESS] All tests passed!
  echo ========================================
  exit /b 0
) else (
  echo [FAILURE] Some tests failed.
  echo ========================================
  exit /b 1
)

:show_help
echo.
echo Usage: build_and_test.bat [OPTIONS]
echo.
echo Build Configuration:
echo   debug              Build in Debug mode
echo   release            Build in Release mode (default)
echo   --skip-build       Skip build step and only run tests
echo.
echo Test Selection:
echo   unit               Run only unit tests
echo   integration        Run only integration tests
echo   e2e                Run only E2E tests
echo   (no option)        Run all tests (default)
echo.
echo Test Filtering:
echo   --test PATTERN     Run tests matching PATTERN
echo   --filter PATTERN   Same as --test
echo.
echo Other:
echo   --help, -h, /?     Show this help message
echo.
echo Examples:
echo   build_and_test.bat
echo       Build in Release mode and run all tests
echo.
echo   build_and_test.bat debug unit
echo       Build in Debug mode and run only unit tests
echo.
echo   build_and_test.bat unit --test CryptoTest.*
echo       Build and run all tests from CryptoTest suite
echo.
echo   build_and_test.bat --test BlockchainServiceTest.ValidateLocalChainWithSingleBlockSucceeds
echo       Build and run specific test
echo.
echo   build_and_test.bat integration --test *Service*
echo       Build and run integration tests matching *Service*
echo.
echo   build_and_test.bat --skip-build e2e
echo       Skip build and run only E2E tests (useful for quick re-runs)
echo.
echo   build_and_test.bat --test MessageServiceTest.*
echo       Run all tests from MessageServiceTest class
echo.
echo Google Test Filter Patterns:
echo   ClassName.*                  All tests in ClassName
echo   *PatternName*                All tests containing PatternName
echo   ClassName.TestName           Specific test
echo   *:-NegativePattern           All tests except those matching NegativePattern
echo   Pattern1:Pattern2            Multiple patterns (OR)
echo.
goto :end

:end
echo.
endlocal
exit /b 0