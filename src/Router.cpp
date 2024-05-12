#include "Router.hpp"
#include <string>

Router::Router()
{
}

Router::Router(ServerBlock serverBlock) : _serverBlock(serverBlock), _FDsRef(NULL), _pollFd(NULL)
{
}

Router::Router(const Router &obj) : _serverBlock(obj._serverBlock), _path(obj._path), _FDsRef(NULL), _pollFd(NULL)
{
}

Router &Router::operator=(const Router &obj)
{
	if (this == &obj)
		return *this;
	_serverBlock = obj._serverBlock;
	_path = obj._path;
	_FDsRef = obj._FDsRef;
	_pollFd = obj._pollFd;
	return *this;
}

Router::~Router()
{
}

// Modifies the HTTP request path:  removes host header and "http://" prefix
// combines with root directory for standardized routing,
void Router::adaptPathForFirefox(HTTPRequest &request)
{
	std::string path = _serverBlock.getRoot() + request.getSingleHeader("host").second;
	std::string requestTarget = request.getRequestTarget();
	size_t hostPos = requestTarget.find(request.getSingleHeader("host").second);
	if (hostPos != std::string::npos)
	{
		requestTarget.erase(hostPos, request.getSingleHeader("host").second.length());
	}

	size_t hostPos2 = requestTarget.find("http://");
	if (hostPos2 != std::string::npos)
	{
		std::string remove = "http://";
		requestTarget.erase(hostPos2, remove.length());
	}
	std::cout << "requestTarget after adaptPathForFirefox: " << requestTarget << std::endl;
	request.setRequestTarget(requestTarget);
	path += requestTarget;
	request.setPath(path);
}

void Router::routeRequest(HTTPRequest &request, HTTPResponse &response)
{
	Debug::log("Routing Request: host = " + request.getSingleHeader("host").second, Debug::NORMAL);
	std::string root = _serverBlock.getRoot();
	request.setRoot(root);
	std::string path = root + request.getSingleHeader("host").second;
	std::string requestTarget = request.getRequestTarget();
	std::cout << YELLOW << "requestTarget: " << requestTarget << RESET << std::endl;

	adaptPathForFirefox(request);

	std::cout << GREEN << "Routing request to path: " << path << RESET << std::endl;
	std::cout << PURPLE << "Request method: " << request.getMethod() << RESET << std::endl;

	// std::cout << request << std::endl;

	PathValidation pathResult = pathIsValid(response, request);
	std::cout << "Path validation result: " << pathResult << std::endl;

	switch (pathResult)
	{
	case PathValid:
		if (isCGI(request))
		{
			CGIHandler cgiHandler;
			cgiHandler.setFDsRef(_FDsRef);
			cgiHandler.setPollFd(_pollFd);
			cgiHandler.handleRequest(request, response);
		}
		else if (request.getMethod() == "POST" || request.getUploadBoundary() != "")
		{
			UploadHandler uploadHandler;
			uploadHandler.handleRequest(request, response);
		}
		else
		{
			StaticContentHandler staticContentHandler;
			staticContentHandler.handleRequest(request, response);
		}
		break;
	case IsDirectoryListing:
		break;
	case PathInvalid:
		std::cout << "Path is not valid, handling as error" << std::endl;
		handleServerBlockError(request, response, 404);
		break;
	}

	if (request.getMethod() == "SALAD")
	{
		std::cout << "🥬 + 🍅 + 🐟 = 🥗" << std::endl;
	}
}

bool Router::isDynamicRequest(const HTTPRequest &request)
{
	if (request.getMethod() == "POST" || request.getMethod() == "DELETE")
	{
		return true;
	}
	return false;
}

std::string Router::getFileExtension(const std::string &fileName)
{
	size_t dotIndex = fileName.find_last_of(".");
	if (dotIndex == std::string::npos)
	{
		return "";
	}

	size_t queryStart = fileName.find("?", dotIndex);
	if (queryStart == std::string::npos)
	{
		return fileName.substr(dotIndex);
	}
	else
	{
		return fileName.substr(dotIndex, queryStart - dotIndex - 1);
	}
}

void Router::handleServerBlockError(HTTPRequest &request, HTTPResponse &response, int errorCode)
{
	Debug::log("handleServerBlockError: entering function", Debug::NORMAL);
	// clang-format off
	std::vector<std::pair<int, std::string> > errorPage = _serverBlock.getErrorPage();
	// clang-format on
	size_t i = 0;
	for (; i < errorPage.size(); i++)
	{
		if (errorPage[i].first == errorCode)
		{
			std::cout << "handleServerBlockError: Error code: " << errorCode << std::endl;
			Debug::log("Path requested: " + request.getPath(), Debug::NORMAL);
			Debug::log("Path to error: " + errorPage[i].second, Debug::NORMAL);
			// setting the path to the custom error page
			request.setPath(_serverBlock.getRoot() + request.getHost() + "/" + errorPage[i].second);
			// std::cout << RED << "         custom error page: " << request.getPath() << RESET << std::endl;
			// TODO: move here what is todo below
			StaticContentHandler staticContentInstance;
			staticContentInstance.handleRequest(request, response);
			response.setStatusCode(errorCode, errorPage[i].second);
			return;
		}
	}
	Debug::log("handleServerBlockError: No custom error page found", Debug::NORMAL);
	StaticContentHandler staticContentInstance;
	// if (errorCode == 404) // we have our own
	// {
	// 	staticContentInstance.handleNotFound(response);
	// }
	// else // we generate it
	// {
	response.setErrorResponse(errorCode);
	// }
	return;

	// TODO: below will not be executed, doublecheck and cleanup

	PathValidation pathResult = pathIsValid(response, request);
	switch (pathResult)
	{
	case PathValid:
		staticContentInstance.handleRequest(request, response);
		response.setStatusCode(errorCode, errorPage[i].second);
		break;
	case PathInvalid:
		Debug::log("handleServerBlockError: path to given error is not valid", Debug::NORMAL);
		response.setErrorResponse(errorCode);
		break;
	case IsDirectoryListing:
		Debug::log("handleServerBlockError: path to given error is a directory listing", Debug::NORMAL);
		break;
	}
}

