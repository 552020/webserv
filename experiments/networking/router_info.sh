#!/bin/bash

# Extract the default gateway for IPv4
GATEWAY=$(netstat -nr | grep 'default' | grep -v 'fe80::' | awk '{print $2}')

echo "Your default gateway IP (Router IP): $GATEWAY"

# Check if nmap is installed and use it to try identifying the router
if command -v nmap &> /dev/null
then
    echo "Attempting to identify the router..."
    sudo nmap -O $GATEWAY
else
    echo "nmap is not installed. Cannot identify the router."
fi

