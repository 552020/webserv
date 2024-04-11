#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "ARequestHandler.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "Environment.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>

class CGIHandler : public ARequestHandler
{
public:
	CGIHandler();
	virtual ~CGIHandler();
	std::string handleRequest(const HTTPRequest &request); // Adjusted to return std::string
	std::string executeCGI(const char *argv[], const Environment &env);

private:
	CGIHandler(const CGIHandler &other);
	CGIHandler &operator=(const CGIHandler &other);
};

#endif