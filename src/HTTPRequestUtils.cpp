#include "HTTPRequest.hpp"
#include <string.h>
int 	hexToInt(std::string hex);
bool	hasCRLF(const char* request, unsigned int& i, int mode);

bool	isOrigForm(std::string &requestTarget, int &queryStart){
	for (int i = 0; i < (int)requestTarget.length(); i++){
		if (requestTarget[i] == '?'){
			queryStart = i;
			return (true);
		}
	}
	return (false);
}

bool	fileExists(std::string &requestTarget, bool isOriginForm, int queryStart){
	if (isOriginForm && \
	access(("." + requestTarget.substr(0, queryStart)).c_str(), F_OK) == -1)
		return (false);
	if (!isOriginForm && access(("." + requestTarget).c_str(), F_OK) == -1)
		return (false);
	return (true);
}

bool	isInvalidChar(const char& c)
{
	if ((c >= 0 && c <= 31) || c == 127)
		return (true);
	return (false);
}

void	skipRequestLine(const char *request, unsigned int& i)
{
	while (request[i]){
		if (hasCRLF(request, i, 0)){
			i += 2;
			return ;	
		}
		i++;
	}
}

void	skipHeader(const char *request, unsigned int& i)
{
	while (request[i]){
		if (hasCRLF(request, i, 1)){
			i += 4; // skip "\r\n\r\n"
			return ;
		}
		i++;
	}
}

bool	isValidContentType(std::string type)
{
	if (type == "text/plain" || type == "text/html")
		return (true);
	return (false);
}

bool	isValidHost(std::string host)
{
	(void)host;
	return (true);
}

/*
The combination of \\r\\n
is as a standard way to denote the end of a line in HTTP headers.
*/
bool	hasCRLF(const char* request, unsigned int& i, int mode)
{
	if (mode == 0){
		if (request[i] && request[i + 1] \
		&& request[i] == '\r' && request[i + 1] == '\n')
			return (true);
		return (false);
	}
	if (mode == 1){
		if (request[i] && request[i + 1] && request[i + 2] && request[i + 3] \
		&& request[i] == '\r' && request[i + 1] == '\n' \
		&& request[i + 2] == '\r' && request[i + 3] == '\n')
			return (true);
		return (false);
	}
	return (false);
}

bool	hasMandatoryHeaders(HTTPRequest& obj)
{
	int	isHost = 0;
	int	isContentLength = 0;
	int	isContentType = 0;
	std::multimap<std::string, std::string> headers = obj.getHeaders();
	std::multimap<std::string, std::string>::iterator it;

	for (it = headers.begin(); it != headers.end(); it++){
		if (it->first == "Host"){
			if (!isValidHost(it->second))
				return (false);
			isHost++;
		}
		else if (it->first == "Content-length"){
			if (obj.getMethod() != "POST")
				return (false);
			isContentLength++;
		}
		else if (it->first == "Content-type"){
			if (!isValidContentType(it->second) || obj.getMethod() != "POST")
				return (false);
			isContentType++;
		}
		else if (it->first == "Transfer-encoding"){
			if (it->second != "chunked" || obj.getMethod() != "POST")
				return (false);
			obj.setIsChunked(true);
		}
	}
	if (obj.getMethod() == "POST")
		return (isHost == 1 && isContentLength == 1 && isContentType == 1);
	else
		return (isHost == 1);
}

std::string	extractValue(std::string& variables, int &i)
{
	int	startPos;

	startPos = i++;
	while (variables[i] && variables[i] != '&'){
		if (variables[i] == '=')
			return ("");
		i++;
	}
	if (variables[startPos] == '=')
		startPos++;
	if (variables[i] == '&' && !variables[i + 1])
		return ("");
	return (variables.substr(startPos, i - startPos));
}

std::string extractKey(std::string& variables, int &i, int startPos)
{
	if (i == 0 || variables[i] == '?')
		return ("");
	if (variables[startPos] == '&' && startPos != 0)
		startPos++;
	if (variables.substr(startPos, i - startPos).find('&') != std::string::npos \
	|| variables.substr(startPos, i - startPos).find('?') != std::string::npos)
		return ("");
	return (variables.substr(startPos, i - startPos));
}

std::string extractRequestTarget(const char *request, unsigned int& i)
{
	std::string		requestTarget;
	std::string		string_request(request);
	unsigned int	start = i;

	while (request[i] && request[i] != ' ' && !isInvalidChar(request[i]))
		i++;
	if (i > MAX_URI)
		return ("");
	requestTarget = string_request.substr(start, i - start);
	//std::cout << requestTarget << std::endl;
	return (requestTarget);
}

std::string	extractVariables(std::string& requestTarget, bool& isOriginForm)
{
	int		queryStart = 0;

	if (requestTarget == "/")
		return ("/");
	isOriginForm = isOrigForm(requestTarget, queryStart);
	// if (!fileExists(requestTarget, isOriginForm, queryStart))
	// 	return ("");
	if (isOriginForm)
		return (requestTarget.substr(queryStart + 1, strlen(requestTarget.c_str()) - queryStart));
	return (requestTarget);
}

std::string extractProtocolVersion(const char *request, unsigned int& i)
{
	std::string		protocolVersion;
	std::string		string_request(request);
	unsigned int	start = i;

	while (request[i] && request[i] != '\r' && !isInvalidChar(request[i]))
		i++;
	protocolVersion = string_request.substr(start, i - start);
	//std::cout << protocolVersion << std::endl;
	if (protocolVersion == "HTTP/1.1")
		return (protocolVersion);
	return ("");
}

std::string	extractMethod(const char *request, unsigned int& i)
{
	std::string	method;
	std::string	string_request(request);

	while (request[i] && request[i] != ' ' && !isInvalidChar(request[i]))
		i++;
	method = string_request.substr(0, i);
	//std::cout << method << std::endl;
	if (method == "GET" || method == "POST" || method == "DELETE")
		return (method);
	return ("");
}

std::string	extractHeaderKey(const char *request, unsigned int& i)
{
	std::string	string_request(request);
	unsigned int	start = i;

	while (request[i] && request[i] != ':'){
		if (isInvalidChar(request[i]) || request[i] == ' ')
			return ("");
		i++;
	}
	//std::cout << "Key: "<< string_request.substr(start, i - start) << std::endl;
	return (string_request.substr(start, i - start));
}

std::string	extractHeaderValue(const char *request, unsigned int& i)
{
	std::string		string_request(request);
	unsigned int	start = i;

	while (request[i] && request[i] != '\r'){
		if (isInvalidChar(request[i]))
			return ("");
		i++;
	}
	//std::cout << "Value: "<< string_request.substr(start, i - start) << std::endl;
	return (string_request.substr(start, i - start));
}

int	extractLineLength(const char *request, unsigned int& i)
{
	std::string string_request(request);
	unsigned int start = i;
	int size = 0;

	while (request[i] && request[i] != '\r')
		i++;
	if (request[i] != '\r' || request[i + 1] != '\n')
		return (-1);
	size = hexToInt(string_request.substr(start, i - start));
	if (size <= 0)
		return (size);
	//std::cout << "Len: " << size << std::endl;
	i += 2; // skip '\r' and '\n'
	return (size);
}

std::string		extractLine(const char *request, unsigned int& i, const unsigned int& size)
{
	std::string string_request(request);
	std::string line = string_request.substr(i, size);
	i += size; // skip line
	if (request[i] != '\r' || request[i + 1] != '\n')
		return ("");
	i += 2; // skip '\r' and '\n'
	//std::cout << "Word: " << line << std::endl;
	return (line);
}
