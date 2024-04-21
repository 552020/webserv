#include "CGIHandler.hpp"

CGIHandler::CGIHandler()
{
}

CGIHandler::~CGIHandler()
{
}

CGIHandler &CGIHandler::operator=(const CGIHandler &other)
{
	if (this != &other)
	{										// Protect against self-assignment
		AResponseHandler::operator=(other); // Call the base class assignment operator
											// Copy or assign other members of CGIHandler if necessary
	}
	return *this;
}

HTTPResponse CGIHandler::handleRequest(const HTTPRequest &request)
{
	CGIHandler cgiInstance;
	MetaVariables env;
	env.HTTPRequestToMetaVars(request, env);
	std::cout << env;
	std::string cgiOutput = executeCGI(env);

	// HTTPResponse response;
	// response.setBody(cgiOutput);
	// response.setIsCGI(true);
	// std::cout << response;
	return CGIStringToResponse(cgiOutput);
}

char *const *CGIHandler::createArgvForExecve(const MetaVariables &env)
{
	int argvSize = env.getVar("X_INTERPRETER_PATH") != "" ? 3 : 2;
	char **argv = new char *[argvSize];

	std::string scriptName = env.getVar("SCRIPT_NAME");
	std::string pathTranslated = env.getVar("PATH_TRANSLATED");
	std::string scriptPath = pathTranslated + scriptName;

	if (env.getVar("X_INTERPRETER_PATH") != "")
	{
		argv[0] = new char[env.getVar("X_INTERPRETER_PATH").length() + 1];
		std::strcpy(argv[0], env.getVar("X_INTERPRETER_PATH").c_str());
		argv[1] = new char[scriptPath.length() + 1];
		std::strcpy(argv[1], scriptPath.c_str());
	}
	else
	{
		argv[0] = new char[scriptPath.length() + 1];
		std::strcpy(argv[0], scriptPath.c_str());
		argv[1] = NULL;
	}

	argv[argvSize] = NULL;

	return argv;
}

HTTPResponse CGIHandler::CGIStringToResponse(const std::string &cgiOutput)
{
	HTTPResponse response;

	std::size_t headerEndPos = cgiOutput.find("\r\n\r\n");
	if (headerEndPos == std::string::npos)
	{
		headerEndPos = cgiOutput.find("\n\n");
		if (headerEndPos != std::string::npos)
		{
			headerEndPos += 2;
		}
	}
	else
	{
		headerEndPos += 4;
	}

	std::string headersPart, bodyPart;
	if (headerEndPos != std::string::npos)
	{
		headersPart = cgiOutput.substr(0, headerEndPos - (cgiOutput[headerEndPos - 1] == '\n' ? 2 : 4));
		bodyPart = cgiOutput.substr(headerEndPos);
	}
	else
	{
		bodyPart = cgiOutput;
	}

	std::istringstream headerStream(headersPart);
	std::string headerLine;
	while (std::getline(headerStream, headerLine))
	{
		if (!headerLine.empty() && headerLine[headerLine.size() - 1] == '\r')
		{
			headerLine.erase(headerLine.size() - 1);
		}

		std::size_t separatorPos = headerLine.find(": ");
		if (separatorPos != std::string::npos)
		{
			std::string headerName = headerLine.substr(0, separatorPos);
			std::string headerValue = headerLine.substr(separatorPos + 2);
			response.setHeader(headerName, headerValue);
		}
	}

	response.setBody(bodyPart);
	response.setIsCGI(true);
	response.setStatusCode(200);
	return response;
}

void CGIHandler::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
	{
		perror("fcntl failed");
		_exit(EXIT_FAILURE);
	}

	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
	{
		perror("fcntl failed");
		_exit(EXIT_FAILURE);
	}
}

std::string CGIHandler::executeCGI(const MetaVariables &env)
{
	std::string cgiOutput = "";
	char *const *argv = createArgvForExecve(env);

	int serverToCGI[2];
	int CGIToServer[2];
	if (pipe(serverToCGI) == -1 || pipe(CGIToServer) == -1)
	{
		perror("pipe failed");
		_exit(EXIT_FAILURE);
	}

	pid_t forked_pid = fork();
	if (forked_pid == -1)
	{
		perror("fork failed");
		_exit(EXIT_FAILURE);
	}
	else if (forked_pid == 0)
	{						   // Child process
		close(serverToCGI[1]); // Close unused write end
		close(CGIToServer[0]); // Close unused read end

		dup2(serverToCGI[0], STDIN_FILENO);
		dup2(CGIToServer[1], STDOUT_FILENO);

		close(serverToCGI[0]);
		close(CGIToServer[1]);

		std::vector<char *> envp = env.getForExecve();
		execve(argv[0], argv, envp.data());

		perror("execve");
		_exit(EXIT_FAILURE);
	}
	else
	{						   // Parent process
		close(serverToCGI[0]); // Close unused read end
		close(CGIToServer[1]); // Close unused write end

		// Set non-blocking mode for CGIToServer[0]
		int flags = fcntl(CGIToServer[0], F_GETFL, 0);
		if (flags == -1)
		{
			perror("fcntl F_GETFL");
			_exit(EXIT_FAILURE);
		}
		flags |= O_NONBLOCK;
		if (fcntl(CGIToServer[0], F_SETFL, flags) == -1)
		{
			perror("fcntl F_SETFL");
			_exit(EXIT_FAILURE);
		}

		char readBuffer[1024];
		ssize_t bytesRead;
		bool isCGIOutputFinished = false;
		while (!isCGIOutputFinished)
		// if (!isCGIOutputFinished) // works with while // no will read only once
		{
			memset(readBuffer, 0, sizeof(readBuffer));
			bytesRead = read(CGIToServer[0], readBuffer, sizeof(readBuffer) - 1);
			if (bytesRead == 0) // finised reading
			{
				isCGIOutputFinished = true;
			}
			else if (bytesRead > 0) // reading
			{
				readBuffer[bytesRead] = '\0';
				cgiOutput += readBuffer;
			}
		}

		close(CGIToServer[0]);

		// SHOUL BE IN Server::startPollEventLoop
		//  int status;
		//  waitpid(forked_pid, &status, WNOHANG);

		while (*argv != NULL)
		{
			delete[] *argv;
			++argv;
		}

		return cgiOutput;
	}
}
