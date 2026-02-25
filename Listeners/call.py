from flask import Flask, Response, request
import time
import threading
from collections import deque
import io
from PIL import Image

app = Flask(__name__)

# Store the last 10 frames (adjust as needed)
frame_buffer = deque(maxlen=10)
latest_frame = None
frame_lock = threading.Lock()

@app.route('/')
def index():
    return """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Phone Screen Streaming</title>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
            * {
                margin: 0;
                padding: 0;
                box-sizing: border-box;
            }
            
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                background-color: #f5f5f5;
                padding: 10px;
            }
            
            h1 {
                color: #333;
                margin-bottom: 15px;
                font-size: 1.5em;
            }
            
            .controls {
                margin: 10px 0;
                display: flex;
                justify-content: center;
                gap: 10px;
                flex-wrap: wrap;
            }
            
            button {
                padding: 8px 16px;
                background-color: #4CAF50;
                color: white;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                font-size: 14px;
                transition: background-color 0.3s;
            }
            
            button:hover {
                background-color: #45a049;
            }
            
            #stream-container {
                position: relative;
                margin: 10px auto;
                max-width: 100%;
                max-height: calc(100vh - 150px);
                border: 2px solid #444;
                border-radius: 8px;
                box-shadow: 0 4px 8px rgba(0,0,0,0.2);
                overflow: hidden;
                background-color: #000;
                display: flex;
                align-items: center;
                justify-content: center;
            }
            
            #video-feed {
                max-width: 100%;
                max-height: 100%;
                width: auto;
                height: auto;
                display: block;
                object-fit: contain;
            }
            
            .status {
                margin-top: 10px;
                font-size: 14px;
                color: #555;
                display: flex;
                justify-content: center;
                gap: 20px;
                flex-wrap: wrap;
            }
            
            .status-item {
                background: #e0e0e0;
                padding: 5px 10px;
                border-radius: 4px;
                font-weight: bold;
            }
            
            .info-panel {
                margin-top: 10px;
                padding: 10px;
                background: #fff;
                border-radius: 4px;
                border: 1px solid #ddd;
                max-width: 600px;
                margin-left: auto;
                margin-right: auto;
            }
            
            .info-panel h3 {
                margin-bottom: 10px;
                color: #333;
            }
            
            .info-grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
                gap: 10px;
                text-align: left;
            }
            
            .info-item {
                background: #f8f8f8;
                padding: 8px;
                border-radius: 4px;
                border: 1px solid #eee;
            }
            
            .info-item label {
                font-weight: bold;
                color: #666;
                display: block;
                margin-bottom: 2px;
            }
            
            /* Fullscreen styles */
            #stream-container:fullscreen {
                max-width: 100vw;
                max-height: 100vh;
                width: 100vw;
                height: 100vh;
                border: none;
                border-radius: 0;
                margin: 0;
            }
            
            #stream-container:fullscreen #video-feed {
                max-width: 100vw;
                max-height: 100vh;
                width: 100vw;
                height: 100vh;
                object-fit: contain;
            }
            
            /* Hide controls in fullscreen */
            #stream-container:fullscreen ~ .controls,
            #stream-container:fullscreen ~ .status,
            #stream-container:fullscreen ~ .info-panel {
                display: none;
            }
            
            /* Responsive adjustments */
            @media (max-width: 768px) {
                body {
                    padding: 5px;
                }
                
                h1 {
                    font-size: 1.2em;
                }
                
                #stream-container {
                    max-height: calc(100vh - 120px);
                }
                
                .status {
                    font-size: 12px;
                }
                
                .info-panel {
                    padding: 8px;
                    font-size: 12px;
                }
            }
        </style>
    </head>
    <body>
        <h1>Phone Screen Streaming</h1>
        
        <div class="controls">
            <button onclick="refreshStream()">Refresh Stream</button>
            <button onclick="toggleFullscreen()">Toggle Fullscreen</button>
            <button onclick="toggleInfo()">Toggle Info</button>
            <button onclick="saveScreenshot()">Save Screenshot</button>
        </div>
        
        <div id="stream-container">
            <img id="video-feed" src="/video_feed" alt="Phone Screen Stream">
        </div>
        
        <div class="status">
            <div class="status-item">FPS: <span id="fps">0</span></div>
            <div class="status-item">Status: <span id="connection-status">Connecting...</span></div>
            <div class="status-item">Resolution: <span id="resolution">Unknown</span></div>
        </div>
        
        <div id="info-panel" class="info-panel" style="display: none;">
            <h3>Stream Information</h3>
            <div class="info-grid">
                <div class="info-item">
                    <label>Image Size:</label>
                    <span id="image-size">Unknown</span>
                </div>
                <div class="info-item">
                    <label>Data Received:</label>
                    <span id="data-received">0 MB</span>
                </div>
                <div class="info-item">
                    <label>Stream Time:</label>
                    <span id="stream-time">00:00</span>
                </div>
                <div class="info-item">
                    <label>Avg FPS:</label>
                    <span id="avg-fps">0</span>
                </div>
            </div>
        </div>
        
        <script>
            let frameCount = 0;
            let totalFrames = 0;
            let lastTime = Date.now();
            let startTime = Date.now();
            let totalDataReceived = 0;
            let currentFps = 0;
            let lastFrameTime = Date.now();
            let isFullscreen = false;
            let autoRefreshEnabled = true;
            let consecutiveErrors = 0;
            
            const fpsElement = document.getElementById('fps');
            const statusElement = document.getElementById('connection-status');
            const resolutionElement = document.getElementById('resolution');
            const videoFeed = document.getElementById('video-feed');
            const infoPanelElement = document.getElementById('info-panel');
            
            // Single FPS calculation interval
            const fpsInterval = setInterval(() => {
                const now = Date.now();
                const delta = now - lastTime;
                currentFps = Math.round((frameCount * 1000) / delta);
                fpsElement.textContent = currentFps;
                
                // Update average FPS
                const avgFps = Math.round((totalFrames * 1000) / (now - startTime));
                document.getElementById('avg-fps').textContent = avgFps;
                
                // Update stream time
                const streamTime = Math.floor((now - startTime) / 1000);
                const minutes = Math.floor(streamTime / 60);
                const seconds = streamTime % 60;
                document.getElementById('stream-time').textContent = 
                    `${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
                
                frameCount = 0;
                lastTime = now;
            }, 1000);
            
            // Handle image load - consolidated single handler
            videoFeed.onload = function() {
                frameCount++;
                totalFrames++;
                lastFrameTime = Date.now();
                consecutiveErrors = 0;
                
                statusElement.textContent = 'Connected';
                statusElement.style.color = '#4CAF50';
                
                // Update resolution
                resolutionElement.textContent = `${this.naturalWidth}x${this.naturalHeight}`;
                
                // Estimate data received (rough calculation)
                totalDataReceived += (this.naturalWidth * this.naturalHeight * 0.5 / 1024); // Rough JPEG size estimate
                document.getElementById('data-received').textContent = 
                    `${(totalDataReceived / 1024).toFixed(2)} MB`;
                
                // Update image size info
                document.getElementById('image-size').textContent = 
                    `${this.naturalWidth}x${this.naturalHeight}`;
            };
            
            // Handle connection errors with exponential backoff
            videoFeed.onerror = function() {
                consecutiveErrors++;
                statusElement.textContent = 'Connection Error';
                statusElement.style.color = '#f44336';
                
                // Only auto-refresh if not in fullscreen and auto-refresh is enabled
                if (autoRefreshEnabled && !isFullscreen && consecutiveErrors < 3) {
                    const delay = Math.min(1000 * Math.pow(2, consecutiveErrors), 10000);
                    setTimeout(() => {
                        if (!isFullscreen) {
                            refreshStream();
                        }
                    }, delay);
                }
            };
            
            // Monitor fullscreen state
            document.addEventListener('fullscreenchange', function() {
                isFullscreen = !!document.fullscreenElement;
                
                // Disable auto-refresh when in fullscreen
                if (isFullscreen) {
                    autoRefreshEnabled = false;
                    console.log('Fullscreen mode: Auto-refresh disabled');
                } else {
                    // Re-enable auto-refresh after a short delay when exiting fullscreen
                    setTimeout(() => {
                        autoRefreshEnabled = true;
                        console.log('Fullscreen exited: Auto-refresh re-enabled');
                    }, 2000);
                }
            });
            
            // Improved fullscreen function
            function toggleFullscreen() {
                const elem = document.getElementById('stream-container');
                if (!document.fullscreenElement) {
                    elem.requestFullscreen().catch(err => {
                        console.log(`Error attempting to enable fullscreen: ${err.message}`);
                        alert('Fullscreen not supported or blocked');
                    });
                } else {
                    document.exitFullscreen();
                }
            }
            
            // Toggle info panel
            function toggleInfo() {
                const panel = document.getElementById('info-panel');
                panel.style.display = panel.style.display === 'none' ? 'block' : 'none';
            }
            
            // Manual refresh function
            function refreshStream() {
                if (isFullscreen) {
                    console.log('Refresh blocked: Currently in fullscreen mode');
                    return;
                }
                
                statusElement.textContent = 'Refreshing...';
                statusElement.style.color = '#ff9800';
                
                // Add timestamp to force refresh
                const timestamp = Date.now();
                videoFeed.src = `/video_feed?t=${timestamp}`;
                
                // Reset error counter
                consecutiveErrors = 0;
            }
            
            // Save screenshot
            function saveScreenshot() {
                try {
                    const canvas = document.createElement('canvas');
                    const ctx = canvas.getContext('2d');
                    canvas.width = videoFeed.naturalWidth;
                    canvas.height = videoFeed.naturalHeight;
                    ctx.drawImage(videoFeed, 0, 0);
                    
                    const link = document.createElement('a');
                    link.download = `screenshot_${new Date().toISOString().replace(/[:.]/g, '-')}.png`;
                    link.href = canvas.toDataURL();
                    link.click();
                } catch (e) {
                    console.error('Screenshot failed:', e);
                    alert('Screenshot failed. Please try again.');
                }
            }
            
            // Keyboard shortcuts
            document.addEventListener('keydown', function(e) {
                // Only handle shortcuts if not in an input field
                if (e.target.tagName.toLowerCase() === 'input') return;
                
                switch(e.key.toLowerCase()) {
                    case 'f':
                        e.preventDefault();
                        toggleFullscreen();
                        break;
                    case 'i':
                        e.preventDefault();
                        toggleInfo();
                        break;
                    case 's':
                        e.preventDefault();
                        saveScreenshot();
                        break;
                    case 'r':
                        e.preventDefault();
                        refreshStream();
                        break;
                    case 'escape':
                        if (isFullscreen) {
                            document.exitFullscreen();
                        }
                        break;
                }
            });
            
            // Improved auto-refresh with better logic
            const autoRefreshInterval = setInterval(() => {
                if (!autoRefreshEnabled || isFullscreen) {
                    return;
                }
                
                const timeSinceLastFrame = Date.now() - lastFrameTime;
                
                // Only refresh if no frames for 15 seconds and not too many consecutive errors
                if (timeSinceLastFrame > 15000 && consecutiveErrors < 3) {
                    console.log('No frames received for 15 seconds, refreshing...');
                    refreshStream();
                }
            }, 5000);
            
            // Prevent accidental navigation
            window.addEventListener('beforeunload', function(e) {
                if (isFullscreen) {
                    e.preventDefault();
                    e.returnValue = '';
                    return '';
                }
            });
            
            // Clean up intervals on page unload
            window.addEventListener('unload', function() {
                clearInterval(fpsInterval);
                clearInterval(autoRefreshInterval);
            });
            
            // Initial status
            console.log('Phone Screen Streaming initialized');
        </script>
    </body>
    </html>
    """

