# Compile from scratch
This repository remove UPX signatures from the PP main binary to address security failures, old install method refactored in C.
To compile the installer manually open WSL then run:

1. ```x86_64-w64-mingw32-windres version.rc -o version.o```
2. ```x86_64-w64-mingw32-gcc -o pypad_installer.exe installer.c version.o -lshlwapi -luuid -lole32 -lshell32 -luser32 -Os -s -ffunction-sections -fdata-sections -Wl,--gc-sections```

To compile the python file in the 'src' directory, cd to that folder and use pyinstaller:
1. ```pyinstaller --onefile --windowed pypad.py```

***Be aware***: if Pyinstaller tease you into packing the final PE with UPX, don't do that. The UPX Packer is very good to optimize and deliver compiled binaries but it has been abused by cybercriminals. Due to their actions, all PE packed with UPX get automatically marked as malicious by EDR or VirusTotal services (yes, even if they are innoquos)


# PyPad Installer
## Installation Directory - Complete file hierarchy in **AppData/Local** (this strategy will also let the program **avoid** admin privileges, since they are known to raise concerns)
`%LOCALAPPDATA%\pypad\`
- pypad.exe (sha256:61b723a1e88feb727202f1e4ad811ac315137783f7cec7537d7583535c1006f9)
- All other files extracted from the embedded archive
## Temporary Directory (Deleted After Install)
`%LOCALAPPDATA%\pypad_setup\`
- pypad_x86-64.exe (sfx_array.c)
## Desktop Shortcut Folder
`%USERPROFILE%\Desktop\PYPAD_Shortcut\`
- PyPad.lnk              -> points to %LOCALAPPDATA%\pypad\pypad.exe
- LICENSE.txt            -> license terms (license_array.c)
- Documentation.txt      -> user documentation (array_documentation.c)
- uninstall_pypad.bat    -> uninstaller script (referenced in installer.c)
- logs.txt               -> installation log (track installation flow)

