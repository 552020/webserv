#include "Server.hpp"
#include "Parser.hpp"
#include "Connection.hpp"

Server::Server()
{
	loadDefaultConfig();
}

Server::Server(const Config &config)
{
	_config = config;
	// while we don't have a config file
	loadDefaultConfig();
}

Server::~Server()
{
}

void Server::startListening()
{

	createServerSocket();
	setReuseAddrAndPort();
	checkSocketOptions();
	bindToPort(_port);
	listen();
}

void Server::startPollEventLoop()
{
	addServerSocketPollFdToVectors();
	int pollCounter = 0;
	while (1)
	{
		// printConnections("BEFORE POLL", _FDs, _connections, true);
		std::cout << CYAN << "++++++++++++++ #" << pollCounter
				  << " Waiting for new connection or Polling +++++++++++++++" << RESET << std::endl;
		int ret = poll(_FDs.data(), _FDs.size(), -1);
		pollCounter++;
		// printFrame("POLL EVENT DETECTED", true);
		// printConnections("AFTER POLL", _FDs, _connections, true);
		if (ret > 0)
		{
			size_t originalSize = _FDs.size();
			// if _FDs becomes bigger than originalSize we don't want to loop over the new elements before we finish the
			// old ones if _FDs becomes smaller than originalSize we don't want to loop over the old elements that are
			// not in _FDs anymore
			for (size_t i = 0; i < originalSize && i < _FDs.size(); i++)
			{
				if (_FDs[i].revents & (POLLIN | POLLOUT))
				{
					Debug::log("Enters revents", Debug::OCF);
					if (i == 0)
					{
						// printFrame("SERVER SOCKET EVENT", true);
						acceptNewConnection(_connections[i]);
					}
					else
					{
						// printFrame("CLIENT SOCKET EVENT", true);
						handleConnection(_connections[i],
										 i,
										 _connections[i].getParser(),
										 _connections[i].getRequest(),
										 _connections[i].getResponse());
						if (_connections[i].getHasFinishedReading() && _connections[i].getHasDataToSend())
							_FDs[i].events = POLLOUT;
					}
				}
				else if (_FDs[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					if (i == 0)
						handleServerSocketError();
					else
						handleClientSocketError(_FDs[i].fd, i);
				}
			}
		}
		else if (ret == 0)
			handleSocketTimeoutIfAny();
		else
			handlePollError();
	}
}

void Server::readFromClient(Connection &conn, size_t &i, Parser &parser, HTTPRequest &request, HTTPResponse &response)
{
	(void)i;
	Debug::log("\033[1;33mEntering read from client\033[0m", Debug::OCF);
	// TODO: change to _areHeadersCopmplete
	if (!parser.getHeadersComplete())
	{
		Debug::log("\033[1;33mReading headers\033[0m", Debug::NORMAL);
		if (!conn.readHeaders(parser))
		{
			Debug::log("Error reading headers", Debug::OCF);
			conn.setHasFinishedReading(true);
			conn.setHasDataToSend(false);
			conn.setCanBeClosed(true);
			return;
		}
		conn.setHasReadSocket(true);
		if (!parser.preParseHeaders(response))
		{
			conn.setCanBeClosed(true);
			conn.setHasFinishedReading(true);
			conn.setHasDataToSend(true);
			Debug::log("Error pre-parsing headers", Debug::OCF);
			return;
		}
	}
	if (!parser.getHeadersComplete())
	{
		Debug::log("Headers incomplete yet, exiting readFromClient.", Debug::NORMAL);
		return;
	}
	if (parser.getHeadersComplete() && !parser.getHeadersAreParsed())
		parser.parseRequestLineAndHeaders(parser.getHeadersBuffer().c_str(), request, response);
	if (response.getStatusCode() != 0)
	{
		conn.setCanBeClosed(false);
		conn.setHasFinishedReading(true);
		conn.setHasDataToSend(false);
		Debug::log("Error parsing headers or request line", Debug::OCF);
		return;
	}
	std::cout << parser.getHeadersComplete() << " ," << request.getMethod() << std::endl;
	if (parser.getHeadersComplete() && request.getMethod() == "GET")
		conn.setHasFinishedReading(true);

	if (response.getStatusCode() != 0)
		Debug::log(toString(response.getStatusCode()), Debug::NORMAL);
	if (request.getMethod() == "GET")
		Debug::log("GET request, no body to read", Debug::NORMAL);
	else
	{
		if (parser.getIsChunked() && !conn.getHasReadSocket())
		{
			Debug::log("Chunked body", Debug::NORMAL);
			if (!conn.readChunkedBody(parser))
			{
				// Case of error while reading chunked body
				Debug::log("Error reading chunked body", Debug::OCF);
				conn.setCanBeClosed(true);
				conn.setHasFinishedReading(true);
				// It could be that we had data that could be sent even if we have an error cause previous data was read
				return;
			}
			conn.setHasReadSocket(true);
		}
		else if (!conn.getHasReadSocket())
		{
			Debug::log("\033[1;33mReading body\033[0m", Debug::NORMAL);
			// TODO: add comments
			if (!parser.getBodyComplete() && parser.getBuffer().size() == request.getContentLength())
			{
				// TODO: in the new design we will return here and go to the function where the response is
				parser.setBodyComplete(true);
				conn.setHasFinishedReading(true);
				conn.setHasDataToSend(true);
			}
			else if (!conn.getHasReadSocket() && !conn.readBody(parser, request, response))
			{
				Debug::log("Error reading body", Debug::OCF);
				conn.setCanBeClosed(true);
				conn.setHasFinishedReading(true);
				// Probably hasDataToSend false, because we have an error on reading the body
				// conn.setHasDataToSend(false);
				return;
			}
		}
		if (!parser.getBodyComplete() && request.getContentLength() != 0 &&
			parser.getBuffer().size() == request.getContentLength())
		{
			// TODO: in the new design we will return here and go to the function where the response is
			parser.setBodyComplete(true);
			conn.setHasFinishedReading(true);
			conn.setCanBeClosed(false);
			conn.setHasDataToSend(false);
			return;
		}
		if (!parser.getBodyComplete())
		{
			Debug::log("Body still incomplete, exiting readFromClient.", Debug::NORMAL);
			conn.setHasFinishedReading(false);
			conn.setHasReadSocket(true);
			return;
		}
		//  std::cout << parser.getBuffer() << std::endl;
		if (!request.getUploadBoundary().empty())
			parser.parseFileUpload(parser.getBuffer(), request, response);
		else if (request.getMethod() != "GET")
		{
			request.setBody(parser.getBuffer());
			conn.setHasFinishedReading(true);
		}
	}
}

void Server::accessThroughLocalhost(HTTPRequest &request)
{
	std::cout << std::endl;
	Debug::log("Entering accessThroughLocalhost", Debug::NORMAL);

	std::string host = request.getSingleHeader("host").second;
	std::cout << "Request host: " << host << std::endl;

	std::string requestTarget = request.getRequestTarget();
	std::cout << "Request target: " << requestTarget << std::endl;

	std::string host1 = "/www.development_site.com";
	std::string host2 = "/www.example.com";
	std::string host3 = "/www.saladbook.xyz";
	std::vector<std::string> realHosts;
	realHosts.push_back(host1);
	realHosts.push_back(host2);
	realHosts.push_back(host3);

	// we identify the website from the realHosts, we erase it from the request target and set it as the host
	for (size_t i = 0; i < realHosts.size(); i++)
	{
		std::cout << "Checking host: " << realHosts[i] << std::endl;
		if (requestTarget.find(realHosts[i]) != std::string::npos)
		{
			std::cout << "Found host: " << realHosts[i] << std::endl;
			// remove the "/" from the host
			std::string hostToSet = realHosts[i].erase(0, 1);
			request.setHost(hostToSet);
			std::string newRequestTarget = request.getRequestTarget();
			std::cout << BLUE << "Request target: " << RESET << newRequestTarget << std::endl;
			// erase first char of requestTarget
			newRequestTarget.erase(0, 1);
			std::cout << BLUE << "Request target after erase: " << RESET << newRequestTarget << std::endl;
			request.setRequestTarget(newRequestTarget.erase(newRequestTarget.find(hostToSet), hostToSet.size()));

			break;
		}
	}
	Debug::log("Request host end of accessThroughLocalhost: " + request.getSingleHeader("host").second, Debug::NORMAL);
	Debug::log("Request target end of accessThroughLocalhost: " + request.getRequestTarget(), Debug::NORMAL);
	Debug::log("Exiting accessThroughLocalhost", Debug::NORMAL);
	std::cout << std::endl;
}

void Server::buildResponse(Connection &conn, size_t &i, HTTPRequest &request, HTTPResponse &response)
{
	(void)i;
	Debug::log("Entering buildResponse", Debug::NORMAL);
	Debug::log("Request method: " + request.getMethod(), Debug::NORMAL);

	accessThroughLocalhost(request);

	ServerBlock serverBlock;
	std::cout << GREEN << "Number of server blocks: " << _config.getServerBlocks().size() << RESET << std::endl;
	if (_config.getServerBlocks().size() > 1)
	{
		// retrieve the server block which has a server name matching the request host header
		for (size_t i = 0; i < _config.getServerBlocks().size(); i++)
		{
			// why getServerName returns a vector ?
			std::string serverName = _config.getServerBlocks()[i].getServerName()[0];
			std::cout << RED << "Checking server name: " << serverName << RESET << std::endl;
			std::string requestTarget = request.getRequestTarget();
			std::cout << "Request target: " << requestTarget << std::endl;
			std::cout << "Request host: " << request.getSingleHeader("host").second << std::endl;
			if (serverName == request.getSingleHeader("host").second)
			{
				std::cout << GREEN << "Server block and request host match" << RESET << std::endl;
				// _config.setServerBlockIndex(i);
				serverBlock = _config.getServerBlocks()[i];
				break;
			}
			else
			{
				// if no server name is found, use the default server block
				static StaticContentHandler staticContentInstance;
				staticContentInstance.handleNotFound(response);
				response.setStatusCode(404, "No server block is matching the request host");
				conn.setHasDataToSend(true);
				Debug::log("Exiting buildResponse", Debug::NORMAL);
				return;
			}
			std::cout << "Index: " << i << std::endl;
		}
	}
	else
	{
		Debug::log("Single server block", Debug::NORMAL);
		serverBlock = _config.getServerBlocks()[0];
	}

	std::string root = serverBlock.getRoot();
	std::cout << RED << "Root: " << root << RESET << std::endl;

	Router router(serverBlock);

	if (response.getStatusCode() != 0)
	{
		Debug::log("Error response" + toString(response.getStatusCode()), Debug::NORMAL);
		response.setErrorResponse(response.getStatusCode());
		router.handleServerBlockError(request, response, response.getStatusCode());
		conn.setHasDataToSend(true);
		return;
	}
	else
	{
		router.setFDsRef(&_FDs);
		router.setPollFd(&conn.getPollFd());
		router.routeRequest(request, response);
	}
	// TODO: check if the listen in the server block is matching port and ip from connection
	conn.setHasDataToSend(true);
}

void Server::writeToClient(Connection &conn, size_t &i, HTTPResponse &response)
{
	std::cout << "\033[1;36m"
			  << "Entering writeToClient"
			  << "\033[0m" << std::endl;
	(void)i;
	std::string responseString = response.objToString();
	send(conn.getPollFd().fd, responseString.c_str(), responseString.size(), 0);
	// conn.setHasDataToSend(); will not be always false in case of chunked response or keep-alive connection
	conn.setHasDataToSend(false);
	conn.setHasFinishedSending(true);
	// setCanBeClosed(true) would not be the case only if we have a keep-alive connection or a chunked response
	conn.setCanBeClosed(true);
}

void Server::closeClientConnection(Connection &conn, size_t &i)
{
	std::cout << "\033[1;36m"
			  << "Entering closeClientConnection"
			  << "\033[0m" << std::endl;
	// if (response.getStatusCode() != 0)
	// if (conn.getResponse().getStatusCode() != 0 && conn.getResponse().getStatusCode() != 499)
	// {
	// 	std::string responseString = conn.getResponse().objToString();
	// 	send(conn.getPollFd().fd, responseString.c_str(), responseString.size(), 0);
	// }
	// TODO: should we close it with the Destructor of the Connection class?
	close(conn.getPollFd().fd);
	_FDs.erase(_FDs.begin() + i);
	_connections.erase(_connections.begin() + i);
	--i;
}

void Server::handleConnection(Connection &conn, size_t &i, Parser &parser, HTTPRequest &request, HTTPResponse &response)
{

	// conn.printConnection();

	// Why is it TRUE when I refresh a page?????
	conn.setHasReadSocket(false);
	std::cout << "Has finished reading: " << conn.getHasFinishedReading() << std::endl;
	if (!conn.getHasFinishedReading())
		readFromClient(conn, i, parser, request, response);
	// TODO: add comments to explain
	if (conn.getHasReadSocket() && !conn.getHasFinishedReading())
	{
		std::cout << "Has read socket: " << conn.getHasReadSocket() << std::endl;
		return;
	}
	// std::cout << request << std::endl;
	if (!conn.getCanBeClosed() && !conn.getHasDataToSend())
		buildResponse(conn, i, request, response);
	// std::cout << "Has data to send: " << conn.getHasDataToSend() << std::endl;
	// std::cout << response << std::endl;
	if (conn.getHasDataToSend())
		writeToClient(conn, i, response);
	// std::cout << "Has finished sending: " << conn.getHasFinishedSending() << std::endl;
	// std::cout << "Can be closed: " << conn.getCanBeClosed() << std::endl;
	if (conn.getCanBeClosed())
		closeClientConnection(conn, i);
}

/*** Private Methods ***/

void Server::loadConfig()
{
	// Add logic to load config from file
}

void Server::loadDefaultConfig()
{
	_webRoot = "var/www";
	_maxClients = 10;
	_clientMaxHeadersSize = CLIENT_MAX_HEADERS_SIZE;
	_clientMaxBodySize = CLIENT_MAX_BODY_SIZE;
	_port = 8080;
}

/* startListening */

void Server::createServerSocket()
{
	if ((_serverFD = socket(AF_INET, SOCK_STREAM, 0)) == 0)
		perrorAndExit("Failed to create server socket");
	// std::cout << "Server socket created" << std::endl;
	// std::cout << "Server socket file descriptor: " << _serverFD << std::endl;
}

void Server::setReuseAddrAndPort()
{
	int opt = 1;
	if (setsockopt(_serverFD, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		perror("setsockopt SO_REUSEADDR: Protocol not available, continuing without SO_REUSEADDR");
	if (setsockopt(_serverFD, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
		perror("setsockopt SO_REUSEPORT: Protocol not available, continuing without SO_REUSEPORT");
	// std::cout << "SO_REUSEADDR and SO_REUSEPORT set" << std::endl;
}

void Server::bindToPort(int port)
{
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_addr.s_addr = INADDR_ANY;
	_serverAddr.sin_port = htons(port);
	std::memset(_serverAddr.sin_zero, '\0', sizeof _serverAddr.sin_zero);

	if (bind(_serverFD, (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
		perrorAndExit("In bind");
	// std::cout << "Server socket bound to port " << port << std::endl;
	// std::cout << "Server socket address: " << inet_ntoa(_serverAddr.sin_addr) << std::endl;
	// std::cout << "Server socket port: " << ntohs(_serverAddr.sin_port) << std::endl;
}

void Server::listen()
{
	if (::listen(_serverFD, _maxClients) < 0)
		perrorAndExit("In listen");
	std::cout << "Server socket listening on port " << ntohs(_serverAddr.sin_port) << std::endl;
}

/* startPollEventsLoop */

void Server::addServerSocketPollFdToVectors()
{
	// In this function we also create the struct pollfd for the server socket
	struct pollfd serverPollFd;
	memset(&serverPollFd, 0, sizeof(serverPollFd));
	serverPollFd.fd = _serverFD;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	if (VERBOSE)
	{
		std::cout << "pollfd struct for Server socket created" << std::endl;
		std::cout << std::endl;
		std::cout << "Printing serverPollFd (struct pollfd) before push_back into _FDs" << std::endl;
		std::cout << "fd: " << serverPollFd.fd << ", events: " << serverPollFd.events
				  << ", revents: " << serverPollFd.revents << std::endl;
	}
	_FDs.push_back(serverPollFd);
	Connection serverConnection(serverPollFd, *this);
	serverConnection.setType(SERVER);
	serverConnection.setServerIp(inet_ntoa(_serverAddr.sin_addr));
	serverConnection.setServerPort(ntohs(_serverAddr.sin_port));
	if (VERBOSE)
	{
		std::cout << "Server Connection object created" << std::endl;
		std::cout << MAGENTA << "Printing serverConnection before push_back" << std::endl << RESET;
		serverConnection.printConnection();
	}
	_connections.push_back(serverConnection);
	if (VERBOSE)
	{
		std::cout << MAGENTA << "Printing serverConnection after push_back" << RESET << std::endl;
		_connections.back().printConnection();
		std::cout << "Server socket pollfd added to vectors" << std::endl;
	}
}

void Server::acceptNewConnection(Connection &conn)
{
	// TODO: think about naming.
	// We have 4 different names for kind of the same thing: clientAddress, newSocket, newSocketPoll,
	// newConnection
	struct sockaddr_in clientAddress;
	socklen_t ClientAddrLen = sizeof(clientAddress);
	std::cout << "New connection detected" << std::endl;
	// int newSocket = accept(_serverFD, (struct sockaddr *)&clientAddress, (socklen_t *)&ClientAddrLen);
	int newSocket = accept(conn.getPollFd().fd, (struct sockaddr *)&clientAddress, (socklen_t *)&ClientAddrLen);
	if (newSocket >= 0)
	{
		struct pollfd newSocketPoll;
		newSocketPoll.fd = newSocket;
		newSocketPoll.events = POLLIN;
		newSocketPoll.revents = 0;
		Connection newConnection(newSocketPoll, *this);
		newConnection.setType(CLIENT);
		newConnection.setServerIp(conn.getServerIp());
		newConnection.setServerPort(conn.getServerPort());
		if (VERBOSE)
		{

			std::cout << PURPLE << "BEFORE PUSH_BACK" << RESET << std::endl;
			std::cout << "Printing newConnection:" << std::endl;
			newConnection.printConnection();
			std::cout << "Printing struct pollfd newSocketPoll:" << std::endl;
			std::cout << "fd: " << newSocketPoll.fd << ", events: " << newSocketPoll.events
					  << ", revents: " << newSocketPoll.revents << std::endl;
		}
		/* start together */
		_FDs.push_back(newSocketPoll);
		_connections.push_back(newConnection);
		std::cout << newConnection.getHasFinishedReading() << std::endl;
		std::cout << _connections.back().getHasFinishedReading() << std::endl;
		/* end together */
		if (VERBOSE)
		{
			std::cout << PURPLE << "AFTER PUSH_BACK" << RESET << std::endl;
			std::cout << "Printing last element of _FDs:" << std::endl;
			std::cout << "fd: " << _FDs.back().fd << ", events: " << _FDs.back().events
					  << ", revents: " << _FDs.back().revents << std::endl;
			std::cout << "Printing last element of _connections:" << std::endl;
			_connections.back().printConnection();
			std::cout << "Pringing the whole _FDs and _connections vectors after adding new connection" << std::endl;
			print_connectionsVector(_connections);
			printFDsVector(_FDs);
		}
		char clientIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);
		std::cout << "New connection from " << clientIP << std::endl;
	}
	else
	{
		// TODO: consider what to do here. Not sure we want to exit the program.
		perror("In accept");
	}
}

void Server::handleServerSocketError()
{
	static int errorCounter = 0;
	perror("poll server socket error");
	if (errorCounter > 5)
	{
		std::cerr << "Too many errors on server socket. Exiting." << std::endl;
		exit(EXIT_FAILURE);
	}
	++errorCounter;
}

void Server::handleClientSocketError(int clientFD, size_t &i)
{
	std::cout << "handleClientSocketError" << std::endl;
	close(clientFD);
	/* start together */
	_FDs.erase(_FDs.begin() + i);
	_connections.erase(_connections.begin() + i);
	/* end together */
	--i;
	perror("poll client socket error");
}

void Server::handleSocketTimeoutIfAny()
{
	// Is not the socket timeout, but the poll timeout
	std::cout << "Timeout occurred!" << std::endl;
	// This should never happen with an infinite timeout
}

void Server::handlePollError()
{
	// linear issue: https://linear.app/web-serv/issue/WEB-91/implement-adequate-response-on-poll-failure
	if (errno == EINTR)
	{
		// The call was interrupted by a signal. Log and possibly retry.
		std::cerr << "poll() interrupted by signal. Retrying." << std::endl;
		// Basically we are not doing anything here.
	}
	else
	{
		// Log the error with as much detail as available.
		perror("Critical poll error from handlePollError()");

		// EBADF, EFAULT, EINVAL, ENOMEM indicate more severe issues.
		// Depending on the nature of your server, you might try to clean up and restart polling,
		// or log the failure and exit for manual recovery.
		// For a server, exiting might not be the best choice without trying to recover,
		// but ensure admins are alerted for investigation.

		// Example: Send alert to admin or trigger automatic recovery process
		AlertAdminAndTryToRecover();

		// In critical cases, consider a graceful shutdown.
		// exit(EXIT_FAILURE); or better yet, GracefulShutdown();
	}
}

void Server::AlertAdminAndTryToRecover()
{
	std::cerr << "Calling Leo and Daniil to fix the server" << std::endl;
	std::cerr << "Super dope function that recovers the server" << std::endl;
}

/* for handleConnection */

/* Others */

size_t Server::getClientMaxHeadersSize() const
{
	return _clientMaxHeadersSize;
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

void Server::checkSocketOptions()
{
	int optval;
	socklen_t optlen = sizeof(optval);

	if (getsockopt(_serverFD, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen) < 0)
	{
		perror("getsockopt SO_REUSEADDR failed");
	}
	else
	{
		// std::cout << "SO_REUSEADDR is " << (optval ? "enabled" : "disabled") << std::endl;
	}

	if (getsockopt(_serverFD, SOL_SOCKET, SO_REUSEPORT, &optval, &optlen) < 0)
	{
		perror("getsockopt SO_REUSEPORT failed");
	}
	else
	{
		// std::cout << "SO_REUSEPORT is " << (optval ? "enabled" : "disabled") << std::endl;
	}
}
