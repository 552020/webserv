#include <cstdlib> // For exit() and EXIT_FAILURE
#include <cstring> // For memset
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <sys/socket.h> // For socket functions
#include <unistd.h>     // For read, write, and close
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include "webserv.hpp"

std::string readHtml(const std::string& filePath) {
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void *ft_memset(void *ptr, int value, size_t num)
{
	// Cast the pointer to a char pointer, as we're dealing with bytes
	unsigned char *p = static_cast<unsigned char *>(ptr);

	// Fill the specified memory area with the given value
	for (size_t i = 0; i < num; ++i)
	{
		p[i] = static_cast<unsigned char>(value);
	}

	// Return the original pointer
	return ptr;
}

char *ft_strcpy(char *dest, const char *src)
{
	int	i;

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
	int	n;

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

int	checkFile(const char *path)
{
	char	buffer[2];

	int	fd = open(path, O_RDONLY);
	if (fd == -1)
		return (-1);
	if (read(fd, buffer, 0) < 1){
		close(fd);
		return (-1);
	}
	return (fd);
}