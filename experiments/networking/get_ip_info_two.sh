#!/bin/bash

# Retrieve IPv4 address
IPV4=$(curl -4s ifconfig.me)

# Retrieve IPv6 address (if available)
IPV6=$(curl -6s ifconfig.me)

# Fetch additional information using ipinfo.io API for IPv4
INFO_IPV4=$(curl -s https://ipinfo.io/${IPV4}/json)

# Display the results
echo "Your IPv4 details:"
echo "=================="
echo "$INFO_IPV4" | jq '.'

# Check if IPv6 is available and display its details
if [ -n "$IPV6" ]; then
    echo "Your IPv6 details:"
    echo "=================="
    echo "IPv6 Address: $IPV6"
else
    echo "No IPv6 address detected."
fi
