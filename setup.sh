#!/bin/bash
MAIN_DIR=$HOME/MOD
SED_DIR=$HOME/MOD/Monitor
SED_DIR2=$HOME/MOD/Monitor2
SED_DIR3=$HOME/MOD/Monitor3
SED_DIR4=$HOME/MOD/websession
APP_NAME="MOD"
EXEC_PATH="$HOME/MOD/builder.sh"
ICON_PATH="$HOME/MOD/MOD.png"
DESKTOP_FILE="$HOME/.local/share/applications/$APP_NAME.desktop"

JAVA_COMMAND="java -version"

# Check if Java is installed and version is 17
if command -v java >/dev/null 2>&1; then
  JAVA_VER=$(java -version 2>&1 | awk -F '"' '/version/ {print $2}' | cut -d. -f1)
  if [ "$JAVA_VER" = "17" ]; then
    echo "JDK 17 already installed."
    java -version
    else
echo "Installing OpenLogic OpenJDK 17..."
wget https://builds.openlogic.com/downloadJDK/openlogic-openjdk/17.0.16+8/openlogic-openjdk-17.0.16+8-linux-x64-deb.deb
sudo apt install ./openlogic-openjdk-17.0.16+8-linux-x64-deb.deb

sudo update-alternatives --install /usr/bin/java java /usr/lib/jvm/openlogic-openjdk-17-hotspot-amd64/bin/java 1
sudo update-alternatives --install /usr/bin/javac javac /usr/lib/jvm/openlogic-openjdk-17-hotspot-amd64/bin/javac 1

# Set OpenLogic JDK 17 as the default
sudo update-alternatives --set java /usr/lib/jvm/openlogic-openjdk-17-hotspot-amd64/bin/java
sudo update-alternatives --set javac /usr/lib/jvm/openlogic-openjdk-17-hotspot-amd64/bin/javac

#check to verify java setup
java -version
javac -version
  fi
fi

#update package list and install JDK17
sudo apt update

#GRADLE INSTALLATION SEETUP
sudo apt install gradle
gradle -v

gradle wrapper --gradle-version 8.11.1
chmod +x gradlew

cd
mkdir -p ~/Android/Sdk

wget https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip
unzip commandlinetools-linux-13114758_latest.zip -d ~/Android/Sdk/

mkdir -p ~/Android/Sdk/cmdline-tools/latest