bool Router::isCGI(const HTTPRequest &request)
{
	// TODO: check against config file, not this hardcoded version
	std::vector<std::string> cgiExtensions = _serverBlock.getCgiExt();
	std::cout << RED << "isCGI" << RESET << std::endl;
	std::cout << "cgiExtensions: " << cgiExtensions.size() << std::endl;
	std::cout << "request target: " << request.getRequestTarget() << std::endl;
	if (!cgiExtensions.empty())
	{
		std::string fileExtension = getFileExtension(request.getRequestTarget());
		std::cout << "fileExtension: " << fileExtension << std::endl;
		for (size_t i = 0; i < cgiExtensions.size(); i++)
		{
			std::cout << "cgiExtensions[" << i << "]: " << cgiExtensions[i] << std::endl;
			if (cgiExtensions[i] == fileExtension)
			{
				Debug::log("isCGI: CGI request detected", Debug::NORMAL);
				return true;
			}
		}
	}
	Debug::log("isCGI: Not a CGI request", Debug::NORMAL);
	return false;
}

void Router::splitTarget(const std::string &target)
{
	_path.directories.clear();
	_path.resource.clear();

	// TODO: Eventually remove if already done in the request
	std::string::size_type queryPos = target.find('?');
	std::string path = target.substr(0, queryPos);

	std::string::size_type start = 0, end = 0;

	while ((end = path.find('/', start)) != std::string::npos)
	{
		std::string token = path.substr(start, end - start);
		if (!token.empty())
		{
			_path.directories.push_back(token);
		}
		start = end + 1;
	}
	std::string lastToken = path.substr(start);
	if (!lastToken.empty())
	{
		_path.resource = lastToken;
	}
}

void Router::generateDirectoryListing(HTTPResponse Response,
									  const std::string &directoryPath,
									  const std::string &requestedPath)
{
	std::ostringstream html;
	html << "<html><head><title>Directory listing for " << requestedPath << "</title></head>"
		 << "<body><h1>Directory listing for " << requestedPath << "</h1><ul>";

	DIR *dir = opendir(directoryPath.c_str());
	if (dir)
	{
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			std::string name(entry->d_name);
			if (name == "." || name == "..")
				continue;

			std::string fullUrl = requestedPath;
			if (!requestedPath.empty() && requestedPath[requestedPath.length() - 1] != '/')
			{
				fullUrl += '/';
			}
			fullUrl += name;

			html << "<li><a href=\"" << fullUrl << "\">" << name << "</a></li>";
		}
		closedir(dir);
	}

	html << "</ul></body></html>";
	Response.setBody(html.str());
	Response.setStatusCode(200, "OK");
	Response.setHeader("Content-Type", "text/html");
}

enum PathValidation Router::pathIsValid(HTTPResponse &response, HTTPRequest &request)
{
	(void)response;
	struct stat buffer;
	std::string path = request.getPath();
	if (stat(path.c_str(), &buffer) != 0)
	{
		std::cout << "Failed to stat the file at path: " << path << std::endl;
		Debug::log("webRoot: " + path, Debug::NORMAL);
		Debug::log("pathIsValid: stat failed, path does not exist", Debug::NORMAL);
		return PathInvalid;
	}
	if (S_ISDIR(buffer.st_mode))
	{
		if (!path.empty() && path[path.length() - 1] != '/')
		{
			path += "/";
		}
		if (_serverBlock.getIndex().empty())
		{
			Debug::log("User did not provided any index", Debug::NORMAL);
			if (_serverBlock.getAutoIndex())
			{
				Debug::log("pathIsValid: Autoindex is on", Debug::NORMAL);
				generateDirectoryListing(response, path, request.getRequestTarget());
				// std::cout << "Directory listing: " << directoryListing << std::endl;
				// path += "index.html";
				// request.setPath(path);
				// std::cout << std::endl;
			}
			else
			{
				Debug::log("pathIsValid: Autoindex is off", Debug::NORMAL);
				return IsDirectoryListing;
			}
		}
		else // user provided an index
		{
			Debug::log("pathIsValid: Index: " + _serverBlock.getIndex()[0], Debug::NORMAL);
			// TODO: implement several indexes
			std::string index = _serverBlock.getIndex()[0];
			Debug::log("pathIsValid: Index: " + index, Debug::NORMAL);
			path += index;
			Debug::log("pathIsValid: path: " + path, Debug::NORMAL);
		}
		request.setPath(path);
		std::cout << std::endl;
	}

	std::ifstream file(path.c_str());
	if (!file.is_open())
	{
		std::cout << "Failed to open the file at path: " << path << std::endl;

		// if (path[path.length() - 1] != '/')
		// {
		// 	path += "/";
		// }
		// path += _serverBlock.getErrorPage()[0].second;
		// std::cout << GREEN << "pathIsValid: path: " << path << RESET << std::endl;
		return PathInvalid;
	}
	file.close();

	return PathValid;
}

void Router::setFDsRef(std::vector<struct pollfd> *FDsRef)
{
	_FDsRef = FDsRef;
}

void Router::setPollFd(struct pollfd *pollFd)
{
	_pollFd = pollFd;
}
