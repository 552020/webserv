#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <poll.h>
#include <fstream>
#include <sstream>
#include "HTTPRequest.hpp"
#include "server_utils.hpp"
#include "webserv.hpp"

class Server
{
  public:
	Server();
	Server(const std::string configFilePath);
	~Server();

	void startListening();
	void startPollEventLoop();

	int getPort() const;
	void setPort(int port);

	std::string getWebRoot() const;
	void setWebRoot(const std::string &webRoot);
	std::string getConfigFilePath() const;

  private:
	/* Private Attributes*/
	int _port;
	int _serverFD;
	int _maxClients; // i.e. max number of pending connections
	std::string _configFilePath;
	std::string _webRoot;
	struct sockaddr_in _serverAddr;
	std::vector<struct pollfd> _FDs;

	/* Private Methods */

	void loadConfig();
	void loadDefaultConfig();
	void createServerSocket();
	void setReuseAddrAndPort();
	void bindToPort(int port);
	void listen();
	void handleConnection(int clientFD);

	/* Not avaiabel constructors */

	// Copy constructor
	Server(const Server &other);
	// Assignment operator
	Server &operator=(const Server &other);
};

#endif