# Only move contents if "latest" doesn't already exist
if [ ! -d ~/Android/Sdk/cmdline-tools/latest/bin ]; then
  echo "Moving command-line tools into 'latest' directory..."
  mv ~/Android/Sdk/cmdline-tools/* ~/Android/Sdk/cmdline-tools/latest/ 2>/dev/null || true
else
  echo "Command-line tools already structured correctly."
fi

echo 'export ANDROID_HOME=$HOME/Android/Sdk' >> ~/.zshrc
echo 'export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools' >> ~/.zshrc
source ~/.zshrc

echo "Accept all Android Licenses"
yes | sdkmanager --licenses

cd $SED_DIR
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
cd $SED_DIR2
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
cd $SED_DIR3
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
cd $SED_DIR4
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties

cd $MAIN_DIR
./gradlew clean
./gradlew build

#C setup and installation
sudo apt-get install libpthread-stubs0-dev libx11-dev libxtst-dev -y
sudo apt-get install mingw-w64 gcc-mingw-w64 g++-mingw-w64 -y

# Install webkit2gtk development package
sudo apt install libwebkit2gtk-4.1-dev -y

# Install OpenCV development package
sudo apt install libopencv-dev -y

# Install other potential missing dependencies
sudo apt install libbluetooth-dev libportaudio2-dev libssl-dev libboost-all-dev libx11-dev libxtst-dev -y

sudo apt install portaudio19-dev libbluetooth-dev -y

sudo apt install libcurl4-openssl-dev -y

sudo apt install libwebsocketpp-dev -y

sudo apt install portaudio19-dev -y

sudo apt-get install libwebsockets-dev -y

sudo apt-get install libssl-dev:i386 -y

sudo apt-get install libboost-all-dev -y

sudo apt install imagemagick -y

#Check Version
x86_64-w64-mingw32-gcc --version

sudo apt install gdown
gdown https://drive.google.com/uc?id=1rm0C8svbPXKqj-ll6B3t406r51OnHRsA -O $HOME/mingw-libs2.zip && \
unzip $HOME/mingw-libs2.zip -d $HOME && \
rm $HOME/mingw-libs2.zip

if command -v pip >/dev/null 2>&1; then
  echo "PIP READY"
else
  echo "pip not found — installing..."
  sudo apt install python3-pip -y
fi

# Create a virtual environment named "venv"
python3 -m venv venv

# Activate it (Linux / macOS)
source venv/bin/activate

sudo apt install direnv

echo 'eval "$(direnv hook bash)"' >> ~/.bashrc
source ~/.bashrc

cd $MAIN_DIR
echo 'source venv/bin/activate' > .envrc
direnv allow

source venv/bin/activate

pip install -r requirements.txt

sudo tee /usr/local/bin/MOD > /dev/null <<'MASH_EOF'
#!/usr/bin/env bash
set -euo pipefail
IFS=$'\n\t'

# Determine real user's home (works if run under sudo)
if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != "root" ]; then
  REAL_HOME="$(getent passwd "$SUDO_USER" | cut -d: -f6)"
else
  REAL_HOME="${HOME:-/root}"
fi

BASE="${REAL_HOME}/MOD"
BUILD="${BASE}/builder.sh"
README="${BASE}/readme.html"

usage() {
  cat <<USAGE
Usage: MOD [command]

Commands:
  help|--help|-wh|--whelp    Show README (if present) or path
  --cleanup|--clean|--remove-clones
                             Remove duplicate git clones inside ${BASE};
                             keeps the newest copy for each repo name.
  (default)                 Exec ${BUILD} with passed args (if executable)
USAGE
}

# cleanup duplicates: for directories under $BASE that are git repos,
# group by repository name (derived from remote.origin.url or directory name)
# and keep the newest (mtime) while removing older duplicates.
cleanup_clones() {
  if [ ! -d "$BASE" ]; then
    echo "Base directory not found: $BASE"
    return 0
  fi

  declare -A keep_map  # repo_name -> kept_dir
  declare -A keep_mtime

  # iterate only one level deep directories
  while IFS= read -r -d '' dir; do
    # only consider directories
    repo_dir="$dir"
    # try to get origin URL; if absent, use basename
    origin_url="$(git -C "$repo_dir" config --get remote.origin.url 2>/dev/null || true)"
    if [ -n "$origin_url" ]; then
      # repo name from URL: take last path component, remove .git
      repo_name="$(basename "$origin_url" | sed 's/\.git$//')"
    else
      repo_name="$(basename "$repo_dir")"
    fi

    mtime=$(stat -c %Y "$repo_dir" 2>/dev/null || echo 0)

    prev="${keep_map[$repo_name]:-}"
    if [ -z "$prev" ]; then
      keep_map["$repo_name"]="$repo_dir"
      keep_mtime["$repo_name"]="$mtime"
    else
      prev_mtime="${keep_mtime[$repo_name]}"
      if (( mtime > prev_mtime )); then
        # new dir is newer — mark previous for deletion and keep new
        echo "Removing older duplicate: '$prev'  (keeping newer: '$repo_dir')"
        rm -rf -- "$prev"
        keep_map["$repo_name"]="$repo_dir"
        keep_mtime["$repo_name"]="$mtime"
      else
        # this one is older — remove it
        echo "Removing older duplicate: '$repo_dir'  (keeping: '$prev')"
        rm -rf -- "$repo_dir"
      fi
    fi
  done < <(find "$BASE" -maxdepth 1 -mindepth 1 -type d -print0 2>/dev/null)

  echo "Cleanup complete."
}

case "${1:-}" in
  help|--help|-wh|--whelp)
    if [ -f "$README" ]; then
      xdg-open "$README" >/dev/null 2>&1 || sensible-browser "$README" >/dev/null 2>&1 || echo "$README"
    else
      echo "README not found: $README"
    fi
    ;;
  --cleanup|--clean|--remove-clones)
    cleanup_clones
    ;;
  *)
    if [ -x "$BUILD" ]; then
      exec "$BUILD" "$@"
    else
      echo "Build script not found or not executable: $BUILD"
    fi
    ;;
esac
MASH_EOF

sudo chmod +x /usr/local/bin/MOD

# Ensure target directory exists
mkdir -p "$(dirname "$DESKTOP_FILE")"

# Create the .desktop file
cat <<EOF > "$DESKTOP_FILE"
[Desktop Entry]
Name=$APP_NAME
Comment=$APP_NAME Terminal Application
Exec=gnome-terminal -- bash -c '$EXEC_PATH; exec bash'
Icon=$ICON_PATH
Terminal=true
Type=Application
Categories=Utility;
StartupNotify=true
EOF

# Make it executable
chmod +x "$DESKTOP_FILE"

# Update desktop database (if supported)
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database ~/.local/share/applications
fi

echo " $APP_NAME terminal desktop shortcut created at: $DESKTOP_FILE"
echo "It will now open inside a terminal window."

echo BUILD COMPLETE!
exit
