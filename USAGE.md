# toralizer usage guide

practical examples and real-world use cases for routing applications through tor.

## what is toralizer?

toralizer intercepts network connections from command-line programs and automatically routes them through the tor network. this masks your real IP address without requiring the application to have built-in proxy support.

the key benefit: you can toralize ANY application without modifying it or configuring proxies.

## why is this useful?

**privacy**: hide your real IP address from services you connect to

**testing**: simulate connections from different geographic locations (tor exit nodes rotate)

**bypassing restrictions**: access content that may be blocked in your region

**web scraping**: avoid IP-based rate limiting or blocking

**security research**: investigate services anonymously

**development**: test how your application behaves from different network contexts

## quick start

basic syntax:
```bash
./toralize <command> [arguments...]
```

the command runs normally, but all TCP connections go through tor.

## real-world use cases

### 1. anonymous web requests

**scenario**: you want to check a website without revealing your IP address.

```bash
# check what IP the server sees (without tor)
curl http://ipinfo.io/ip
# output: 173.178.53.50 (your real IP)

# check what IP the server sees (with tor)
./toralize curl http://ipinfo.io/ip
# output: 185.220.101.34 (tor exit node IP)
```

**why useful**: when researching competitors, checking if a site is accessible, or simply browsing without being tracked.

### 2. web scraping without getting blocked

**scenario**: you're scraping data but the site blocks your IP after too many requests.

```bash
# scrape data through tor
./toralize curl -s https://example.com/api/data | jq .

# run multiple requests - each might exit from different tor nodes
for i in {1..10}; do
    ./toralize curl -s https://example.com/page/$i
    sleep 2
done
```

**why useful**: many sites block IP addresses that make too many requests. tor rotation helps avoid detection.

**note**: respect robots.txt and terms of service. some sites explicitly prohibit scraping.

### 3. testing geo-restrictions

**scenario**: you're developing a website and need to test how it behaves from different countries.

```bash
# check current geolocation
./toralize curl https://ipapi.co/json/ | jq '{country, city, ip}'

# example output might show:
# {
#   "country": "DE",
#   "city": "Frankfurt",
#   "ip": "185.220.101.34"
# }

# run again - might exit from different country
./toralize curl https://ipapi.co/json/ | jq '{country, city, ip}'
```

**why useful**: test if your CDN, payment processing, or content delivery works correctly from different regions.

### 4. anonymous api testing

**scenario**: testing a third-party API during development without linking requests to your real IP.

```bash
# test api endpoint
./toralize curl -X POST https://api.example.com/v1/data \
    -H "Content-Type: application/json" \
    -d '{"test": "data"}'

# check rate limiting behavior
for i in {1..50}; do
    ./toralize curl -s https://api.example.com/test
done
```

**why useful**: when testing rate limits, abuse detection, or just exploring an API anonymously.

### 5. downloading files anonymously

**scenario**: downloading files without revealing your identity or location.

```bash
# download through tor
./toralize wget https://example.com/file.zip

# or with curl
./toralize curl -O https://example.com/document.pdf
```

**why useful**: downloading controversial content, whistleblowing documents, or accessing blocked resources.

### 6. checking if you're blocked

**scenario**: you suspect a service has blocked your IP address.

```bash
# try accessing from your real IP
curl https://example.com/service
# (might get 403 or timeout)

# try through tor
./toralize curl https://example.com/service
# (might work if it's an IP block)
```

**why useful**: determine if issues are IP-related or service-wide.

### 7. security research

**scenario**: analyzing malicious websites or testing security tools without exposing your network.

```bash
# check suspicious URL safely
./toralize curl -I https://suspicious-site.com

# test if service leaks information
./toralize nmap -sT scanme.nmap.org
```

**why useful**: investigate threats without revealing your research infrastructure.

### 8. automated monitoring from "different" locations

**scenario**: monitoring if your service is accessible globally.

```bash
#!/bin/bash
# monitor.sh - check service availability through tor

for i in {1..5}; do
    echo "check $i:"
    response=$(./toralize curl -s -o /dev/null -w "%{http_code}" https://yoursite.com)
    echo "  status: $response"
    sleep 10
done
```

**why useful**: basic availability testing from different network perspectives.

### 9. bypass ip-based rate limiting

**scenario**: working with an API that has strict per-IP rate limits.

```bash
# your script needs to make many requests
#!/bin/bash

for item in $(cat items.txt); do
    # each request goes through tor
    ./toralize curl https://api.example.com/item/$item > results/$item.json
    
    # rotate tor circuit between requests (requires tor control)
    # (pkill -HUP tor)
    
    sleep 2
done
```

**why useful**: legitimate use cases where you need more requests than per-IP limits allow.

### 10. privacy-focused git operations

**scenario**: cloning or accessing git repositories anonymously.

```bash
# clone repository through tor
./toralize git clone https://github.com/user/repo.git

# fetch updates
cd repo
../toralize git fetch origin
```

**why useful**: accessing repositories without revealing your identity or location.

## working with homebrew packages

since macOS system integrity protection (sip) blocks /usr/bin binaries, use homebrew versions:

```bash
# install homebrew curl (not sip-protected)
brew install curl-openssl

# use with toralizer
./toralize /opt/homebrew/bin/curl http://ipinfo.io/ip

# or add to path
export PATH="/opt/homebrew/bin:$PATH"
./toralize curl http://ipinfo.io/ip
```

