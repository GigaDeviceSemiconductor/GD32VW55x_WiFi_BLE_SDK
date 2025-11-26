@echo off

IF NOT "%1"=="" (
    SET APP=%1
    IF NOT "%2"=="" (
        SET USER_CMD=%2
    ) ELSE (
        SET USER_CMD="-j"
    )
) ELSE (
    SET APP=app
    SET USER_CMD="-j"
)


IF NOT EXIST "%CD%\MSDK\%APP%" (
    echo build app error: %CD%\MSDK\%APP% does not exist !!
    EXIT /B 1
)


IF NOT EXIST "%CD%\tools\gd32vw55x_toolchain_windows" (
    IF EXIST "%CD%\tools\gd32vw55x_toolchain_windows.7z.001" (
        echo Unzipping gd32vw55x toolchain .......
        "%PROGRAMFILES%\7-Zip\7z.exe" x "%CD%\tools\gd32vw55x_toolchain_windows.7z.001" -o"%CD%\tools"
    ) ELSE (
        echo toolchain error: %CD%\tools\gd32vw55x_toolchain_windows.7z.001 is not exist!
        EXIT /B 1
    )
)


IF NOT EXIST "%CD%\tools\xpack-openocd-0.11.0-3_windows" (
    IF EXIST "%CD%\tools\xpack-openocd-0.11.0-3_windows.7z" (
        echo Unzipping gd32vw55x OpenOCD .......
        "%PROGRAMFILES%\7-Zip\7z.exe" x "%CD%\tools\xpack-openocd-0.11.0-3_windows.7z" -o"%CD%\tools"
    ) ELSE (
        echo openocd error: %CD%\tools\xpack-openocd-0.11.0-3_windows.7z is not exist!
        EXIT /B 1
    )
)


SET PATH=%PATH%;%CD%\tools\gd32vw55x_toolchain_windows\bin



if NOT EXIST cmake_build (
    mkdir cmake_build
)
cd cmake_build

if "%USER_CMD%"=="clean" (
    DEL /S /Q *.* 2>NUL
    FOR /D %%D IN (*.*) DO (
        RD /S /Q "%%D" 2 > NUL
    )
) else (
    :: Configure
    cmake -G "Unix Makefiles" -DAPP=%app% -DCONFIG_BLE_FEATURE=MAX -DCONFIG_MBEDTLS_VERSION="3.6.2" -DCMAKE_TOOLCHAIN_FILE:PATH=./scripts/cmake/toolchain.cmake  ..

    :: Make
    make %USER_CMD%
)
cd ..
if ERRORLEVEL 2 pause

:end