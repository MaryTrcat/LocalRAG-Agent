@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0run-localrag.ps1" %*
if errorlevel 1 (
    echo.
    echo LocalRAG 启动失败，请查看上面的错误信息。
    pause
)
