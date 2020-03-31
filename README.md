<center>
  <h1 align="center">BlockTheSpot</h1>
  <h4 align="center">A multi-purpose adblocker and skip bypass for the <strong>Windows</strong> Spotify Desktop Application.</h4>
  <h5 align="center">Please support Spotify by purchasing premium</h5>
  <p align="center">
    <strong>Last updated:</strong> 8 March 2020<br>
    <strong>Last tested version:</strong> 1.1.27.472.gf6574b97
  </p>
  <h4 align="center">Important Notice(s)</h4>
  <p align="center">
    This .dll is virus free, false positive may appear. This can be reassured via the source code. <br>
    "chrome_elf.dll" gets replaced by Spotify Installer each time it updates, make sure to replace it again.
  </p>
</center>

### Features:
* Blocks all banner/video/audio ads within the app
* Retains friend, vertical video and radio functionality
* Unlocks the skip function for any track

:warning: This mod is for the [**Desktop Application**](https://www.spotify.com/download/windows/) of Spotify on Windows, **not the Microsoft Store version**.

#### Installation/Update:
* Just run BlockTheSpot.bat (https://github.com/rednek46)
or
1. Browse to Spotify installation folder `%APPDATA%/Spotify`
2. Backup existing `chrome_elf.dll` by renaming it (e.g. chrome_elf.bak)
3. Download chrome_elf.zip from [releases](https://github.com/mrpond/BlockTheSpot/releases)
4. Extract chrome_elf.dll, config.ini from chrome_elf.zip to that folder. 

#### Uninstall:
* Just run uninstall.bat or reinstall spotify (https://github.com/rednek46)
or
1. Browse to Spotify installation folder `%APPDATA%/Spotify` 
2. Simply delete chrome_elf.dll, config.ini from your Spotify installation
3. Rename your backup dll to chrome_elf.dll

#### Note:
* Ads banner maybe appear if you network use 'Web Proxy Auto-Discovery Protocol'
https://en.wikipedia.org/wiki/Web_Proxy_Auto-Discovery_Protocol
set Skip_wpad in config.ini to 1 may help.
* if automatic install/uninstall bat not working please contact (https://github.com/rednek46)

#### Known Issues and Caveats:
* Windows Defender - false positive.

Please follow below instruction - this is official from windows defender team.

Analyst comments:

We have removed the detection. Please follow the steps below to clear cached detection and obtain the latest malware definitions.
1. Open command prompt as administrator and change directory to c:\Program Files\Windows Defender
2. Run "MpCmdRun.exe -removedefinitions -dynamicsignatures"
3. Run "MpCmdRun.exe -SignatureUpdate"

Alternatively, the latest definition is available for download here: https://www.microsoft.com/en-us/wdsi/definitions

Thank you for contacting Microsoft.