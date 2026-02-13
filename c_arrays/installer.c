#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "sfx_array.c"
#include "license_array.c"
#include "array_documentation.c"

static const unsigned char uninstall_bat[] = 
    "@echo off\r\n"
    "echo Removing PyPad...\r\n"
    "\r\n"
    "set INSTALL_DIR=%LOCALAPPDATA%\\pypad\r\n"
    "set SHORTCUT_DIR=%USERPROFILE%\\Desktop\\PYPAD_Shortcut\r\n"
    "\r\n"
    "if exist \"%INSTALL_DIR%\" (\r\n"
    "    rmdir /s /q \"%INSTALL_DIR%\"\r\n"
    "    echo Removed installation directory.\r\n"
    ") else (\r\n"
    "    echo Installation directory not found.\r\n"
    ")\r\n"
    "\r\n"
    "if exist \"%SHORTCUT_DIR%\" (\r\n"
    "    cd /d \"%USERPROFILE%\\Desktop\"\r\n"
    "    rmdir /s /q \"%SHORTCUT_DIR%\"\r\n"
    "    echo Removed PyPad shortcut folder.\r\n"
    ") else (\r\n"
    "    echo Shortcut folder not found.\r\n"
    ")\r\n"
    "\r\n"
    "echo PyPad has been uninstalled.\r\n";
static const size_t uninstall_bat_len = sizeof(uninstall_bat) - 1;

char g_log_path[MAX_PATH];

void WriteLog(const char* format, ...) {
    FILE* log = fopen(g_log_path, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    fprintf(log, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}

int IsWindows10Or11(void) {
    typedef long NTSTATUS;
    typedef struct {
        ULONG dwOSVersionInfoSize;
        ULONG dwMajorVersion;
        ULONG dwMinorVersion;
        ULONG dwBuildNumber;
        ULONG dwPlatformId;
        WCHAR szCSDVersion[128];
    } OSVERSIONINFOW;

    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(OSVERSIONINFOW*);

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        WriteLog("ERROR: GetModuleHandleW for ntdll.dll failed");
        return 0;
    }

    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
    if (!RtlGetVersion) {
        WriteLog("ERROR: GetProcAddress for RtlGetVersion failed");
        return 0;
    }

    OSVERSIONINFOW vi = { sizeof(vi) };
    if (RtlGetVersion(&vi) != 0) {
        WriteLog("ERROR: RtlGetVersion call failed");
        return 0;
    }

    WriteLog("Detected Windows version: %lu.%lu (Build %lu)", vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
    return vi.dwMajorVersion >= 10;
}

void CreateShortcut(char* target_exe, char* working_dir, char* folder, char* name, char* desc) {
    char link_path[MAX_PATH];
    snprintf(link_path, MAX_PATH, "%s\\%s", folder, name);

    WriteLog("Creating shortcut: '%s' -> '%s'", link_path, target_exe);

    IShellLinkA* psl;
    CoInitialize(NULL);

    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  &IID_IShellLinkA, (void**)&psl);

    if (SUCCEEDED(hr)) {
        psl->lpVtbl->SetPath(psl, target_exe);
        psl->lpVtbl->SetDescription(psl, desc);
        psl->lpVtbl->SetWorkingDirectory(psl, working_dir);

        IPersistFile* ppf;
        hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void**)&ppf);

        if (SUCCEEDED(hr)) {
            WCHAR wpath[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, link_path, -1, wpath, MAX_PATH);
            hr = ppf->lpVtbl->Save(ppf, wpath, TRUE);
            if (SUCCEEDED(hr)) {
                WriteLog("Shortcut saved successfully");
            }
            else {
                WriteLog("ERROR: IPersistFile::Save failed with HRESULT 0x%lx", hr);
            }
            ppf->lpVtbl->Release(ppf);
        }
        else {
            WriteLog("ERROR: QueryInterface for IPersistFile failed with HRESULT 0x%lx", hr);
        }
        psl->lpVtbl->Release(psl);
    }
    else {
        WriteLog("ERROR: CoCreateInstance for IShellLink failed with HRESULT 0x%lx", hr);
    }
    CoUninitialize();
}

