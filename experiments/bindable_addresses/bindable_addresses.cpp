#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>
#include <vector>
#include <cerrno>
#include <cstdlib>

struct AddressDescription
{
	std::string address;
	std::string description;
};

void readHostsFile(std::vector<AddressDescription> &additionalAddresses)
{
	std::ifstream hostsFile("/etc/hosts");
	if (!hostsFile.is_open())
	{
		std::cerr << "Failed to open /etc/hosts: " << strerror(errno) << '\n';
		return;
	}

	std::string line;
	while (std::getline(hostsFile, line))
	{
		if (line.empty() || line[0] == '#')
			continue; // Skip comments and empty lines
		std::istringstream iss(line);
		std::string address, alias;
		iss >> address;
		while (iss >> alias)
		{
			AddressDescription ad;
			ad.address = address;
			ad.description = alias;
			additionalAddresses.push_back(ad);
			std::cout << "Found in /etc/hosts: " << address << " -> " << alias << '\n';
		}
	}
	hostsFile.close();
}

void describe_address(const char *host, const std::map<std::string, std::string> &address_map)
{
	for (std::map<std::string, std::string>::const_iterator it = address_map.begin(); it != address_map.end(); ++it)
	{
		if (strncmp(host, it->first.c_str(), it->first.size()) == 0)
		{
			std::cout << " (" << it->second << ")";
			return; // Exit after the first match
		}
	}
}

// Find the appropriate description
void print_addresses(struct ifaddrs *ifaddr, const std::map<std::string, std::string> &address_map, bool verbose)
{
	for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		char host[NI_MAXHOST];
		int family = ifa->ifa_addr->sa_family;
		if (family == AF_INET || family == AF_INET6)
		{
			int s = getnameinfo(ifa->ifa_addr,
								(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
								host,
								NI_MAXHOST,
								NULL,
								0,
								NI_NUMERICHOST);
			if (s != 0)
			{
				std::cout << "getnameinfo() failed: " << gai_strerror(s) << '\n';
				continue;
			}
			std::cout << ifa->ifa_name << " " << ((family == AF_INET) ? "IPv4" : "IPv6") << " " << host;
			if (verbose)
			{
				describe_address(host, address_map);
			}
			std::cout << '\n';
		}
	}
}

int main()
{
	AddressDescription address_descriptions[] = {
		{"127.0.0.1", "Loopback"},
		{"::1", "Loopback"},
		{"fe80:", "Link-local"},
		{"fd00:", "Unique local"},
		{"ff00:", "Multicast"},
		{"2001:", "Teredo"},
		{"2002:", "6to4"},
		{"fc00:", "Unique local"},
		{"fec0:", "Site-local"},
		{"ff02:", "Link-local multicast"},
		{"ff05:", "Site-local multicast"},
		{"ff08:", "Organization-local multicast"},
		{"ff0e:", "Global multicast"}};				// Array of address descriptions using structs
	std::map<std::string, std::string> address_map; // Map of IP prefixes to descriptions
	for (int i = 0; i < sizeof(address_descriptions) / sizeof(AddressDescription); ++i)
	{
		address_map[address_descriptions[i].address] = address_descriptions[i].description;
	}
	std::vector<AddressDescription> additionalAddresses;
	readHostsFile(additionalAddresses);
	for (size_t i = 0; i < additionalAddresses.size(); ++i)
	{
		address_map[additionalAddresses[i].address] = additionalAddresses[i].description;
	}
	struct ifaddrs *ifaddr;

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	bool verbose = true; // Set verbosity based on user preference or command line options
	print_addresses(ifaddr, address_map, verbose);

	freeifaddrs(ifaddr);
	return 0;
}
