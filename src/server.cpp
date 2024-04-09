#include "Server.hpp"

bool MyReadLine(int socket, std::string &line)
{
	line.clear();
	while (true)
	{
		char buffer;
		ssize_t bytesRead = recv(socket, &buffer, 1, 0);
		if (bytesRead > 0)
		{
			line.push_back(buffer);
			if (line.size() >= 2 && line.substr(line.size() - 2) == "\r\n")
			{
				line.resize(line.size() - 2); // remove the CRLF
				return true;
			}
		}
		else if (bytesRead < 0)
		{
			perror("recv failed");
			return false;
		}
		else
		{
			std::cout << "Connection closed" << std::endl;
			return false;
		}
	}
	return true;
}

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

// // Determine the type of request and call the appropriate handler
// void handleConnection(int socket)
// {
// 	char buffer[BUFFER_SIZE] = {0};
// 	long valRead = read(socket, buffer, BUFFER_SIZE);
// 	HTTPRequest obj(buffer);
// 	if (obj.getIsChunked() == true)
// 		obj.parseChunkedBody(buffer);
// 	std::cout << obj.getStatusCode() << std::endl;
// 	printVariablesHeadersBody(obj);
// 	if (valRead < 0)
// 	{
// 		perror("In read");
// 		exit(EXIT_FAILURE);
// 	}
// 	std::cout << "Received HTTP request: " << std::endl << buffer << std::endl;

// 	// test to execute the python script (see: https://www.tutorialspoint.com/python/python_cgi_programming.htm)
// 	const char *argv[] = {"./cgi-bin/hello_py.cgi", NULL};
// 	// const char* argv[] = { "./cgi-bin/thirty_py.cgi", NULL };
// 	// const char* argv[] = { "./cgi-bin/hello.cgi", NULL };

// 	// std::string response;
// 	HTTPResponse response;
// 	Router router;
// 	if (!router.pathExists(response, obj.getRequestTarget()))
// 	{
// 		StaticContentHandler staticContentHandler;
// 		// This shoud be a method of the requestHandler obect
// 		// response = router.handleNotFound();
// 		response = staticContentHandler.handleNotFound();
// 	}
// 	else if (router.isDynamicRequest(obj))
// 	{
// 		if (obj.getMethod() == "GET" && obj.getRequestTarget() == "/hello")
// 		{
// 			// env has to be created before CGI, because it is passed to the CGI
// 			CGIHandler cgiHandler;
// 			Environment env;
// 			env.setVar("QUERY_STRING", "Hello from C++ CGI!");
// 			response = cgiHandler.handleCGIRequest(argv, env);
// 		}
// 		else
// 		{
// 			CGIHandler cgiHandler;
// 			Environment env;
// 			env.setVar(obj.getQueryString(), obj.getBody());
// 			response = cgiHandler.handleCGIRequest(argv, obj);
// 		}
// 	}
// 	else
// 	{
// 		StatcContentHandler staticContentHandler;
// 		// This if condition only for legacy reasons! TODO: remove
// 		if (obj.getMethod() == "GET" && (obj.getRequestTarget() == "/" || obj.getRequestTarget() == "/home")
// 		{
// 			response = staticContentHandler.handleHomePage();

// 		}
// 		else
// 		{
// 			response = staticContentHandler.handleRequest(obj);
// 		}
// 	}
// 	std::string responseString = response.toString();

// 	write(socket, responseString.c_str(), responseString.size());
// 	close(socket);
// }

// Default constructor
Server::Server()
{
	loadDefaultConfig();
}
// Constructor with config file path
Server::Server(const std::string configFilePath) : _configFilePath(configFilePath)
{
	loadDefaultConfig();
	loadConfig();
}
// Destructor
Server::~Server()
{
}

