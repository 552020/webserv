#!/bin/bash

# Retrieve public IP address
IP=$(curl -s https://ifconfig.me)

# Fetch information using ipinfo.io API
INFO=$(curl -s https://ipinfo.io/${IP}/json)

# Print the results
echo "Your IP details:"
echo "================="
echo "$INFO" | jq '.'

