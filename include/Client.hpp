#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

class Client
{
private:
	int _fd;
	std::string _nickname;
	std::string _username;
	std::string _realname;
	std::string _hostname;
	bool _authenticated;
	bool _registered;
	std::string _recvBuffer;
	std::string _sendBuffer;

	// Orthodox Canonical Form
	Client();
	Client(const Client& other);
	Client& operator=(const Client& other);

public:
	Client(int fd);
	~Client();

	// Getters
	int getFd() const;
	const std::string& getNickname() const;
	const std::string& getUsername() const;
	const std::string& getRealname() const;
	const std::string& getHostname() const;
	bool isAuthenticated() const;
	bool isRegistered() const;
	const std::string& getRecvBuffer() const;

	// Setters
	void setNickname(const std::string& nickname);
	void setUsername(const std::string& username);
	void setRealname(const std::string& realname);
	void setAuthenticated(bool authenticated);
	void setRegistered(bool registered);

	// Buffer management
	void appendToRecvBuffer(const std::string& data);
	std::string extractMessage();
	void appendToSendBuffer(const std::string& message);
	bool hasMessageToSend() const;
	std::string getSendBuffer() const;
	void clearSendBuffer();
};

#endif