// Start listening
void Server::startListen()
{

	// Server socket is blocking by default, which should not matter since we are using poll
	if ((_serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("In socket");
		// TODO: consider what to do here. Not sure we want to exit the program.
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	// Set SO_REUSEADDR to allow re-binding to the same address and port
	if (setsockopt(_serverFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		// TODO: consider what to do here. Not sure we want to exit the program.
		exit(EXIT_FAILURE);
	}
	if (setsockopt(_serverFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt SO_REUSEPORT: Protocol not available, continuing without SO_REUSEPORT");
		// Don't exit on failure; it's not critical for basic server functionality
		// Setting SO_REUSEPORT is not supported on all systems and setting it on the same call was causing the server
		// to fail
	}
	// We bind the server to the address and port
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_addr.s_addr = INADDR_ANY;
	_serverAddr.sin_port = htons(_port);
	ft_memset(_serverAddr.sin_zero, '\0', sizeof _serverAddr.sin_zero);

	if (bind(_serverFD, (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
	{
		perror("In bind");
		// TODO: consider what to do here. Not sure we want to exit the program.
		exit(EXIT_FAILURE);
	}
	// 10 is the maximum size of the queue of pending connections: check this value.
	if (listen(_serverFD, 10) < 0)
	{
		perror("In listen");
		// TODO: consider what to do here. Not sure we want to exit the program.
		exit(EXIT_FAILURE);
	}
}

void Server::startPollEventLoop()
{
	// Set up the pollfd structure for the server socket
	struct pollfd serverFdPoll;
	serverFdPoll.fd = _serverFD;
	serverFdPoll.events = POLLIN;
	serverFdPoll.revents = 0;
	_FDs.push_back(serverFdPoll);
	while (1)
	{
		std::cout << "++++++++++++++ Waiting for new connection +++++++++++++++" << std::endl;
		int ret = poll(_FDs.data(), _FDs.size(), -1); // -1 means wait indefinitely
		// _FDs.data() returns a pointer to the underlying array of pollfd structures
		// if the server socket is readable, then a new connection is available
		if (ret > 0)
		{
			for (size_t i = 0; i < _FDs.size(); i++)
			{
				if (_FDs[i].revents & POLLIN)
				{
					// Check if the server socket is readable: fds[0] is the server socket
					if (i == 0)
					{
						struct sockaddr_in address;
						socklen_t addrLen = sizeof(address);
						std::cout << "New connection detected" << std::endl;
						// accept is blocking but we know that the server socket is readable
						int new_socket = accept(_serverFD, (struct sockaddr *)&address, (socklen_t *)&addrLen);
						if (new_socket >= 0)
						{
							struct pollfd newSocketPoll;
							newSocketPoll.fd = new_socket;
							newSocketPoll.events = POLLIN;
							newSocketPoll.revents = 0;
							_FDs.push_back(newSocketPoll);
							// We can log the address of the newly connected client
							// char clientIP[INET_ADDRSTRLEN];
							// inet_ntop(AF_INET, &address.sin_addr, clientIP, INET_ADDRSTRLEN);
							// std::cout << "New connection from " << clientIP << std::endl;
						}
						else
						{
							perror("In accept");
							// TODO: consider what to do here. Not sure we want to exit the program.
						}
					}
					else
					{
						handleConnection(_FDs[i].fd);
						// Remove the client socket from the pollfd structure
						_FDs.erase(_FDs.begin() + i);
						// Handle incoming data
						// Add logic to handle incoming data
					}
				}
				else if (_FDs[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					if (i == 0)
					{
						// Handle the server socket error
						// Log the error if necessary
						// Attempt recovery or initiate a graceful shutdown
						// Possibly alert administrators
						perror("poll server socket error");
						// exit(EXIT_FAILURE);
					}
					else
					{
						// Handle the client socket error
						close(_FDs[i].fd);
						_FDs.erase(_FDs.begin() + i);
						perror("poll client socket error");
						--i; // Adjust the index to account for the removed element
							 // exit(EXIT_FAILURE);
					}
				}
			}
		}
		else if (ret == 0)
		{
			std::cout << "Timeout occurred!" << std::endl; // This should never happen with an infinite timeout
		}
		else
		{
			// handle the errors on poll on the server socket
			if (errno == EINTR)
			{
				// poll was interrupted by a signal
				// Log the interruption if necessary
				continue; // Retry the poll operation
			}
			else
			{
				// poll failes: EBADF, EFAULT, EINVAL, ENOMEM
				// Log critical error details
				// Attempt recovery or initiate a graceful shutdown
				// Possibly alert administrators
				perror("poll");
				// exit(EXIT_FAILURE);
			}
		}
	}
}

void Server::handleConnection(int clientFD)
{
	std::string headers;
	size_t totalRead = 0;
	bool headersComplete = false;
	while (!headersComplete)
	{
		// We reinitialize it at each iteration to have a clean buffer
		char buffer[BUFFER_SIZE] = {0};
		// we could do recv non blocking with MSG_DONTWAIT but we will keep it simple for now
		ssize_t bytesRead = recv(clientFD, buffer, BUFFER_SIZE, 0);
		if (bytesRead > 0)
			headers.append(buffer, bytesRead);
		totalRead += bytesRead;
		if (totalRead > MAX_HEADER_SIZE)
		{
			std::cout << "Header too large" << std::endl;
			// This would be a 413 Payload Too Large in a real server
			close(clientFD);
			return;
		}
		if (headers.find("\r\n\r\n") != std::string::npos)
			headersComplete = true;
		else if (bytesRead < 0)
		{
			perror("recv failed");
			close(clientFD);
			return;
			// We could eventually also brake the loop and retry the recv
			// break;
			// We could eventually retry the recv, but for now we will just close the connection
			// Handle error
		}
		else
		{
			// This means biyeRead == 0
			std::cout << "Connection closed" << std::endl;
			close(clientFD);
			return;
		}
	}
	std::string body;
	if (isChunked(headers))

	{
		// TODO: check if this is blocking; I mean the recv in readChunk
		bool bodyComplete = false;
		while (!bodyComplete)
		{
			// Chunk Structure: size in hex, CRLF, chunk data, CRLF
			// chunkSizeLine will contain the size of the next chunk in hexadecimal
			std::string chunkSizeLine;
			// Read the line containing the size of the next chunk
			MyReadLine(clientFD, chunkSizeLine);
			// We transform the size from hexadecimal to an integer
			size_t chunkSize = std::stoul(chunkSizeLine, 0, 16);

			if (chunkSize == 0)
			{
				bodyComplete = true;
			}
			else
			{
				// Read the chunk of data
				std::string chunkData = readChunk(clientFD, chunkSize); // Implement this function
				body.append(chunkData);
				// Consume the CRLF at the end of the chunk
			}
			// Now, body contains the full body of the request
		}
	}
	else
	{
		size_t contentLength = getContentLength(headers);
		char buffer[BUFFER_SIZE];
		size_t bytesRead = 0;
		while (bytesRead < contentLength)
		{
			// TODO: check if this is blocking
			ssize_t read = recv(clientFD, buffer, BUFFER_SIZE, 0);
			if (read > 0)
			{
				body.append(buffer, read);
				bytesRead += read;
			}
			else if (read < 0)
			{
				perror("recv failed");
				// Consiger if we should close the clientFD or retry the recv
				close(clientFD);
				return;
			}
			else
			{
				std::cout << "Connection closed" << std::endl;
				close(clientFD);
				return;
			}
		}
	}
	// It should be double "\r\n" to separate the headers from the body
	std::string httpRequestString = headers + "\r\n\r\n" + body;

	HTTPRequest obj(httpRequestString.c_str());
	// HTTPRequest obj(buffer);
	std::cout << obj.getStatusCode() << std::endl;
	std::cout << "Received HTTP request: " << std::endl << httpRequestString << std::endl;

	// test to execute the python script (see: https://www.tutorialspoint.com/python/python_cgi_programming.htm)
	const char *argv[] = {"./cgi-bin/hello_py.cgi", NULL};
	// const char* argv[] = { "./cgi-bin/thirty_py.cgi", NULL };
	// const char* argv[] = { "./cgi-bin/hello.cgi", NULL };

	// std::string response;
	HTTPResponse response;
	Router router;
	if (!router.pathExists(response, obj.getRequestTarget()))
	{
		StaticContentHandler staticContentHandler;
		// This shoud be a method of the requestHandler obect
		// response = router.handleNotFound();
		response = staticContentHandler.handleNotFound();
	}
	else if (router.isDynamicRequest(obj))
	{
		if (obj.getMethod() == "GET" && obj.getRequestTarget() == "/hello")
		{
			// env has to be created before CGI, because it is passed to the CGI
			CGIHandler cgiHandler;
			Environment env;
			env.setVar("QUERY_STRING", "Hello from C++ CGI!");
			// cgiHandler.executeCGI(argv, env);
			handleCGIRequest(argv, env);
		}
		else
		{
			CGIHandler cgiHandler;
			Environment env;
			env.setVar("obj.getQueryString()", "obj.getBody()");
			// response = cgiHandler.handleCGIRequest(argv, obj);
			// cgiHandler.executeCGI(argv, env);
			handleCGIRequest(argv, env);
		}
	}
	else
	{
		StaticContentHandler staticContentHandler;
		// This if condition only for legacy reasons! TODO: remove
		if (obj.getMethod() == "GET" && (obj.getRequestTarget() == "/" || obj.getRequestTarget() == "/home"))
		{
			response = staticContentHandler.handleHomePage();
		}
		else
		{
			response = staticContentHandler.handleRequest(obj);
		}
	}
	std::string responseString = response.toString();

	write(clientFD, responseString.c_str(), responseString.size());
	close(clientFD);
}

// Server Server::getServer()
// {
// 	return *this;
// }

void Server::loadDefaultConfig()
{
	_webRoot = "var/www";
}

std::string Server::getWebRoot() const
{
	return _webRoot;
}

void Server::setWebRoot(const std::string &webRoot)
{
	_webRoot = webRoot;
}

std::string Server::getConfigFilePath() const
{
	return _configFilePath;
}

void Server::loadConfig()
{
	// Add logic to load config from file
}

void Server::start()
{
	// Add logic to start the server
}

void Server::stop()
{
	// Add logic to stop the server
}