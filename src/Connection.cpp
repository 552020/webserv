#include "Connection.hpp"

Connection::Connection(struct pollfd &pollFd, Server &server)

{
	(void)server;
	_pollFd.fd = pollFd.fd;
	_pollFd.events = POLLIN;
	_pollFd.revents = 0;
	// TODO: should I initialize the _response here?
	//_response = HTTPResponse();
	//_request = HTTPRequest();
	//_parser = Parser();
	std::cout << "Connection created" << std::endl;
	std::cout << "pollFd.fd: " << _pollFd.fd << std::endl;
	std::cout << "pollFd.events: " << _pollFd.events << std::endl;
	std::cout << "pollFd.revents: " << _pollFd.revents << std::endl;
}

Connection::Connection(const Connection &other)
{
	_pollFd = other._pollFd;
	_response = other._response;

	std::cout << "Connection object copied" << std::endl;
}

Connection &Connection::operator=(const Connection &other)
{
	if (this != &other)
	{
		_pollFd = other._pollFd;
		_response = other._response;
	}
	std::cout << "Connection object assigned" << std::endl;
	return *this;
}

Connection::~Connection()
{
	std::cout << "Connection object destroyed" << std::endl;
	// Should I close the file descriptor here?
	// This was causing errors, cause when exiting from scope of the function where the Connection object was creted it
	// was closing the file descriptor. When we push back the Connection object in the vector of connections we make a
	// copy of the object, so the file descriptor was closed in the original object and the copy was left with an
	// invalid file descriptor. if (_pollFd.fd != -1)
	// {
	// 	close(_pollFd.fd);
	// 	_pollFd.fd = -1; // Invalidate the file descriptor to prevent double closing
	// }
}

struct pollfd Connection::getPollFd() const
{
	return _pollFd;
}

HTTPResponse &Connection::getResponse()
{
	return _response;
}
HTTPRequest &Connection::getRequest()
{
	return _request;
}

Parser &Connection::getParser()
{
	return _parser;
}
// Attempts to read HTTP request headers from the client connection into _headersBuffer on the Parser.
bool Connection::readSocket(Parser &parser)
{
	std::cout << "\nEntering readSocket" << std::endl;
	char buffer[BUFFER_SIZE] = {0};
	std::cout << "buffers size: " << sizeof(buffer) << std::endl;
	ssize_t bytesRead = recv(_pollFd.fd, buffer, BUFFER_SIZE, 0);
	std::cout << "bytesRead: " << bytesRead << std::endl;
	if (bytesRead > 0)
	{
		parser.setBuffer(parser.getBuffer() + std::string(buffer, bytesRead));
		std::cout << "The buffer is: " << parser.getBuffer() << std::endl;

		std::cout << "Exiting readSocket" << std::endl;
		return true;
	}
	else if (bytesRead < 0)
	{
		perror("recv failed");
		return false;
	}
	else
	{
		std::cout << "Connection closed before headers being completely sent" << std::endl;
		return false;
	}
	std::cout << "Exiting readSocket. This will never happen here!" << std::endl;
	return true;
}
// About the hexa conversion
// Convert the hexadecimal string from `chunkSizeLine` to a size_t value.
// Here, `std::istringstream` is used to create a stream from the string,
// which then allows for input operations similar to cin. The `std::hex`
// manipulator is used to interpret the input as a hexadecimal value.
// We attempt to stream the input into the `chunkSize` variable. If this operation
// fails (e.g., because of invalid input characters that can't be interpreted as hex),
// the stream's failbit is set, and the conditional check fails. In this case,
// we return false indicating an error in parsing the chunk size.

bool Connection::readChunkedBody(Parser &parser)
{
	// TODO: check if this is blocking; I mean the two recvs in readChunkSize and readChunk
	if (!parser.getBodyComplete())
	{
		std::string chunkSizeLine;
		// readChiunkSize cuould be a method of Connection, now it's a free function.
		if (!readChunkSize(chunkSizeLine))
			return false;

		std::istringstream iss(chunkSizeLine);
		size_t chunkSize;
		if (!(iss >> std::hex >> chunkSize))
			return false;

		if (chunkSize == 0)
		{
			parser.setBodyComplete(true);
			return true;
		}
		else
		{
			std::string chunkData;
			if (!readChunk(chunkSize, chunkData, _response))
				return false;
			parser.setBuffer(parser.getBuffer() + chunkData);
		}
	}
	return false;
}

bool Connection::readChunkSize(std::string &line)
{
	// TODO: check this while loop that could be infinite
	// TODO: check if this is blocking

	line.clear();
	while (true)
	{
		char buffer;
		ssize_t bytesRead = recv(_pollFd.fd, &buffer, 1, 0);
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

bool Connection::readChunk(size_t chunkSize, std::string &chunkData, HTTPResponse &response)
{
	// Reserve space in the string to avoid reallocations
	chunkData.reserve(chunkSize + chunkData.size());
	while (chunkSize > 0)
	{
		char buffer[BUFFER_SIZE];
		size_t bytesToRead = std::min(chunkSize, (size_t)BUFFER_SIZE);
		ssize_t bytesRead = recv(_pollFd.fd, buffer, bytesToRead, 0);
		if (bytesRead > 0)
		{
			chunkData.append(buffer, bytesRead);
			chunkSize -= bytesRead;
		}
		else if (bytesRead < 0)
		{
			perror("recv failed in readChunk");
			// Internal Server Error
			response.setStatusCode(500, "");
			return false;
		}
		else
		{
			// bytes read == 0, connection closed prematurely
			response.setStatusCode(400, "Connection closed while reading chunk"); // Bad Request
			return false;
		}
	}
	char crlf[2];
	ssize_t crlfRead = recv(_pollFd.fd, crlf, 2, 0);
	if (crlfRead < 2)
	{
		response.setStatusCode(400, "Connection closed while reading CRLF"); // Bad Request
		return false;
	}
	return true;
}

bool Connection::readBody(Parser &parser, HTTPRequest &req, HTTPResponse &res)
{
	std::cout << "\nEntering readBody" << std::endl;
	size_t contentLength = req.getContentLength();
	char buffer[BUFFER_SIZE];
	// We could also use _bodyTotalBytesRead from the parser
	size_t bytesRead = parser.getBuffer().size();
	std::cout << "contentLength: " << contentLength << std::endl;
	std::cout << "bytesRead: " << bytesRead << std::endl;
	if (bytesRead < contentLength)
	{
		// TODO: check if this is blocking
		ssize_t read = recv(_pollFd.fd, buffer, BUFFER_SIZE, 0);
		if (read > 0)
		{
			std::cout << "read > 0" << std::endl;
			// _body.append(buffer, read);j
			parser.setBuffer(parser.getBuffer() + std::string(buffer, read));
			std::cout << "The 'body; is: " << parser.getBuffer() << std::endl;
			bytesRead += read;
			if (bytesRead == contentLength)
			{
				parser.setBodyComplete(true);
				return true;
			}
		}
		else if (read < 0)
		{
			perror("recv failed");
			res.setStatusCode(500, ""); // Internal Server Error
			return false;
		}
		else
		{
			std::cout << "read == 0" << std::endl;
			res.setStatusCode(499, "Connection closed by the client"); // Client Closed Request
			return false;
		}
	}
	else
		parser.setBodyComplete(true);
	std::cout << "Exiting readBody" << std::endl;
	return true;
}

/* Debugging */

void Connection::printConnection() const
{
	std::cout << "\nprintConnection" << std::endl;
	std::cout << "Connection: " << _pollFd.fd << std::endl;
}