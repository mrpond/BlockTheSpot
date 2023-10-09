@echo off
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/thororen1234/BlockTheSpot/master/runners/nointeractioninstall.ps1' | Invoke-Expression}"
exit /b
