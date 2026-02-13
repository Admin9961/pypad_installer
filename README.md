# pypad_installer
This repository remove UPX signatures from the PP main binary to address security failures, old install method refactored in C.
To compile the installer manually open WSL then run:

1. ```x86_64-w64-mingw32-windres version.rc -o version.o```
2. ```x86_64-w64-mingw32-gcc -o pypad_installer.exe installer.c version.o -lshlwapi -luuid -lole32 -lshell32 -luser32 -Os -s -ffunction-sections -fdata-sections -Wl,--gc-sections```

To compile the python file in the 'src' directory, cd to that folder and use pyinstaller:
1. ```pyinstaller --onefile --windowed pypad.py```

***Be aware***: if Pyinstaller tease you into packing the final PE with UPX, don't do that. The UPX Packer is very good to optimize and deliver compiled binaries but it has been abused by cybercriminals. Due to their actions, all PE packed with UPX get automatically marked as malicious by EDR or VirusTotal services (yes, even if they are innoquos)

Complete file hierarchy in **%AppData%** (this strategy will also let the program **avoid** admin privileges, since they are known to raise concerns)
# PyPad Installer -
## Installation Directory
`%LOCALAPPDATA%\pypad\`
- pypad.exe (main editor)
- All other files extracted from the embedded archive
## Temporary Directory (Deleted After Install)
`%LOCALAPPDATA%\pypad_setup\`
- pypad_x86-64.exe (SFX archive)
## Desktop Shortcut Folder
`%USERPROFILE%\Desktop\PYPAD_Shortcut\`
- PyPad.lnk              -> points to %LOCALAPPDATA%\pypad\pypad.exe
- LICENSE.txt            -> license terms
- Documentation.txt      -> user documentation
- uninstall_pypad.bat    -> uninstaller script
- logs.txt               -> installation log

