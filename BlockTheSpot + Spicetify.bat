@echo off
echo ========================================
echo  BlockTheSpot + Spicetify Installer
echo ========================================
echo.

set /p UserInput="Spicetify will be installed. If you don't agree, use the BlockTheSpot script. Do you want to continue with the installation? (y/n): "
if /i "%UserInput%"=="y" (
    echo.
    echo Installing BlockTheSpot with Spicetify...
    powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; $scriptContent = (Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1').Content; $scriptBlock = [ScriptBlock]::Create($scriptContent); & $scriptBlock -InstallSpicetify}") else (
    echo.
    echo Installation cancelled.
    pause
    exit /b
)
