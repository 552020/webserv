#include "server_utils.hpp"
#include <sstream>	// std::istringstream
#include <iostream> // std::cerr
#include "Connection.hpp"

void perrorAndExit(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

char customToLower(char c)
{
	// Check if c is uppercase (A-Z)
	if (c >= 'A' && c <= 'Z')
	{
		// Convert to lowercase
		return c + 32;
	}
	// Return the character unchanged if not uppercase
	return c;
}

// size_t getContentLength(const std::string &headers)
// {
// 	std::string lowerHeaders;
// 	for (std::string::const_iterator it = headers.begin(); it != headers.end(); ++it)
// 	{
// 		lowerHeaders += customToLower(*it);
// 	}

// 	std::string search = "content-length: ";
// 	std::string::size_type pos = lowerHeaders.find(search);
// 	if (pos != std::string::npos)
// 	{
// 		std::string contentLengthLine = headers.substr(pos + search.size());
// 		std::string::size_type endPos = contentLengthLine.find("\r\n");
// 		std::string contentLengthStr = contentLengthLine.substr(0, endPos);

// 		// Convert content length string to size_t
// 		std::istringstream iss(contentLengthStr);
// 		size_t contentLength;
// 		if (!(iss >> contentLength))
// 		{
// 			std::cerr << "Failed to convert content length to size_t\n";
// 			return 0; // Or use another way to indicate an error
// 		}
// 		return contentLength;
// 	}
// 	return 0;
// }

void printVariablesHeadersBody(const HTTPRequest &obj)
{
	std::multimap<std::string, std::string> a = obj.getHeaders();
	std::multimap<std::string, std::string> b = obj.getQueryString();
	std::vector<std::string> c = obj.getBody();

	std::multimap<std::string, std::string>::iterator it;
	std::cout << "Variables: =>" << std::endl;
	for (it = b.begin(); it != b.end(); it++)
	{
		std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
	}
	std::cout << "Headers: =>" << std::endl;
	for (it = a.begin(); it != a.end(); it++)
	{
		std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
	}
	std::cout << "Body: =>" << std::endl;
	for (size_t i = 0; i < c.size(); ++i)
		std::cout << c[i] << std::endl;
}

void printFDsVector(const std::vector<pollfd> &fds)
{
	std::cout << "printFDsVector" << std::endl;
	std::cout << "fds.size(): " << fds.size() << std::endl;
	std::cout << "_FDs: =>" << std::endl;
	for (size_t i = 0; i < fds.size(); ++i)
	{
		std::cout << "fd: " << fds[i].fd << ", events: " << fds[i].events << ", revents: " << fds[i].revents
				  << std::endl;
	}
}

void print_connectionsVector(const std::vector<Connection> &connections)
{
	std::cout << "print_connectionsVector" << std::endl;
	std::cout << "connections.size(): " << connections.size() << std::endl;
	std::cout << "connections: =>" << std::endl;
	for (size_t i = 0; i < connections.size(); ++i)
	{
		std::cout << "fd: " << connections[i].getPollFd().fd << std::endl;
		std::cout
			<< "We don't print anymore the header and the body here because they have been moved to the Parser class"
			<< std::endl;
	}
}