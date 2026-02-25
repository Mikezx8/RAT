![RAT Logo](https://phongthuybta.com/wp-content/uploads/2025/12/rat-1.webp)

# RAT - Remote Administration Tool

A penetration testing framework I built over 2 months. Fully modular payload system like Metasploit. Can build payloads for Windows, Linux and Android. Supports ws, wss, http, https, tcp protocols. More payloads coming in the future.

## Features

- **Modular Payload System** - Stagers and payloads can be mixed and matched
- **Cross-Platform Support** - Windows (.exe), Linux (binary), Android (.apk)
- **Multi-Protocol Listeners** - ws, wss, http, https, tcp
- **Session Management** - Interact with active agents
- **Extensible** - Easy to add new payloads and protocols

## Architecture

The payload system works in stages:
- **Stager** - Small initial payload that connects back and fetches the main agent
- **Payload** - Full agent with post-exploitation functionality

This keeps the initial footprint small while allowing complex functionality.

## Screenshots

**Main Menu**
![Main Menu](images/Screenshot_2026-02-25_08-13-59.png)

**Windows Payload Build**
![Windows Build](images/Screenshot_2026-02-25_09-03-06.png)

**Android APK Build**
![Android Build](images/Screenshot_2026-02-25_09-04-29.png)

**Listener Management**
![Listeners](images/Screenshot_2026-02-25_09-10-22.png)

**Active Sessions**
![Sessions](images/Screenshot_2026-02-25_09-11-15.png)

**Build Guide**
![Build Guide](images/Screenshot_2026-02-25_09-15-29.png)

## Installation

```bash
# Clone the repository
git clone https://github.com/Mikezx8/RAT.git
mv RAT MOD
cd MOD
chmod +x setup.sh
./setup.sh
```

## Basic Usage

1. **Start a listener**
   ```
   >_ listeners
   listeners >_ create
   ```

2. **Generate a payload**
   ```
   >_ build_windows
   LHOST: 192.168.1.100
   LPORT: 443
   Protocol: https
   ```

3. **Deploy and interact**
   ```
   >_ sessions
   sessions >_ interact 1
   ```

## Build Commands

- `build_windows` - Generate Windows executable
- `build_linux` - Generate Linux binary  
- `build_android` - Generate Android APK

## Supported Protocols

- ws (WebSocket)
- wss (Secure WebSocket)
- http
- https
- tcp

## Roadmap

- More payload modules
- Additional protocols (DNS, ICMP)
- Encrypted communication
- Pivoting support

## Disclaimer

This tool is for authorized security testing and educational purposes only. Unauthorized access to computer systems is illegal. I am not responsible for misuse.

---

**More features coming soon.**
