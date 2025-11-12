#include "Client.hpp"

Client::Client(int fd)
	: _fd(fd), _authenticated(false), _registered(false)
{
}

Client::~Client()
{
}

// Getters
int Client::getFd() const
{
	return _fd;
}

const std::string& Client::getNickname() const
{
	return _nickname;
}

const std::string& Client::getUsername() const
{
	return _username;
}

const std::string& Client::getRealname() const
{
	return _realname;
}

const std::string& Client::getHostname() const
{
	return _hostname;
}

bool Client::isAuthenticated() const
{
	return _authenticated;
}

bool Client::isRegistered() const
{
	return _registered;
}

const std::string& Client::getRecvBuffer() const
{
	return _recvBuffer;
}

// Setters
void Client::setNickname(const std::string& nickname)
{
	_nickname = nickname;
}

void Client::setUsername(const std::string& username)
{
	_username = username;
}

void Client::setRealname(const std::string& realname)
{
	_realname = realname;
}

void Client::setAuthenticated(bool authenticated)
{
	_authenticated = authenticated;
}

void Client::setRegistered(bool registered)
{
	_registered = registered;
}

// Buffer management
void Client::appendToRecvBuffer(const std::string& data)
{
	_recvBuffer += data;
}

std::string Client::extractMessage()
{
	std::string::size_type pos = _recvBuffer.find("\r\n");
	if (pos == std::string::npos)
	{
		return std::string();
	}

	std::string message = _recvBuffer.substr(0, pos);
	_recvBuffer.erase(0, pos + 2);
	return message;
}

void Client::appendToSendBuffer(const std::string& message)
{
	_sendBuffer += message;
}

bool Client::hasMessageToSend() const
{
	return !_sendBuffer.empty();
}

std::string Client::getSendBuffer() const
{
	return _sendBuffer;
}

void Client::clearSendBuffer()
{
	_sendBuffer.clear();
}

