@echo off
setlocal

REM ==== Config ====
set PROJECT_PATH=.\ps2batchrenamer.csproj
set CONFIGURATION=Release
set RUNTIME=win-x64
set FRAMEWORK=net8.0-windows
set PUBLISH_DIR=.\publish

echo.
echo Building and publishing standalone single-file EXE...
echo.

dotnet publish "%PROJECT_PATH%" ^
    --configuration %CONFIGURATION% ^
    --runtime %RUNTIME% ^
    --framework %FRAMEWORK% ^
    --self-contained true ^
    /p:PublishSingleFile=true ^
    /p:PublishTrimmed=false ^
    /p:IncludeAllContentForSelfExtract=true ^
    --output "%PUBLISH_DIR%"

if errorlevel 1 (
    echo.
    echo Build FAILED!
    pause
    exit /b 1
)

echo.
echo Build succeeded. Output folder: %PUBLISH_DIR%
pause
endlocal
