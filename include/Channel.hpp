#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <vector>
# include <map>

class Client;

class Channel
{
private:
	std::string _name;
	std::string _topic;
	std::string _key;
	std::map<int, Client*> _members;
	std::map<int, bool> _operators; // fd -> isOperator
	std::map<int, bool> _inviteList; // fd -> isInvited
	bool _inviteOnly;
	bool _topicRestricted;
	bool _hasKey;
	bool _hasUserLimit;
	int _userLimit;

	// Orthodox Canonical Form
	Channel();
	Channel(const Channel& other);
	Channel& operator=(const Channel& other);

public:
	Channel(const std::string& name, Client* creator);
	~Channel();

	// Getters
	const std::string& getName() const;
	const std::string& getTopic() const;
	const std::string& getKey() const;
	bool isInviteOnly() const;
	bool isTopicRestricted() const;
	bool hasKey() const;
	bool hasUserLimit() const;
	int getUserLimit() const;
	bool isMember(int clientFd) const;
	bool isOperator(int clientFd) const;
	bool isInvited(int clientFd) const;
	std::vector<Client*> getMembers() const;
	std::string getMembersString() const; // For NAMES list with @ prefix
	std::string getModeString() const; // For MODE query

	// Setters
	void setTopic(const std::string& topic);
	void setKey(const std::string& key);
	void setInviteOnly(bool inviteOnly);
	void setTopicRestricted(bool restricted);
	void setHasKey(bool hasKey);
	void setHasUserLimit(bool hasLimit);
	void setUserLimit(int limit);

	// Member management
	void addMember(Client* client);
	void removeMember(int clientFd);
	void setOperator(int clientFd, bool isOp);
	void addOperator(int clientFd);
	void removeOperator(int clientFd);
	void addToInviteList(int clientFd);
	void removeFromInviteList(int clientFd);
	size_t getMemberCount() const;

	// Broadcasting
	void broadcast(const std::string& message, int excludeFd = -1);
};

#endif

