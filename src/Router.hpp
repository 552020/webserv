#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "StaticContentHandler.hpp"
#include "CGIHandler.hpp"
#include "UploadHandler.hpp"
#include "ServerBlock.hpp"
#include "sys/stat.h"

struct resourcePath
{
	std::vector<std::string> directories;
	std::string resource;
};

class Router
{
  public:
	Router();
	Router(std::vector<ServerBlock> serverBlocks);
	~Router();
	void routeRequest(const HTTPRequest &request, HTTPResponse &response);

	void splitTarget(const std::string &target);
	bool isDynamicRequest(const HTTPRequest &request);
	bool pathIsValid(HTTPRequest &request, std::string &webRoot);
	void setFDsRef(std::vector<struct pollfd> *FDsRef);
	void setPollFd(struct pollfd *pollFd);

  private:
	ServerBlock _serverBlock;
	Router(const Router &other);
	Router &operator=(const Router &other);
	StaticContentHandler _staticContentHandler;
	CGIHandler _cgiHandler;
	std::vector<pollfd> *_FDsRef;
	struct pollfd *_pollFd;
	std::string getFileExtension(const std::string &fileName);
	bool isCGI(const HTTPRequest &request);
	resourcePath _path;
};

#endif