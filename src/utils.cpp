#include <cstdlib> // For exit() and EXIT_FAILURE
#include <cstring> // For memset
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <sys/socket.h> // For socket functions
#include <unistd.h>		// For read, write, and close
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include "webserv.hpp"
#include <list>
#include <algorithm>

std::string readHtml(const std::string &filePath)
{
	std::ifstream file(filePath.c_str());
	if (!file.is_open())
	{
		std::cerr << "Could not open file: " << filePath << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

char *ft_strcpy(char *dest, const char *src)
{
	int i;

	i = 0;
	if (!dest || !src)
		return (NULL);
	while (dest[i] && src[i])
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
	return (dest);
}

int hexToInt(std::string hex)
{
	int n;

	std::stringstream ss;
	ss << std::hex << hex;
	ss >> n;
	if (ss.fail())
		return (-1);
	return (n);
}

bool	isNumber(std::string line)
{
	for (unsigned int i = 0; i < line.length(); ++i){
		if (!(line[i] >= '0' && line[i] <= '9'))
			return (false);
	}
	return (true);
}

bool	isValidErrorCode(std::string errorCode)
{
	std::string error_codes[] = {
        // 4xx Client Error
        "400", "401", "402", "403", "404", "405", "406", "407",
        "408", "409", "410", "411", "412", "413", "414", "415",
        "416", "417", "418", "421", "422", "423", "424", "426",
        "428", "429", "431", "451",

        // 5xx Server Error
        "500", "501", "502", "503", "504", "505", "506", "507",
        "508", "510", "511"
    };
	std::list<std::string> validCodes(error_codes, error_codes + sizeof(error_codes) / sizeof(error_codes[0]));
	if (std::find(validCodes.begin(), validCodes.end(), errorCode) != validCodes.end())
		return (true);
	return (false);
}

int	checkFile(const char *path)
{
	char	buffer[2];

	int	fd = open(path, O_RDONLY);
	if (fd == -1)
		return (-1);
	if (read(fd, buffer, 0) == -1){
		close(fd);
		return (-1);
	}
	return (fd);
}