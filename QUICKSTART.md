# toralizer quick start

get up and running in 5 minutes.

## 1. verify prerequisites (30 seconds)

```bash
./tests/test_phase0.sh
```

if this passes (16/16 tests), you're ready to go.

## 2. build toralizer (30 seconds)

```bash
make
```

this creates `toralize.dylib` - the magic that intercepts connections.

## 3. test it works (1 minute)

```bash
make phase2
```

you should see all 5/5 tests pass with output showing:
- `[toralize] intercepting connection`
- `[toralize] connected to tor proxy`
- `[toralize] connection granted, routing through tor`

## 4. run your first command (30 seconds)

```bash
# test with the included binary
./toralize ./test_http
```

you'll see interception messages and the connection routes through tor.

## 5. try the examples (2 minutes)

```bash
cd examples
./test-with-binary.sh
```

this shows direct vs tor connections side-by-side.

## that's it!

you now have a working toralizer installation.

## what works

**custom binaries** (always work):
```bash
./toralize ./your_program
./toralize ./test_http
./toralize python3 your_script.py
```

**homebrew binaries** (work):
```bash
brew install curl
# example scripts automatically detect homebrew curl (even if keg-only)
cd examples
./check-ip.sh
```

## what doesn't work

**system binaries** (blocked by sip):
```bash
./toralize /usr/bin/curl  # won't be intercepted
./toralize /usr/bin/ssh   # won't be intercepted
```

these are in `/usr/bin`, `/bin`, `/sbin` and are protected by macOS system integrity protection.

## next steps

**learn more about usage**:
- read `USAGE.md` for 10+ real-world use cases
- explore `examples/` directory for ready-to-run scripts
- check `PHASES.md` to understand how it works

**create your own tools**:
```bash
# write a script
cat > my_tool.py << 'EOF'
import socket
s = socket.socket()
s.connect(('1.1.1.1', 80))
s.send(b'GET / HTTP/1.1\r\nHost: one.one.one.one\r\n\r\n')
print(s.recv(1024).decode())
EOF

# run through tor
./toralize python3 my_tool.py
```

**common patterns**:
```bash
# check your tor exit node
./toralize ./test_http 2>&1 | grep intercepting

# run multiple commands
for i in {1..3}; do
    ./toralize ./test_http
done

# save output
./toralize ./test_http > output.txt 2>&1
```

## troubleshooting

**tor not running**:
```bash
brew services start tor
lsof -i :9050  # verify it's listening
```

**library not found**:
```bash
ls -la toralize.dylib  # check it exists
make clean && make     # rebuild if needed
```

**no interception**:
```bash
# are you using a system binary?
which curl  # /usr/bin/curl = won't work

# use homebrew instead
brew install curl
# homebrew curl is keg-only, so it stays at /opt/homebrew/opt/curl/bin/curl
# the example scripts automatically detect and use it
```

**still having issues**:
```bash
# run the full test suite
./tests/test_phase0.sh  # prerequisites
make phase1             # socks4 protocol
make phase2             # dynamic library
```

all three should pass.

## important notes

1. **system binaries don't work** - use homebrew or compile your own
2. **tor is slow** - expect 200-1000ms latency, this is normal
3. **dns might leak** - use IP addresses directly for maximum anonymity
4. **respect rate limits** - just because you can doesn't mean you should
5. **legal use only** - this tool is for legitimate privacy and testing

## quick reference

```bash
# build
make

# test
make phase2

# use
./toralize <command>

# clean
make clean

# examples
cd examples && ./test-with-binary.sh
```

## documentation

- `README.md` - main documentation
- `USAGE.md` - comprehensive usage guide with 10+ use cases
- `PHASES.md` - implementation details
- `examples/README.md` - example scripts documentation
- `tests/README.md` - testing documentation

## getting help

1. run tests: `make phase2`
2. check tor: `lsof -i :9050`
3. try example: `./examples/test-with-binary.sh`
4. read docs: `USAGE.md`

## ready to use

you're all set! start routing traffic through tor:

```bash
./toralize ./test_http
```

welcome to anonymous networking.

