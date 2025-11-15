# Toralizer

A command-line tool written in C that intercepts network traffic from any other command-line program and automatically routes it through the Tor privacy network. This effectively masks your real IP address, and the application being used doesn't even need to be aware of it.

## 🚀 How It Works

Toralizer uses the `DYLD_INSERT_LIBRARIES` technique on macOS (similar to `LD_PRELOAD` on Linux) to intercept `connect()` system calls and redirect them through Tor's SOCKS proxy using the SOCKS4 protocol.

## 📋 Prerequisites

- macOS with Xcode Command Line Tools
- Homebrew package manager
- Tor service running (automatically installed and started during setup)

## 🛠️ Installation

### Phase 0: Setup (Completed ✓)

All prerequisites have been installed:
- ✓ Xcode Command Line Tools
- ✓ gcc, make, and tor via Homebrew
- ✓ Tor service started (listening on 127.0.0.1:9050)
- ✓ Project structure created

### Build the Library

```bash
make
```

This will compile `toralize.dylib`, the dynamic library that intercepts network connections.

## 📖 Usage

Run any command through Tor using the `toralize` wrapper script:

```bash
./toralize <command> [args...]
```

### Examples

Check your real IP vs Tor IP:
```bash
# Your real IP
curl http://ipinfo.io/ip

# Through Tor
./toralize curl http://ipinfo.io/ip
```

SSH through Tor:
```bash
./toralize ssh user@example.com
```

Download files through Tor:
```bash
./toralize wget http://example.com/file.txt
```

### Quick Test

Run the built-in test:
```bash
make test
```

This will show your real IP and then your IP through Tor.

## 📁 Project Structure

```
Toralizer/
├── toralize.h        # Header file with SOCKS4 protocol definitions
├── toralize.c        # Main implementation (connect() interception)
├── Makefile          # Build configuration
├── toralize          # Wrapper script to run commands through Tor
└── README.md         # This file
```

## 🔧 Development

### Clean Build Artifacts

```bash
make clean
```

### Rebuild Everything

```bash
make clean && make
```

## 🔒 Security Notes

- This tool routes TCP connections through Tor
- DNS requests may still leak (use Tor Browser for complete anonymity)
- Some applications may not work correctly with intercepted connections
- This is for educational purposes and legitimate privacy needs

## 📝 Technical Details

**SOCKS4 Protocol:**
- Connect to Tor proxy at 127.0.0.1:9050
- Send connection request with destination IP/port
- Receive response (code 90 = granted)
- Use `dup2()` to replace original socket with Tor-connected socket

**macOS-Specific:**
- Uses `DYLD_INSERT_LIBRARIES` environment variable
- Compiled as `.dylib` (dynamic library)
- Uses `dlsym(RTLD_NEXT, "connect")` to get original function

## 🐛 Troubleshooting

**Library not found:**
```bash
make
```

**Tor not running:**
```bash
brew services start tor
```

**Check Tor is listening:**
```bash
lsof -i :9050
```

## 📜 License

MIT License - Feel free to use and modify

## 👤 Author

Filip Snitil

---

**Next Steps:** Ready for Phase 1 - Building and testing the SOCKS4 client!

