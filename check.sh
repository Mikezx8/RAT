#!/bin/bash

# === CONFIG ===
MAIN_DIR="$HOME/MOD"
PROJECTS=("$HOME/MOD/Monitor" "$HOME/MOD/Monitor2" "$HOME/MOD/Monitor3" "$HOME/MOD/websession")
SDK_PATH="$HOME/Android/Sdk"

# === COLORS ===
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RESET='\033[0m'
BOLD='\033[1m'

# === ICONS ===
OK="[✔]"
FAIL="[✖]"

echo -e "${BOLD}${BLUE}Environment Verification${RESET}"
echo "--------------------------------------"
echo ""

# === FUNCTION HELPERS ===
check_cmd() {
    if command -v "$1" &>/dev/null; then
        echo -e "${GREEN}${OK} $1 found → $(command -v $1)${RESET}"
    else
        echo -e "${RED}${FAIL} $1 missing${RESET}"
    fi
}

check_file() {
    if [ -f "$1" ]; then
        echo -e "${GREEN}${OK} File found → $1${RESET}"
    else
        echo -e "${RED}${FAIL} Missing file → $1${RESET}"
    fi
}

check_dir() {
    if [ -d "$1" ]; then
        echo -e "${GREEN}${OK} Directory found → $1${RESET}"
    else
        echo -e "${RED}${FAIL} Missing directory → $1${RESET}"
    fi
}

# === JAVA ===
echo -e "${YELLOW}Checking Java (JDK 17)...${RESET}"
if command -v java &>/dev/null; then
    JAVA_VER=$(java -version 2>&1 | awk -F '"' '/version/ {print $2}')
    if [[ "$JAVA_VER" == 17* ]]; then
        echo -e "${GREEN}${OK} Java 17 detected → version $JAVA_VER${RESET}"
    else
        echo -e "${RED}${FAIL} Wrong Java version ($JAVA_VER), expected 17${RESET}"
    fi
else
    echo -e "${RED}${FAIL} Java not installed${RESET}"
fi
echo ""

# === GRADLE ===
echo -e "${YELLOW}Checking Gradle...${RESET}"
if command -v gradle &>/dev/null; then
    echo -e "${GREEN}${OK} gradle found → $(command -v gradle)${RESET}"
else
    echo -e "${RED}${FAIL} gradle missing${RESET}"
fi

if [ -f "$MAIN_DIR/gradlew" ]; then
    echo -e "${GREEN}${OK} gradlew found in $MAIN_DIR${RESET}"
else
    echo -e "${RED}${FAIL} gradlew missing in $MAIN_DIR${RESET}"
fi
echo ""

# === ANDROID SDK ===
echo -e "${YELLOW}Checking Android SDK...${RESET}"
if [ -d "$SDK_PATH" ]; then
    echo -e "${GREEN}${OK} SDK directory exists → $SDK_PATH${RESET}"
else
    echo -e "${RED}${FAIL} SDK directory missing → $SDK_PATH${RESET}"
fi

if command -v sdkmanager &>/dev/null; then
    echo -e "${GREEN}${OK} sdkmanager found${RESET}"
else
    echo -e "${RED}${FAIL} sdkmanager missing${RESET}"
fi

if command -v adb &>/dev/null; then
    echo -e "${GREEN}${OK} adb found${RESET}"
else
    echo -e "${RED}${FAIL} adb missing${RESET}"
fi
echo ""

# === MINGW TOOLCHAIN ===
echo -e "${YELLOW}Checking MinGW toolchain...${RESET}"
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    echo -e "${GREEN}${OK} x86_64-w64-mingw32-gcc found${RESET}"
else
    echo -e "${RED}${FAIL} x86_64-w64-mingw32-gcc missing${RESET}"
fi

if command -v x86_64-w64-mingw32-g++ &>/dev/null; then
    echo -e "${GREEN}${OK} x86_64-w64-mingw32-g++ found${RESET}"
else
    echo -e "${RED}${FAIL} x86_64-w64-mingw32-g++ missing${RESET}"
fi
echo ""

# === IMAGE MAGICK ===
echo -e "${YELLOW}Checking ImageMagick...${RESET}"
if magick -version &>/dev/null; then
    echo -e "${GREEN}${OK} ImageMagick installed${RESET}"
else
    echo -e "${RED}${FAIL} ImageMagick not found${RESET}"
fi
echo ""

# === PYTHON ENV ===
echo -e "${YELLOW}Checking Python environment...${RESET}"
if command -v python3 &>/dev/null; then
    echo -e "${GREEN}${OK} python3 found${RESET}"
else
    echo -e "${RED}${FAIL} python3 missing${RESET}"
fi

if command -v pip &>/dev/null; then
    echo -e "${GREEN}${OK} pip found${RESET}"
else
    echo -e "${RED}${FAIL} pip missing${RESET}"
fi

if [ -d "$MAIN_DIR/venv" ]; then
    echo -e "${GREEN}${OK} Virtual environment found → $MAIN_DIR/venv${RESET}"
else
    echo -e "${RED}${FAIL} Virtual environment missing → $MAIN_DIR/venv${RESET}"
fi
echo ""

# === PROJECT SDK PATHS ===
echo -e "${YELLOW}Checking Android project SDK paths...${RESET}"
for dir in "${PROJECTS[@]}"; do
    if [ -f "$dir/local.properties" ]; then
        SDK_REF=$(grep "sdk.dir=" "$dir/local.properties" | cut -d'=' -f2)
        if [[ "$SDK_REF" == "$SDK_PATH" ]]; then
            echo -e "${GREEN}${OK} $dir → SDK path correct${RESET}"
        else
            echo -e "${RED}${FAIL} $dir → SDK path mismatch ($SDK_REF)${RESET}"
        fi
    else
        echo -e "${RED}${FAIL} Missing local.properties in $dir${RESET}"
    fi
done
echo ""

# === LIBRARIES ===
echo -e "${YELLOW}Checking Dev Libraries...${RESET}"
LIBS=(libcurl4-openssl-dev libopencv-dev libwebkit2gtk-4.1-dev libportaudio2-dev libbluetooth-dev)
for lib in "${LIBS[@]}"; do
    if dpkg -s "$lib" &>/dev/null; then
        echo -e "${GREEN}${OK} $lib installed${RESET}"
    else
        echo -e "${RED}${FAIL} $lib missing${RESET}"
    fi
done
echo ""

# === FINAL ===
echo -e "${BOLD}${BLUE}Environment check complete.${RESET}"
echo "--------------------------------------"

