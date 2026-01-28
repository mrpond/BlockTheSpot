@echo off
REM BlockTheSpotKlim - Quick Launcher
REM Runs the PowerShell patcher with appropriate execution policy

echo ================================================
echo   BlockTheSpotKlim - Spotify Patcher
echo ================================================
echo.

REM Check for admin privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Not running as Administrator
    echo Some operations may require elevated privileges.
    echo.
    pause
)

REM Run PowerShell script
powershell.exe -ExecutionPolicy Bypass -NoProfile -File "%~dp0BlockTheSpot-Patcher.ps1"

echo.
pause
