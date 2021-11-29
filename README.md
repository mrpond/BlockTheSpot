[![Build status](https://ci.appveyor.com/api/projects/status/31l6ynm0a1fhr2vs/branch/master?svg=true)](https://ci.appveyor.com/project/mrpond/blockthespot/branch/master)  [![Discord](https://discord.com/api/guilds/807273906872123412/widget.png)](https://discord.gg/p43cusgUPm)

<center>
    <h3 align="center"><a href="https://en.wikipedia.org/wiki/2020_Thai_protests">Support the 2020 Thailand & 2021 Myanmar protests</a></h3>
    <h3 align="center">Help me stop the dictatorship in Thailand and Myanmar!</h3>
    <h3 align="center">Southeast Asia strong together!</h3>
</center>

<center>
    <h1 align="center">BlockTheSpot</h1>
    <h4 align="center">A multi-purpose adblocker and skip-bypass for the <strong>Windows</strong> Spotify desktop application.</h4>
    <h5 align="center">Please support Spotify by purchasing premium</h5>
    <p align="center">
        <strong>Last updated:</strong> 29 October 2021<br>
        <strong>Last tested version:</strong> 1.1.73.517.gbef50fdb
    </p> 
</center>

#### Important checks before installing:
0. Update Windows, update Spotify and update BlockTheSpot
1. Go to "Windows Security" -> "Virus & Threat Protection"
2. Click "Allowed threats" -> "Remove all allowed threats"

### Features:
* Blocks all banner/video/audio ads within the app
* Retains friend, vertical video and radio functionality
* Unlocks the skip function for any track
* Now supports the new Alpha version (New UI)

:warning: This mod is for the [**Desktop Application**](https://www.spotify.com/download/windows/) of Spotify on Windows only and **not the Microsoft Store version**.

### Installation/Update:
* Just download and run [BlockTheSpot.bat](https://raw.githack.com/mrpond/BlockTheSpot/master/BlockTheSpot.bat)  

or

* Run The following command in PowerShell:
```ps1
Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1' | Invoke-Expression
```

or

1. Browse to your Spotify installation folder `%APPDATA%\Spotify`
2. Rename `chrome_elf.dll` to `chrome_elf_bak.dll`
2. Download `chrome_elf.zip` from [releases](https://github.com/mrpond/BlockTheSpot/releases)
3. Unzip `chrome_elf.dll` and `config.ini` 

### Uninstall:
* Just run uninstall.bat
or
* Reinstall Spotify 

### Known Issues:  
* You may face issue [#150](https://github.com/mrpond/BlockTheSpot/issues/150). Can be fixed by enabling the experimental feature when using `BlockTheSpot.bat`.    
* We support last 2 version of Spotify (latest + previous) only. Please check it before opening an issue.

### Additional Notes:  
* Remove "Upgrade" Button [#83](https://github.com/mrpond/BlockTheSpot/issues/83) and Remove "Ad Placeholder" [#150](https://github.com/mrpond/BlockTheSpot/issues/150) only works when you use any of the auto installation methods and press `y` when prompted.  
* "chrome_elf.dll" gets replaced by the Spotify installer each time it updates, hence why you'll probably need to apply the patch again when it happens
* [Spicetify](https://github.com/khanhas/spicetify-cli) users will need to reapply BlockTheSpot after applying a Spicetify patches.
* If the automatic install/uninstall scripts do not work, please contact [Nuzair46](https://github.com/Nuzair46)
* For more support and discussions, join our [Discord server](https://discord.gg/p43cusgUPm) 