void WriteLicenseFile(const char* shortcut_folder) {
    char license_path[MAX_PATH];
    snprintf(license_path, MAX_PATH, "%s\\LICENSE.txt", shortcut_folder);

    WriteLog("Writing license file to: %s", license_path);

    FILE* f = fopen(license_path, "wb");
    if (f) {
        size_t written = fwrite(LICENSE_txt, 1, LICENSE_txt_len, f);
        fclose(f);
        if (written == LICENSE_txt_len) {
            WriteLog("License file written successfully (%zu bytes)", written);
        }
        else {
            WriteLog("ERROR: Only wrote %zu of %zu license bytes", written, LICENSE_txt_len);
        }
    }
    else {
        WriteLog("ERROR: Cannot create license file '%s'", license_path);
    }
}

void WriteDocumentationFile(const char* shortcut_folder) {
    char doc_path[MAX_PATH];
    snprintf(doc_path, MAX_PATH, "%s\\Documentation.txt", shortcut_folder);

    WriteLog("Writing documentation file to: %s", doc_path);

    FILE* f = fopen(doc_path, "wb");
    if (f) {
        size_t written = fwrite(Documentation_txt, 1, Documentation_txt_len, f);
        fclose(f);
        if (written == Documentation_txt_len) {
            WriteLog("Documentation file written successfully (%zu bytes)", written);
        }
        else {
            WriteLog("ERROR: Only wrote %zu of %zu documentation bytes", written, Documentation_txt_len);
        }
    }
    else {
        WriteLog("ERROR: Cannot create documentation file '%s'", doc_path);
    }
}

void WriteUninstallerBat(const char* shortcut_folder) {
    char uninstaller_path[MAX_PATH];
    snprintf(uninstaller_path, MAX_PATH, "%s\\uninstall_pypad.bat", shortcut_folder);

    WriteLog("Writing uninstaller batch to: %s", uninstaller_path);

    FILE* f = fopen(uninstaller_path, "wb");
    if (f) {
        size_t written = fwrite(uninstall_bat, 1, uninstall_bat_len, f);
        fclose(f);
        if (written == uninstall_bat_len) {
            WriteLog("Uninstaller batch written successfully (%zu bytes)", written);
        }
        else {
            WriteLog("ERROR: Only wrote %zu of %zu uninstaller bytes", written, uninstall_bat_len);
        }
    }
    else {
        WriteLog("ERROR: Cannot create uninstaller batch '%s'", uninstaller_path);
    }
}

