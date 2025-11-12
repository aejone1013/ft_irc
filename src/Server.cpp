#include "Server.hpp"
#include "Client.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <signal.h>
#include <poll.h>

// Global Server pointer for signal handler
static Server* g_serverInstance = NULL;

// Signal handler for graceful shutdown
static void signalHandler(int sig)
{
	(void)sig;
	if (g_serverInstance != NULL)
	{
		g_serverInstance->stop();
	}
}

Server::Server(int port, const std::string& password)
	: _port(port), _password(password), _serverSocket(-1), _isRunning(false)
{
}

Server::~Server()
{
	// Cleanup all clients
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		delete it->second;
		close(it->first);
	}
	_clients.clear();

	// Close server socket
	if (_serverSocket != -1)
	{
		close(_serverSocket);
	}
}

void Server::setupSocket()
{
	// Create socket
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket == -1)
	{
		throw std::runtime_error(std::string("Failed to create socket: ") + strerror(errno));
	}

	// Set socket to non-blocking
	int flags = fcntl(_serverSocket, F_GETFL, 0);
	if (flags == -1)
	{
		close(_serverSocket);
		throw std::runtime_error(std::string("Failed to get socket flags: ") + strerror(errno));
	}
	if (fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		close(_serverSocket);
		throw std::runtime_error(std::string("Failed to set socket to non-blocking: ") + strerror(errno));
	}

	// Set SO_REUSEADDR option
	int reuse = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
	{
		close(_serverSocket);
		throw std::runtime_error(std::string("Failed to set SO_REUSEADDR: ") + strerror(errno));
	}

	// Bind socket
	struct sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(_port);

	if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		close(_serverSocket);
		std::ostringstream oss;
		oss << "Failed to bind socket to port " << _port << ": " << strerror(errno);
		throw std::runtime_error(oss.str());
	}

	// Listen
	if (listen(_serverSocket, 10) == -1)
	{
		close(_serverSocket);
		throw std::runtime_error(std::string("Failed to listen on socket: ") + strerror(errno));
	}

	// Add server socket to pollfds
	struct pollfd serverPollfd;
	serverPollfd.fd = _serverSocket;
	serverPollfd.events = POLLIN;
	serverPollfd.revents = 0;
	_pollfds.push_back(serverPollfd);
}

void Server::handleNewConnection()
{
	// Accept new connection
	int clientFd = accept(_serverSocket, NULL, NULL);
	if (clientFd == -1)
	{
		std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
		return;
	}

	// Set client socket to non-blocking
	int flags = fcntl(clientFd, F_GETFL, 0);
	if (flags == -1)
	{
		std::cerr << "Failed to get client socket flags: " << strerror(errno) << std::endl;
		close(clientFd);
		return;
	}
	if (fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		std::cerr << "Failed to set client socket to non-blocking: " << strerror(errno) << std::endl;
		close(clientFd);
		return;
	}

	// Create new Client object
	Client* client = new Client(clientFd);

	// Add to clients map
	_clients[clientFd] = client;

	// Add client fd to pollfds
	struct pollfd clientPollfd;
	clientPollfd.fd = clientFd;
	clientPollfd.events = POLLIN;
	clientPollfd.revents = 0;
	_pollfds.push_back(clientPollfd);

	std::cout << "New client connected: fd " << clientFd << std::endl;
}

void Server::start()
{
	// Setup socket
	setupSocket();

	// Set running flag
	_isRunning = true;

	// Set global instance for signal handler
	g_serverInstance = this;

	// Setup signal handlers
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	std::cout << "Server started on port " << _port << std::endl;

	// Main event loop
	while (_isRunning)
	{
		// Poll with 100ms timeout
		int pollResult = poll(&_pollfds[0], _pollfds.size(), 100);

		// Handle poll errors
		if (pollResult == -1)
		{
			if (errno == EINTR)
			{
				// Interrupted by signal, continue
				continue;
			}
			std::cerr << "Poll error: " << strerror(errno) << std::endl;
			break;
		}

		// Process ready file descriptors
		for (size_t i = 0; i < _pollfds.size(); ++i)
		{
			if (_pollfds[i].revents == 0)
				continue;

			int fd = _pollfds[i].fd;

			// Check for errors or hangup
			if (_pollfds[i].revents & (POLLHUP | POLLERR))
			{
				if (fd != _serverSocket)
				{
					removeClient(fd);
				}
				continue;
			}

			// Server socket: new connection
			if (fd == _serverSocket && (_pollfds[i].revents & POLLIN))
			{
				handleNewConnection();
			}
			// Client socket: incoming message
			else if (fd != _serverSocket && (_pollfds[i].revents & POLLIN))
			{
				handleClientMessage(fd);
			}
		}
	}

	// Cleanup
	stop();
}

void Server::stop()
{
	_isRunning = false;
	std::cout << "Server shutting down..." << std::endl;
}

void Server::removeClient(int clientFd)
{
	// Find and remove client from map
	std::map<int, Client*>::iterator it = _clients.find(clientFd);
	if (it != _clients.end())
	{
		delete it->second;
		_clients.erase(it);
	}

	// Remove from pollfds
	for (std::vector<struct pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it)
	{
		if (it->fd == clientFd)
		{
			close(clientFd);
			_pollfds.erase(it);
			break;
		}
	}

	std::cout << "Client disconnected: fd " << clientFd << std::endl;
}

void Server::handleClientMessage(int clientFd)
{
	// Placeholder - will be implemented later
	(void)clientFd;
}

