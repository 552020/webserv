# Bindable Address Finder

Small utility test program to find and enumerate all network interfaces on a machine and list their corresponding IP addresses, both IPv4 and IPv6. This can be used to identify all possible IP addresses to which a server socket can bind on the host machine.

## How It Works

The program uses several system headers to interact with network interfaces:

- `ifaddrs.h`: Provides the `getifaddrs()` function which is used to obtain the linked list of network interfaces.
- `netdb.h`: Used for the `getnameinfo()` function which converts socket addresses into human-readable textual representations of hostnames or IP addresses.

Both functions are not allowed by the subject of Webserv. C++ doesn't have own headers for these functions.

### Key Functions

#### `getifaddrs()`

- **Purpose**: Retrieves the network interfaces of the local system.
- **Returns**: 0 on success, -1 on error.
- **Usage**: It fills a linked list of structures describing the network interfaces of the local system, and any address associated with those interfaces.
- **Structure**: Each `struct ifaddrs` includes:
  - `ifa_next`: Pointer to the next structure in the list.
  - `ifa_name`: Name of the interface.
  - `ifa_flags`: Flags as defined in `<net/if.h>`.
  - `ifa_addr`: Address of the interface.
  - `ifa_netmask`: Netmask of the interface.
  - Other members for addressing specifics.

#### `getnameinfo()`

- **Purpose**: Converts a socket address to a corresponding host and service, in a protocol-independent manner.
- **Usage**: Used here to convert the addresses in the `struct sockaddr` (contained within the `struct ifaddrs`) into human-readable IP addresses.

## Example Usage

Compile and run the program to list all network interfaces and their respective IP addresses. The output will include the interface name, the type of IP (IPv4 or IPv6), and the IP address itself.

### Compilation

To compile the program, you can use g++ or any standard C++ compiler. For example:

```bash
g++ -o bindable_address bindable_address.cpp
./bindable_address
```

This will generate output similar to:

```
lo IPv4 127.0.0.1
eth0 IPv4 192.168.1.101
eth0 IPv6 fe80::f03c:91ff:fe69:4562
```

### Error Handling

Errors from `getifaddrs` are reported to `stderr` with a description of the error. The program exits with `EXIT_FAILURE` in case of failure.

## Conclusion

This utility is a handy tool for system and network administrators, as well as developers, to quickly find all the IPs available for binding applications such as web servers.

---

Feel free to adjust this documentation according to your project's structure or specific needs!
