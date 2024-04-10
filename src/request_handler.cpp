#include <cstdlib> // For exit() and EXIT_FAILURE
#include <cstring> // For memset
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <sys/socket.h> // For socket functions
#include <unistd.h>     // For read, write, and close
#include <fstream>
#include <sstream>
#include "webserv.hpp"

std::string handleHomePage() {
    std::string htmlContent = readHtml("./html/home.html");
	std::stringstream ss;
    ss << htmlContent.length();
    std::string htmlLength = ss.str();
    std::string httpResponse = "HTTP/1.1 200 OK\nContent-Type: text/html\n" +
                               std::string("Content-Length: ") + htmlLength + "\n\n" +
                               htmlContent;
    std::cout << "------------------Home page returned from handleHomePage()-------------------" << std::endl;
    return httpResponse;
}

std::string handleNotFound(void) {
    std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    // write(socket, response.c_str(), response.size());
    std::cout << "------------------404 Not Found sent-------------------" << std::endl;

    return response;
}