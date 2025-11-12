#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/socket.h>
# include <poll.h>
# include <vector>
# include <map>
# include <string>

class Client;

class Server
{
private:
	int _port;
	std::string _password;
	int _serverSocket;
	std::vector<struct pollfd> _pollfds;
	std::map<int, Client*> _clients;
	bool _isRunning;

	// Orthodox Canonical Form
	Server();
	Server(const Server& other);
	Server& operator=(const Server& other);

	// Private helper methods
	void setupSocket();
	void handleNewConnection();
	void handleClientMessage(int clientFd);
	void removeClient(int clientFd);

public:
	Server(int port, const std::string& password);
	~Server();

	void start();
	void stop();
};

#endif

