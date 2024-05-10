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
	_FDsRef = NULL;
	_pollFd = NULL;
	return *this;
}

Router::~Router()
{
}

void Router::routeRequest(const HTTPRequest &request, HTTPResponse &response)
{
	Debug::log("routeRequest Request host: " + request.getSingleHeader("Host").second, Debug::NORMAL);

	Debug::log("routeRequest _webRoot: " + _serverBlock.getRoot(), Debug::NORMAL);
	std::string _webRoot = _serverBlock.getRoot();
	if (isCGI(request) && pathIsValid(const_cast<HTTPRequest &>(request), _webRoot))
	{
		CGIHandler cgiHandler;
		cgiHandler.setFDsRef(_FDsRef);
		cgiHandler.setPollFd(_pollFd);
		cgiHandler.handleRequest(request, response);
		return;
	}
	else if (request.getMethod() == "POST") // && !request.getUploadBoundary().empty())
	{
		std::cout << "Router: POST request" << std::endl;

		UploadHandler uploadHandler;
		uploadHandler.handleRequest(request, response);
	}
	else if (request.getMethod() == "SALAD")
	{
		std::cout << "🥬 + 🍅 + 🐟 = 🥗" << std::endl;
	}
	else
	{
		StaticContentHandler staticContentInstance;
		if (!pathIsValid(const_cast<HTTPRequest &>(request), _webRoot))
		{
			std::cout << "Path is not valid, calling handleNotFound" << std::endl;
			staticContentInstance.handleNotFound(response);
		}
		else
		{
			staticContentInstance.handleRequest(request, response);
		}
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
		return fileName.substr(dotIndex + 1);
	}
	else
	{
		return fileName.substr(dotIndex + 1, queryStart - dotIndex - 1);
	}
}

void Router::handleServerBlockError(const HTTPRequest &request, HTTPResponse &response, int errorCode)
{
	// clang-format off
	std::vector<std::pair<int, std::string> > errorPage = _serverBlock.getErrorPage();
	// clang-format on
	std::string errorPath;
	for (size_t i = 0; i < errorPage.size(); i++)
	{
		if (errorPage[i].first == errorCode)
		{
			errorPath = errorPage[i].second;
			break;
		}
	}
	if (errorPath.empty())
	{
		return;
	}
	StaticContentHandler staticContentInstance;
	if (!pathIsValid(const_cast<HTTPRequest &>(request), errorPath))
	{
		Debug::log("handleServerBlockError: path to given error is not valid", Debug::NORMAL);
		staticContentInstance.handleNotFound(response);
	}
	else
	{
		Debug::log("handleServerBlockError: Response set to custom error", Debug::NORMAL);
		staticContentInstance.handleRequest(request, response);
	}
}

bool Router::isCGI(const HTTPRequest &request)
{
	// TODO: check against config file, not this hardcoded version
	std::string fileExtension = getFileExtension(request.getRequestTarget());
	return (fileExtension == "cgi" || fileExtension == "pl" || fileExtension == "py" || fileExtension == "php");
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

bool Router::pathIsValid(HTTPRequest &request, std::string &webRoot)
{
	std::string host = request.getHost();
	Debug::log("pathIsValid Host: " + host, Debug::NORMAL);
	Debug::log("pathIsValid WebRoot: " + webRoot, Debug::NORMAL);
	size_t pos = host.find(":");
	if (pos != std::string::npos)
	{
		host = host.substr(0, pos);
	}

	// for ease of use during deployment
	// this if/else allows to reach target with tester or browser
	// if (host != "localhost")
	// 	webRoot = host + webRoot;

	struct stat buffer;
	if (stat(webRoot.c_str(), &buffer) != 0)
	{
		Debug::log("pathIsValid: stat failed, path does not exist", Debug::NORMAL);
		return false;
	}
	if (S_ISDIR(buffer.st_mode))
	{
		if (!webRoot.empty() && webRoot[webRoot.length() - 1] != '/')
		{
			webRoot += "/";
		}
		// TODO: check directory listing
		if (_serverBlock.getIndex().empty())
		{
			Debug::log("User did not provided any index", Debug::NORMAL);
			// if directory listing
			//____show dirs
			// else
			//____return 403
		}
		else
		{
			Debug::log("pathIsValid: Index: " + _serverBlock.getIndex()[0], Debug::NORMAL);
			// TODO: implement several indexes
			std::string index = _serverBlock.getIndex()[0];
			Debug::log("pathIsValid: Index: " + index, Debug::NORMAL);
			webRoot += index;
			Debug::log("pathIsValid: WebRoot: " + webRoot, Debug::NORMAL);
		}
		request.setPath(webRoot);
	}

	std::ifstream file(webRoot.c_str());
	if (!file.is_open())
	{
		std::cout << "Failed to open the file at path: " << webRoot << std::endl;
		return false;
	}
	file.close();

	return true;
}

void Router::setFDsRef(std::vector<struct pollfd> *FDsRef)
{
	_FDsRef = FDsRef;
}

void Router::setPollFd(struct pollfd *pollFd)
{
	_pollFd = pollFd;
}
