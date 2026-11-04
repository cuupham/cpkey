@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

rem Removes CPKey from this PC: stops it, drops autostart, deletes this folder's files.
rem Only entries that point at the CPKey.exe next to this script are touched, so another copy keeps its own autostart.
set "APP=CPKey"
set "DIR=%~dp0"
set "EXE=%DIR%%APP%.exe"
set "RUN_KEY=HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
set "APPROVED_KEY=HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run"

if not exist "%EXE%" (
    echo Không thấy %APP%.exe cạnh file này, nên không gỡ gì cả.
    pause
    exit /b 1
)

choice /c YN /n /m "Gỡ %APP% khỏi máy này? [Y] Có  [N] Không: "
if errorlevel 2 exit /b 0

powershell -NoProfile -Command "Get-Process %APP% -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $env:EXE } | Stop-Process -Force"
ping -n 2 127.0.0.1 >nul

rem Autostart in normal mode: a Run value holding the quoted path of this exe.
set "RUN_VALUE="
for /f "tokens=2,*" %%a in ('reg query "%RUN_KEY%" /v %APP% 2^>nul') do set "RUN_VALUE=%%b"
set "RUN_VALUE=!RUN_VALUE:"=!"
if /i "!RUN_VALUE!"=="%EXE%" (
    reg delete "%RUN_KEY%" /v %APP% /f >nul
    reg delete "%APPROVED_KEY%" /v %APP% /f >nul 2>&1
    echo Đã tắt khởi động cùng Windows.
)

rem Autostart in Admin mode: a scheduled task whose action is this exe. Deleting it needs an elevated prompt.
schtasks /query /tn %APP% /v /fo list 2>nul | findstr /i /l /c:"%EXE%" >nul
if not errorlevel 1 (
    schtasks /delete /tn %APP% /f >nul 2>&1
    if errorlevel 1 (
        echo Không xóa được tác vụ %APP% của chế độ Admin. Hãy chạy lại file này bằng Run as administrator.
    ) else (
        echo Đã xóa tác vụ %APP% của chế độ Admin.
    )
)

del /f /q "%EXE%" 2>nul
if exist "%EXE%" (
    echo Không xóa được %APP%.exe, có thể %APP% đang chạy quyền Admin. Hãy thoát %APP% hoặc chạy lại file này bằng Run as administrator.
    pause
    exit /b 1
)
del /f /q "%DIR%%APP%.ini" "%DIR%LICENSE" "%DIR%NOTICE.txt" 2>nul

echo Đã gỡ %APP%.
pause

rem This script cannot delete itself while running, so a detached shell does it, then removes the folder if it is empty.
cd /d "%TEMP%"
start "" /b cmd /s /c "ping -n 3 127.0.0.1 >nul & del /f /q "%~f0" & rmdir "%DIR%" 2>nul"
exit /b 0
