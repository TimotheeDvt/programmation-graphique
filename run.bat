@echo off
REM Delete old shader files if they exist
del basic.frag 2>nul
del basic.vert 2>nul

REM Copy new shader files from shaders directory
copy shaders\basic.frag basic.frag
copy shaders\basic.vert basic.vert

REM Build the project
cmake --build build --config Release
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
)

REM Run the executable if build succeeded
.\build\Release\main.exe

REM Delete shader files after execution
del basic.frag 2>nul
del basic.vert 2>nul