common homebrew packages that work:
- curl-openssl
- wget
- python (custom scripts)
- node (javascript)
- ruby scripts
- go programs
- rust programs

## advanced usage

### running your own compiled programs

toralizer works perfectly with programs you compile yourself:

```python
# fetch.py - simple http client
import socket
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('1.1.1.1', 80))
sock.send(b'GET / HTTP/1.1\r\nHost: one.one.one.one\r\n\r\n')
print(sock.recv(1024).decode())
sock.close()
```

```bash
# run through tor
./toralize python3 fetch.py
# connection will be intercepted and routed through tor
```

### combining with other tools

```bash
# use with ssh (homebrew version)
./toralize ssh user@server.com

# use with custom http client
./toralize ./my_http_client --url https://example.com

# use in scripts
./toralize python3 my_scraper.py --target site.com
```

## limitations and workarounds

### system binaries don't work

**problem**: `/usr/bin/curl`, `/usr/bin/ssh` are protected by sip

**solution**: use homebrew or compile yourself
```bash
brew install curl-openssl
./toralize /opt/homebrew/bin/curl http://example.com
```

### dns leaks

**problem**: dns requests might not go through tor

**solution**: use IP addresses directly or configure system-wide dns over tor
```bash
# use IP instead of hostname
./toralize curl http://1.1.1.1/

# or setup tor dns resolver separately
```

### https certificate validation

**problem**: some applications verify ssl certificates that might look suspicious from tor

**solution**: this is actually good - don't disable certificate validation
```bash
# tor exit nodes can't see https content (it's encrypted)
# certificate validation still works normally
./toralize curl https://secure-site.com
```

### udp traffic not supported

**problem**: toralizer only routes tcp connections

**solution**: tor only supports tcp anyway. for udp, need different solutions

### slow connections

**problem**: tor routing adds latency

**solution**: this is normal - you're routing through multiple relays globally
```bash
# expect slower speeds than direct connection
# typical tor speed: 1-5 MB/s
# typical latency: 200-1000ms
```

## troubleshooting

### tor not running

```bash
# check if tor is running
lsof -i :9050

# if not, start it
brew services start tor

# check logs if issues
brew services info tor
```

### library not loading

```bash
# verify library exists
ls -la toralize.dylib

# rebuild if needed
make clean && make
```

### no interception happening

```bash
# verify you're not using a system binary
which curl  # if /usr/bin/curl, won't work

# install homebrew version
brew install curl-openssl
which curl  # should show /opt/homebrew/bin/curl
```

### connection refused

```bash
# verify tor proxy is accessible
nc -zv 127.0.0.1 9050

# check tor configuration
cat /opt/homebrew/etc/tor/torrc
```

## best practices

1. **test first**: run `./tests/test_phase2.sh` to verify everything works

2. **use responsibly**: respect rate limits and terms of service

3. **verify anonymity**: always check your exit IP with `./toralize curl http://ipinfo.io/ip`

4. **don't rely solely on tor**: for true anonymity, use tor browser with additional precautions

5. **understand limitations**: tor provides network-level anonymity, not application-level

6. **respect tor network**: avoid high-bandwidth activities like torrenting

7. **check exit node location**: different exit nodes = different geographic locations

## security considerations

**what toralizer protects**:
- hides your real IP address from destination servers
- routes tcp connections through tor network
- prevents direct connection to destination

**what toralizer does NOT protect**:
- application-level tracking (cookies, fingerprinting)
- dns leaks (use additional dns protection)
- traffic analysis by tor network itself
- content of https requests (but exit node can't see it either)

**for maximum privacy**:
- use tor browser for web browsing
- use toralizer for command-line tools and scripts
- combine with vpn for additional layers
- be aware of application-specific leaks

## legal and ethical considerations

**legal uses**:
- privacy protection
- security research
- bypassing censorship
- legitimate testing and development

**use responsibly**:
- respect website terms of service
- don't use for illegal activities
- be aware of local laws regarding tor usage
- don't abuse services or networks

**remember**: anonymity tools can be used for good or bad. always act ethically and legally.

## examples directory

create a directory with useful scripts:

```bash
mkdir examples

# check-ip.sh
cat > examples/check-ip.sh << 'EOF'
#!/bin/bash
echo "real ip: $(curl -s http://ipinfo.io/ip)"
echo "tor ip:  $(./toralize curl -s http://ipinfo.io/ip)"
EOF

# test-geo.sh  
cat > examples/test-geo.sh << 'EOF'
#!/bin/bash
./toralize curl -s https://ipapi.co/json/ | jq '{ip, city, country, org}'
EOF

chmod +x examples/*.sh
```

## getting help

if you encounter issues:

1. run the test suite: `make phase2`
2. check tor is running: `lsof -i :9050`
3. verify library built: `ls -la toralize.dylib`
4. check logs: `brew services info tor`
5. try simple test: `./toralize ./test_http`

for more information, see:
- README.md - basic setup and installation
- PHASES.md - technical implementation details  
- tests/README.md - testing procedures

## conclusion

toralizer provides a simple way to route any command-line application through tor without modifying the application or configuring complex proxy settings. use it responsibly for privacy, testing, and development purposes.

remember: with great power comes great responsibility. use tor and toralizer ethically and legally.

