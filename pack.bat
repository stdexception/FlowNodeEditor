@echo off
setlocal
cd /d "%~dp0"
if not exist build cmake -B build
cmake --build build --config Release
if errorlevel 1 exit /b 1
echo Built: build\Release\FlowNodeEditor.exe
endlocal
