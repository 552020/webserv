#include <iostream>
#include <ifaddrs.h>
#include <netdb.h>
#include <cstring>

void print_addresses()
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	// Walk through the linked list of interfaces
	for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == nullptr)
			continue;

		family = ifa->ifa_addr->sa_family;
		if (family == AF_INET || family == AF_INET6)
		{
			s = getnameinfo(ifa->ifa_addr,
							(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
							host,
							NI_MAXHOST,
							nullptr,
							0,
							NI_NUMERICHOST);
			if (s != 0)
			{
				std::cout << "getnameinfo() failed: " << gai_strerror(s) << '\n';
				continue;
			}

			std::cout << ifa->ifa_name << " " << ((family == AF_INET) ? "IPv4" : "IPv6") << " " << host << '\n';
		}
	}

	freeifaddrs(ifaddr);
}

int main()
{
	print_addresses();
	return 0;
}
