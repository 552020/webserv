#include <cstdlib> // For exit() and EXIT_FAILURE
#include <cstring> // For memset
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <sys/socket.h> // For socket functions
#include <unistd.h>     // For read, write, and close
#include <fstream>
#include <sstream>
#include "webserv.hpp"
#include "HTTPRequest.hpp"

const int BUFFER_SIZE = 1024;


void printVariablesHeadersBody(const HTTPRequest& requestInstance)
{
	std::multimap<std::string, std::string> a = requestInstance.getHeaders();
	std::multimap<std::string, std::string> b = requestInstance.getQueryString();
	std::vector<std::string>				c = requestInstance.getBody();

	std::multimap<std::string, std::string>::iterator it;
	std::cout << "---------------------Variables--------------------" << std::endl;
	for (it = b.begin(); it != b.end(); it++){
		std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
	}
	std::cout << "---------------------End--------------------------" << std::endl;
	std::cout << "---------------------Headers----------------------" << std::endl;
	for (it = a.begin(); it != a.end(); it++){
		std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
	}
	std::cout << "---------------------End--------------------------" << std::endl;
	std::cout << "---------------------Body-------------------------" << std::endl;
	for (size_t i = 0; i < c.size(); ++i)
		std::cout << c[i] << std::endl;
	std::cout << "---------------------End--------------------------" << std::endl;
}

// Determine the type of request and call the appropriate handler
void handleConnection(int socket) {
    char buffer[BUFFER_SIZE] = {0};
    long valRead = read(socket, buffer, BUFFER_SIZE);

	HTTPRequest requestInstance(buffer);
	if (requestInstance.getIsChunked() == true)
		requestInstance.parseChunkedBody(buffer);
	std::cout << requestInstance.getStatusCode() << std::endl;
	printVariablesHeadersBody(requestInstance);

    if (valRead < 0) {
        perror("In read");
        exit(EXIT_FAILURE);
    }
    std::cout << "Received HTTP request: " << std::endl << buffer << std::endl;

    //test to execute the python script (see: https://www.tutorialspoint.com/python/python_cgi_programming.htm)
    // const char* argv[] = { "./cgi-bin/hello_py.cgi", NULL };
    // const char* argv[] = { "./cgi-bin/thirty_py.cgi", NULL };
    // const char* argv[] = { "./cgi-bin/hello.cgi", NULL };

    std::string response;
    if (strstr(buffer, "GET / HTTP/1.1") || strstr(buffer, "GET /home HTTP/1.1")) {
        response = handleHomePage();
    } else if (strstr(buffer, "GET /hello HTTP/1.1")) {
        // response = handleCGIRequest(argv, env);
        CGIHandler cgiInstance;
        response = cgiInstance.handleRequest(requestInstance);
        std::cout << "------------------CGI output received-------------------" << std::endl;
        std::cout << response << std::endl;
    } else {
        response = handleNotFound();
    }

    write(socket, response.c_str(), response.size());
    close(socket);
    }

