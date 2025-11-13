#include <iostream>
#include <cstdlib>
#include <cctype>
#include <string>
#include "Server.hpp"

bool isValidPort(const std::string& portStr) {
	if (portStr.empty())
		return false;
	
	for (size_t i = 0; i < portStr.length(); ++i) {
		if (!std::isdigit(portStr[i]))
			return false;
	}
	
	int port = std::atoi(portStr.c_str());
	return (port >= 1024 && port <= 65535);
}

void printUsage(const char* programName) {
	std::cout << "Usage: " << programName << " <port> <password>" << std::endl;
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printUsage(argv[0]);
		return 1;
	}
	
	std::string portStr(argv[1]);
	std::string password(argv[2]);
	
	if (!isValidPort(portStr)) {
		std::cout << "Error: Port must be a number between 1024 and 65535" << std::endl;
		printUsage(argv[0]);
		return 1;
	}
	
	int port = std::atoi(portStr.c_str());
	
	try
	{
		Server server(port, password);
		server.start();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}

