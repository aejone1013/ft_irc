#include "Channel.hpp"
#include "Client.hpp"
#include <algorithm>
#include <sstream>

Channel::Channel(const std::string& name, Client* creator)
	: _name(name), _inviteOnly(false), _topicRestricted(false), _hasKey(false), _hasUserLimit(false), _userLimit(0)
{
	if (creator != NULL)
	{
		_members[creator->getFd()] = creator;
		_operators[creator->getFd()] = true;
	}
}

Channel::~Channel()
{
}

const std::string& Channel::getName() const
{
	return _name;
}

const std::string& Channel::getTopic() const
{
	return _topic;
}

const std::string& Channel::getKey() const
{
	return _key;
}

bool Channel::isInviteOnly() const
{
	return _inviteOnly;
}

int Channel::getUserLimit() const
{
	return _userLimit;
}

bool Channel::isMember(int clientFd) const
{
	return _members.find(clientFd) != _members.end();
}

bool Channel::isOperator(int clientFd) const
{
	std::map<int, bool>::const_iterator it = _operators.find(clientFd);
	if (it != _operators.end())
	{
		return it->second;
	}
	return false;
}

std::vector<Client*> Channel::getMembers() const
{
	std::vector<Client*> members;
	for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it)
	{
		members.push_back(it->second);
	}
	return members;
}

std::string Channel::getMembersString() const
{
	std::ostringstream oss;
	bool first = true;
	for (std::map<int, Client*>::const_iterator it = _members.begin(); it != _members.end(); ++it)
	{
		if (!first)
		{
			oss << " ";
		}
		first = false;
		
		if (isOperator(it->first))
		{
			oss << "@";
		}
		oss << it->second->getNickname();
	}
	return oss.str();
}

void Channel::setTopic(const std::string& topic)
{
	_topic = topic;
}

void Channel::setKey(const std::string& key)
{
	_key = key;
}

void Channel::setInviteOnly(bool inviteOnly)
{
	_inviteOnly = inviteOnly;
}

void Channel::setUserLimit(int limit)
{
	_userLimit = limit;
}

bool Channel::isTopicRestricted() const
{
	return _topicRestricted;
}

bool Channel::hasKey() const
{
	return _hasKey;
}

bool Channel::hasUserLimit() const
{
	return _hasUserLimit;
}

void Channel::setTopicRestricted(bool restricted)
{
	_topicRestricted = restricted;
}

void Channel::setHasKey(bool hasKey)
{
	_hasKey = hasKey;
}

void Channel::setHasUserLimit(bool hasLimit)
{
	_hasUserLimit = hasLimit;
}

bool Channel::isInvited(int clientFd) const
{
	return _inviteList.find(clientFd) != _inviteList.end();
}

void Channel::addToInviteList(int clientFd)
{
	_inviteList[clientFd] = true;
}

void Channel::removeFromInviteList(int clientFd)
{
	_inviteList.erase(clientFd);
}

void Channel::addOperator(int clientFd)
{
	_operators[clientFd] = true;
}

void Channel::removeOperator(int clientFd)
{
	_operators[clientFd] = false;
}

std::string Channel::getModeString() const
{
	std::ostringstream oss;
	oss << "+";
	if (_inviteOnly)
		oss << "i";
	if (_topicRestricted)
		oss << "t";
	if (_hasKey)
		oss << "k";
	if (_hasUserLimit)
		oss << "l";
	
	std::string modes = oss.str();
	if (modes == "+")
		return "+";
	return modes;
}

void Channel::addMember(Client* client)
{
	if (client != NULL)
	{
		_members[client->getFd()] = client;
		_operators[client->getFd()] = false;
	}
}

void Channel::removeMember(int clientFd)
{
	_members.erase(clientFd);
	_operators.erase(clientFd);
	_inviteList.erase(clientFd);
}

void Channel::setOperator(int clientFd, bool isOp)
{
	_operators[clientFd] = isOp;
}

size_t Channel::getMemberCount() const
{
	return _members.size();
}

void Channel::broadcast(const std::string& message, int excludeFd)
{
	for (std::map<int, Client*>::iterator it = _members.begin(); it != _members.end(); ++it)
	{
		if (it->first != excludeFd)
		{
			it->second->appendToSendBuffer(message);
		}
	}
}

