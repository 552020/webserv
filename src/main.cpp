#include "Server.hpp"
#include "ConfigFile.hpp"

int main(int argc, char **argv)
{
	// (void)argc;
	// (void)argv;
	if (argc > 2)
	{
		std::cerr << "Wrong number of arguments." << std::endl;
		std::cerr << "Usage: ./webserv [config_file]" << std::endl;
		std::cerr << "The extra arguments are ignored." << std::endl;
	}
	if (argc == 2)
	{
		std::string configFile = argv[1];
		ConfigFile config(configFile);
		std::cout << config << std::endl;
	}
	Server webserv;
	webserv.startListening();
	webserv.startPollEventLoop();

	return 0;
}