@echo off
REM Delete old shader files if they exist
del *.frag 2>nul
del *.vert 2>nul

REM Copy new shader files from shaders directory
copy shaders\minecraft.frag
copy shaders\minecraft.vert
copy shaders\crosshair.frag
copy shaders\crosshair.vert
copy shaders\debug_line.frag
copy shaders\debug_line.vert

REM Build the project
cmake --build build --config Release
IF %ERRORLEVEL% NEQ 0 (
    del *.frag 2>nul
    del *.vert 2>nul
    echo Build failed.
    exit /b %ERRORLEVEL%
)

REM Run the executable if build succeeded
.\build\Release\main.exe

REM Delete shader files after execution
del *.frag 2>nul
del *.vert 2>nul