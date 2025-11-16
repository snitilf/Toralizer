# toralizer examples

practical example scripts demonstrating common use cases.

## quick start

**important**: these example scripts use `curl` which may be blocked by macOS system integrity protection (sip). for guaranteed results, the test suite uses custom-compiled binaries.

to see toralizer working:
```bash
cd ..
make phase2  # this uses test_http (not sip-protected)
```

or install homebrew curl (recommended):
```bash
brew install curl
# scripts automatically detect homebrew curl (even if keg-only)
```

note: homebrew curl is installed as "keg-only" by default, meaning it won't override system curl but the scripts will find it automatically.

basic usage:
```bash
cd examples
chmod +x *.sh
./check-ip.sh
```

## scripts

### check-ip.sh

compares your real ip address with the tor exit node ip.

usage:
```bash
./check-ip.sh
```

output:
```
comparing ip addresses...

your real ip:    173.178.53.50
tor exit node:   185.220.101.34

if the ips are different, toralizer is working!
```

### check-geo.sh

shows the geographic location of the current tor exit node.

requirements: `jq` (install with `brew install jq`)

usage:
```bash
./check-geo.sh
```

output:
```
checking tor exit node location...

ip:       185.220.101.34
city:     Frankfurt am Main
country:  Germany
region:   Hesse
org:      AS24940 Hetzner Online GmbH
```

### test-anonymity.sh

comprehensive test to verify toralizer is working correctly.

usage:
```bash
./test-anonymity.sh
```

checks:
- real ip retrieval
- tor ip retrieval
- ip comparison
- exit node rotation

### scrape-example.sh

demonstrates basic web scraping through tor with proper delays.

usage:
```bash
./scrape-example.sh
```

shows:
- making multiple requests through tor
- different exit nodes for each request
- proper delay between requests
- basic error handling

### test-with-binary.sh

demonstrates toralizer with a custom-compiled binary (not sip-protected).

usage:
```bash
./test-with-binary.sh
```

shows:
- direct connection vs tor connection
- interception messages
- guaranteed to work (uses test_http)

## creating your own scripts

basic template:

```bash
#!/bin/bash
#
# my-script.sh - description

# your command here
../toralize curl https://example.com

# or with error handling
if ../toralize curl -s https://example.com > output.txt; then
    echo "success"
else
    echo "failed"
fi
```

## common patterns

### checking exit node info

```bash
# get ip only
../toralize curl -s http://ipinfo.io/ip

# get full json
../toralize curl -s http://ipinfo.io/json | jq .

# get specific field
../toralize curl -s http://ipinfo.io/json | jq -r '.city'
```

### looping with delays

```bash
for i in {1..10}; do
    ../toralize curl https://example.com/page/$i
    sleep 5  # wait between requests
done
```

### saving responses

```bash
# save to file
../toralize curl https://example.com > response.html

# save multiple
for id in {1..100}; do
    ../toralize curl https://api.example.com/item/$id > data/$id.json
    sleep 2
done
```

### error handling

```bash
if ! ../toralize curl https://example.com; then
    echo "request failed"
    exit 1
fi
```

## notes

- all scripts assume they're run from the `examples/` directory
- adjust paths if running from elsewhere
- always add appropriate delays between requests
- respect rate limits and terms of service

## troubleshooting

**scripts don't run**:
```bash
chmod +x *.sh
```

**curl not found or not working**:
```bash
brew install curl
# scripts will automatically find it
# optionally: brew link --force curl (to override system curl)
```

**jq not found**:
```bash
brew install jq
```

**toralizer not working**:
```bash
cd ..
./tests/test_phase2.sh
```

for more examples and use cases, see USAGE.md in the project root.