@app.route('/stream', methods=['POST'])
def stream():
    global latest_frame
    # Get the image data from the request
    image_data = request.data
    
    try:
        # Convert bytes to image
        image = Image.open(io.BytesIO(image_data))
        
        # Log image dimensions for debugging
        print(f"Received image: {image.size[0]}x{image.size[1]} pixels")
        
        # Store the latest frame
        with frame_lock:
            frame_buffer.append(image)
            latest_frame = image
        
        return Response(status=200)
    except Exception as e:
        print(f"Error processing image: {e}")
        return Response(status=500)

@app.route('/video_feed')
def video_feed():
    def generate():
        while True:
            with frame_lock:
                if latest_frame:
                    # Convert image to JPEG with higher quality
                    img_io = io.BytesIO()
                    latest_frame.save(img_io, 'JPEG', quality=85, optimize=True)
                    img_io.seek(0)
                    frame = img_io.getvalue()
                    
                    yield (b'--frame\r\n'
                           b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
                else:
                    # Send a blank frame if no image is available
                    blank_frame = Image.new('RGB', (1080, 1920), (0, 0, 0))  # Common phone resolution
                    img_io = io.BytesIO()
                    blank_frame.save(img_io, 'JPEG')
                    img_io.seek(0)
                    frame = img_io.getvalue()
                    
                    yield (b'--frame\r\n'
                           b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
            
            time.sleep(0.1)  # Adjust frame rate (0.1 = ~10fps)
            
    return Response(generate(),
                   mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/info')
def info():
    """Endpoint to get current stream information"""
    with frame_lock:
        if latest_frame:
            return {
                'width': latest_frame.size[0],
                'height': latest_frame.size[1],
                'format': latest_frame.format,
                'mode': latest_frame.mode
            }
        else:
            return {'error': 'No frame available'}

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, threaded=True, debug=True)