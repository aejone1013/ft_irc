#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "CommandHandler.hpp"
#include "PassCommand.hpp"
#include "JoinCommand.hpp"
#include "PartCommand.hpp"
#include "PrivmsgCommand.hpp"
#include "QuitCommand.hpp"
#include "KickCommand.hpp"
#include "TopicCommand.hpp"
#include "InviteCommand.hpp"
#include "ModeCommand.hpp"
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
#include <cctype>

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
	registerCommands();
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

	// Cleanup command handlers
	for (std::map<std::string, CommandHandler*>::iterator it = _commandHandlers.begin(); 
		 it != _commandHandlers.end(); ++it)
	{
		delete it->second;
	}
	_commandHandlers.clear();

	// Cleanup channels
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); 
		 it != _channels.end(); ++it)
	{
		delete it->second;
	}
	_channels.clear();

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

		// Send pending messages to clients
		for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			Client* client = it->second;
			if (client->hasMessageToSend())
			{
				sendToClient(*client);
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
	// Remove client from all channels
	std::vector<Channel*> clientChannels = getChannelsForClient(clientFd);
	for (std::vector<Channel*>::iterator it = clientChannels.begin(); it != clientChannels.end(); ++it)
	{
		(*it)->removeMember(clientFd);
		if ((*it)->getMemberCount() == 0)
		{
			removeChannel((*it)->getName());
		}
	}

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
	// Find client in map
	std::map<int, Client*>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
	{
		std::cerr << "Client not found in map: fd " << clientFd << std::endl;
		return;
	}

	Client* client = it->second;

	// Receive data
	char buffer[512];
	ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	// Check for connection closed
	if (bytesReceived == 0)
	{
		std::cout << "Client disconnected (recv returned 0): fd " << clientFd << std::endl;
		removeClient(clientFd);
		return;
	}

	// Check for errors
	if (bytesReceived == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// No data available, non-blocking socket
			return;
		}
		std::cerr << "Recv error for client fd " << clientFd << ": " << strerror(errno) << std::endl;
		removeClient(clientFd);
		return;
	}

	// Null-terminate the buffer for safety
	buffer[bytesReceived] = '\0';

	// Append received data to client's receive buffer
	std::string receivedData(buffer, bytesReceived);
	client->appendToRecvBuffer(receivedData);

	// Extract and process complete messages
	while (true)
	{
		std::string messageStr = client->extractMessage();
		if (messageStr.empty())
		{
			// No complete message available
			break;
		}

		// Parse message
		Message msg(messageStr);
		
		// Execute command
		executeCommand(*client, msg);
	}
}

void Server::registerCommand(const std::string& cmd, CommandHandler* handler)
{
	_commandHandlers[cmd] = handler;
}

void Server::executeCommand(Client& client, const Message& msg)
{
	std::string command = msg.getCommand();
	if (command.empty())
	{
		return;
	}

	// Lookup command handler
	std::map<std::string, CommandHandler*>::iterator it = _commandHandlers.find(command);
	if (it != _commandHandlers.end())
	{
		it->second->execute(*this, client, msg);
	}
	else
	{
		// Unknown command
		std::string nick = client.getNickname().empty() ? "*" : client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 421 " << nick << " " << command << " :Unknown command\r\n";
		sendReply(client, oss.str());
	}
}

void Server::sendReply(Client& client, const std::string& reply)
{
	client.appendToSendBuffer(reply);
}

const std::string& Server::getPassword() const
{
	return _password;
}

void Server::registerCommands()
{
	// Register commands
	registerCommand("PASS", new PassCommand());
	registerCommand("JOIN", new JoinCommand());
	registerCommand("PART", new PartCommand());
	registerCommand("PRIVMSG", new PrivmsgCommand());
	registerCommand("QUIT", new QuitCommand());
	registerCommand("KICK", new KickCommand());
	registerCommand("TOPIC", new TopicCommand());
	registerCommand("INVITE", new InviteCommand());
	registerCommand("MODE", new ModeCommand());
}

Client* Server::getClientByNickname(const std::string& nickname)
{
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (toLowerCase(it->second->getNickname()) == toLowerCase(nickname))
		{
			return it->second;
		}
	}
	return NULL;
}

Channel* Server::getChannel(const std::string& channelName)
{
	std::string lowerName = toLowerCase(channelName);
	std::map<std::string, Channel*>::iterator it = _channels.find(lowerName);
	if (it != _channels.end())
	{
		return it->second;
	}
	return NULL;
}

Channel* Server::createChannel(const std::string& channelName, Client* creator)
{
	std::string lowerName = toLowerCase(channelName);
	Channel* channel = new Channel(channelName, creator);
	_channels[lowerName] = channel;
	return channel;
}

void Server::removeChannel(const std::string& channelName)
{
	std::string lowerName = toLowerCase(channelName);
	std::map<std::string, Channel*>::iterator it = _channels.find(lowerName);
	if (it != _channels.end())
	{
		delete it->second;
		_channels.erase(it);
	}
}

std::vector<Channel*> Server::getChannelsForClient(int clientFd)
{
	std::vector<Channel*> result;
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->isMember(clientFd))
		{
			result.push_back(it->second);
		}
	}
	return result;
}

bool Server::isValidChannelName(const std::string& name) const
{
	if (name.empty() || name[0] != '#')
	{
		return false;
	}
	if (name.length() < 2 || name.length() > 50)
	{
		return false;
	}
	return true;
}

std::string Server::toLowerCase(const std::string& str) const
{
	std::string result = str;
	for (std::string::size_type i = 0; i < result.length(); ++i)
	{
		result[i] = std::tolower(result[i]);
	}
	return result;
}

void Server::sendToClient(Client& client)
{
	std::string sendBuffer = client.getSendBuffer();
	if (sendBuffer.empty())
	{
		return;
	}

	int clientFd = client.getFd();
	ssize_t bytesSent = send(clientFd, sendBuffer.c_str(), sendBuffer.length(), 0);

	if (bytesSent == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "Send error for client fd " << clientFd << ": " << strerror(errno) << std::endl;
			removeClient(clientFd);
			return;
		}
		// Would block, keep data in buffer for next attempt
		return;
	}

	// Remove sent data from buffer
	if (bytesSent > 0)
	{
		std::string remaining = sendBuffer.substr(bytesSent);
		client.clearSendBuffer();
		if (!remaining.empty())
		{
			client.appendToSendBuffer(remaining);
		}
	}
}

