#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <fstream>
#include <sstream>
#include "MetaVariables.hpp"
#include "HTTPRequest.hpp"
#include "server_utils.hpp"
#include "Connection.hpp"
#include "webserv.hpp"
#include "Parser.hpp"

class Connection; // Forward declaration for circular dependency

class Server
{
  public:
	Server();
	Server(const std::string configFilePath);
	~Server();

	void startListening();
	void startPollEventLoop();

	// GETTERS
	std::string getConfigFilePath() const;
	int getPort() const;
	std::string getWebRoot() const;
	size_t getClientMaxHeadersSize() const;

	// SETTERS
	void setPort(int port);
	void setWebRoot(const std::string &webRoot);

	void checkSocketOptions();

  private:
	/* Private Attributes */
	int _port;
	int _serverFD;
	size_t _clientMaxHeadersSize;
	int _clientMaxBodySize;
	int _maxClients; // i.e. max number of pending connections
	std::string _configFilePath;
	std::string _webRoot;
	struct sockaddr_in _serverAddr;
	std::vector<struct pollfd> _FDs;
	std::vector<Connection> _connections;

	/*** Private Methods ***/
	/* for Constructors */
	void loadConfig();
	void loadDefaultConfig();
	/* for startListening */
	void createServerSocket();
	void setReuseAddrAndPort();
	void bindToPort(int port);
	void listen();
	/* for startPollEventLoop */
	void addServerSocketPollFdToVectors();
	void acceptNewConnection(Connection &conn);
	void handleConnection(Connection &conn, size_t &i, Parser &parser, HTTPRequest &request, HTTPResponse &response);
	void handleServerSocketError();
	void handleClientSocketError(int clientFD, size_t &i);
	void handleSocketTimeoutIfAny();
	void handlePollError();
	void AlertAdminAndTryToRecover();

	/* for handleConnection */
	void readFromClient(Connection &conn, size_t &i, Parser &parser, HTTPRequest &request, HTTPResponse &response);
	void buildResponse(Connection &conn, size_t &i, HTTPRequest &request, HTTPResponse &response);
	void writeToClient(Connection &conn, size_t &i, HTTPResponse &response);
	void closeClientConnection(Connection &conn, size_t &i);

	/* Not avaiable constructors */
	// Copy constructor
	Server(const Server &other);
	// Assignment operator
	Server &operator=(const Server &other);
};

#endif