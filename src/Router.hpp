#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "StaticContentHandler.hpp"
#include "CGIHandler.hpp"
#include "UploadHandler.hpp"
#include "ServerBlock.hpp"
#include "Debug.hpp"
#include "sys/stat.h"
#include "Connection.hpp"
#include <dirent.h>

struct resourcePath
{
	std::vector<std::string> directories;
	std::string resource;
};

enum PathValidation
{
	PathValid,
	PathInvalid,
	IsDirectoryListing
};

class Router
{
  public:
	Router();
	Router(ServerBlock serverBlock, Server &server, Connection &conn);
	~Router();
	Router(const Router &other);
	Router &operator=(const Router &other);
	void routeRequest(Connection &conn, HTTPRequest &request, HTTPResponse &response);

	void splitTarget(const std::string &target);
	bool isDynamicRequest(const HTTPRequest &request);
	enum PathValidation pathIsValid(HTTPResponse &response, HTTPRequest &request);
	void setFDsRef(std::vector<struct pollfd> *FDsRef);
	void setPollFd(struct pollfd *pollFd);
	void setWebRoot(std::string &webRoot);
	void handleServerBlockError(HTTPRequest &request, HTTPResponse &response, int errorCode);

  private:
	Server &_server;
	Connection &_conn;
	ServerBlock _serverBlock;
	StaticContentHandler _staticContentHandler;
	resourcePath _path;
	std::vector<pollfd> *_FDsRef;
	struct pollfd *_pollFd;
	std::string getFileExtension(const std::string &fileName);
	void generateDirectoryListing(HTTPResponse &Response,
								  const std::string &directoryPath,
								  const std::string &requestedPath);
	bool isCGI(const HTTPRequest &request);
	CGIHandler _cgiHandler;
	void adaptPathForFirefox(HTTPRequest &request);
};

#endif