int main(int argc, char* argv[]) {
    int silent = 0;

    char desktop[MAX_PATH];
    char shortcut_folder[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop);
    snprintf(shortcut_folder, MAX_PATH, "%s\\PYPAD_Shortcut", desktop);

    CreateDirectoryA(shortcut_folder, NULL);
    WriteLog("Shortcut folder: %s", shortcut_folder);

    snprintf(g_log_path, MAX_PATH, "%s\\logs.txt", shortcut_folder);

    FILE* log_init = fopen(g_log_path, "w");
    if (log_init) {
        fprintf(log_init, "=== PyPad Installer Log ===\n");
        fclose(log_init);
    }

    WriteLog("========== PyPad Installer Started ==========");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "/S") == 0) silent = 1;
    }
    WriteLog("Command line arguments: %d", argc);
    for (int i = 0; i < argc; i++) {
        WriteLog("  argv[%d]: %s", i, argv[i]);
    }
    WriteLog("Silent mode: %s", silent ? "YES" : "NO");

    if (!IsWindows10Or11()) {
        WriteLog("ERROR: Unsupported Windows version (need 10 or 11)");
        if (!silent) {
            MessageBoxA(NULL,
                       "System Error.\n\nThis program requires Windows 10 or Windows 11.",
                       "PyPad - Unsupported OS",
                       MB_OK | MB_ICONERROR);
        }
        return 1;
    }
    WriteLog("Windows version check passed.");

    char localappdata[MAX_PATH];
    char sfx_dir[MAX_PATH];
    char sfx_path[MAX_PATH];
    char install_dir[MAX_PATH];
    char cmd[MAX_PATH * 2];

    SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localappdata);
    SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop);

    snprintf(sfx_dir, MAX_PATH, "%s\\pypad_setup", localappdata);
    snprintf(sfx_path, MAX_PATH, "%s\\pypad_x86-64.exe", sfx_dir);
    snprintf(install_dir, MAX_PATH, "%s\\pypad", localappdata);

    WriteLog("LocalAppData: %s", localappdata);
    WriteLog("Desktop: %s", desktop);
    WriteLog("Temporary SFX directory: %s", sfx_dir);
    WriteLog("Temporary SFX path: %s", sfx_path);
    WriteLog("Installation directory: %s", install_dir);

    if (CreateDirectoryA(sfx_dir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        WriteLog("Created or verified SFX directory: %s", sfx_dir);
    }
    else {
        WriteLog("ERROR: Failed to create SFX directory '%s', error=%lu", sfx_dir, GetLastError());
    }

    if (CreateDirectoryA(install_dir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        WriteLog("Created or verified install directory: %s", install_dir);
    }
    else {
        WriteLog("ERROR: Failed to create install directory '%s', error=%lu", install_dir, GetLastError());
    }

    WriteLog("Writing embedded SFX to: %s (size %zu bytes)", sfx_path, pypad_x86_64_exe_len);
    FILE* f = fopen(sfx_path, "wb");
    if (!f) {
        WriteLog("ERROR: Cannot create SFX file '%s', error=%lu", sfx_path, GetLastError());
        return 1;
    }
    size_t written = fwrite(pypad_x86_64_exe, 1, pypad_x86_64_exe_len, f);
    fclose(f);
    if (written == pypad_x86_64_exe_len) {
        WriteLog("SFX file written successfully (%zu bytes)", written);
    }
    else {
        WriteLog("ERROR: Only wrote %zu of %zu SFX bytes", written, pypad_x86_64_exe_len);
        return 1;
    }

    snprintf(cmd, sizeof(cmd),
             "\"%s\" -p\"J87k&3qL@9z#P2mX$5rV*bNw^4cYf%%DgR\" -o\"%s\" -y",
             sfx_path, install_dir);

    WriteLog("Extracting SFX archive: \"%s\" -p\"[REDACTED]\" -o\"%s\" -y", sfx_path, install_dir);
    WriteLog("Working directory for extraction: %s", sfx_dir);

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = silent ? SW_HIDE : SW_SHOW;

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, sfx_dir, &si, &pi)) {
        WriteLog("ERROR: CreateProcessA failed, error=%lu", GetLastError());
        return 1;
    }
    WriteLog("Extraction process started (PID: %lu)", pi.dwProcessId);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    WriteLog("Extraction process finished with exit code %lu", exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    WriteLicenseFile(shortcut_folder);
    WriteDocumentationFile(shortcut_folder);

    if (DeleteFileA(sfx_path)) {
        WriteLog("Deleted temporary SFX: %s", sfx_path);
    }
    else {
        WriteLog("WARNING: Could not delete SFX file '%s', error=%lu", sfx_path, GetLastError());
    }

    if (RemoveDirectoryA(sfx_dir)) {
        WriteLog("Removed temporary SFX directory: %s", sfx_dir);
    }
    else {
        WriteLog("WARNING: Could not remove directory '%s', error=%lu", sfx_dir, GetLastError());
    }

    char target_path[MAX_PATH];
    snprintf(target_path, MAX_PATH, "%s\\pypad.exe", install_dir);
    WriteLog("Shortcut target executable: %s", target_path);

    CreateShortcut(target_path,
                   install_dir,
                   shortcut_folder,
                   "PyPad.lnk",
                   "PyPad Text Editor");

    WriteUninstallerBat(shortcut_folder);

    WriteLog("========== PyPad Installer Completed Successfully ==========");
    return 0;
}
