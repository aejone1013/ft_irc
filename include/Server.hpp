#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/socket.h>
# include <poll.h>
# include <vector>
# include <map>
# include <string>

class Client;
class CommandHandler;
class Message;
class Channel;

class Server
{
private:
	int _port;
	std::string _password;
	int _serverSocket;
	std::vector<struct pollfd> _pollfds;
	std::map<int, Client*> _clients;
	std::map<std::string, CommandHandler*> _commandHandlers;
	std::map<std::string, Channel*> _channels;
	bool _isRunning;

	// Orthodox Canonical Form
	Server();
	Server(const Server& other);
	Server& operator=(const Server& other);

	// Private helper methods
	void setupSocket();
	void handleNewConnection();
	void handleClientMessage(int clientFd);
	void registerCommands();
	void sendToClient(Client& client);

public:
	Server(int port, const std::string& password);
	~Server();

	void start();
	void stop();
	
	// Command handling
	void registerCommand(const std::string& cmd, CommandHandler* handler);
	void executeCommand(Client& client, const Message& msg);
	void sendReply(Client& client, const std::string& reply);
	
	// Client management
	void removeClient(int clientFd);
	
	// Getters
	const std::string& getPassword() const;
	Client* getClientByNickname(const std::string& nickname);
	
	// Channel management
	Channel* getChannel(const std::string& channelName);
	Channel* createChannel(const std::string& channelName, Client* creator);
	void removeChannel(const std::string& channelName);
	std::vector<Channel*> getChannelsForClient(int clientFd);
	
	// Helper methods
	bool isValidChannelName(const std::string& name) const;
	std::string toLowerCase(const std::string& str) const;
};

#endif

