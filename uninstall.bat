@echo off
echo *****************
echo Author: @rednek46
echo *****************
echo Removing Patch...
if exist "%APPDATA%\Spotify\dpapi.dll" (
    del /s /q "%APPDATA%\Spotify\dpapi.dll" > NUL 2>&1
    if exist "%APPDATA%\Spotify\dpapi.dll" (
        echo Failed to remove patch. Please close Spotify and try again.
    ) else (
        echo Patch successfully removed.
    )
) else (
    echo No patch found. Nothing to remove.
)
pause