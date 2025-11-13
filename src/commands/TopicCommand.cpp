#include "TopicCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>

TopicCommand::TopicCommand()
{
}

TopicCommand::~TopicCommand()
{
}

void TopicCommand::execute(Server& server, Client& client, const Message& msg)
{
	// Check if registered
	if (!client.isRegistered())
	{
		std::string nick = client.getNickname().empty() ? "*" : client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 451 " << nick << " :You have not registered\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check parameters
	if (msg.getParamCount() == 0)
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 461 " << nick << " TOPIC :Not enough parameters\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	std::string channelName = msg.getParam(0);

	// Get channel
	Channel* channel = server.getChannel(channelName);
	if (channel == NULL)
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 403 " << nick << " " << channelName << " :No such channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check membership
	if (!channel->isMember(client.getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 442 " << nick << " " << channelName << " :You're not on that channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Query mode (1 parameter)
	if (msg.getParamCount() == 1)
	{
		if (!channel->getTopic().empty())
		{
			std::string nick = client.getNickname();
			std::ostringstream oss;
			oss << ":irc.server 332 " << nick << " " << channelName << " :" << channel->getTopic() << "\r\n";
			server.sendReply(client, oss.str());
		}
		else
		{
			std::string nick = client.getNickname();
			std::ostringstream oss;
			oss << ":irc.server 331 " << nick << " " << channelName << " :No topic is set\r\n";
			server.sendReply(client, oss.str());
		}
		return;
	}

	// Set mode (2+ parameters)
	// Extract new topic
	std::string newTopic = msg.getParam(1);
	for (size_t i = 2; i < msg.getParamCount(); ++i)
	{
		newTopic += " " + msg.getParam(i);
	}

	// Check if topic restricted and client not operator
	if (channel->isTopicRestricted() && !channel->isOperator(client.getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 482 " << nick << " " << channelName << " :You're not channel operator\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Set topic
	channel->setTopic(newTopic);

	// Broadcast to all members
	std::string nick = client.getNickname();
	std::string user = client.getUsername();
	std::string host = client.getHostname().empty() ? "localhost" : client.getHostname();
	std::ostringstream topicMsg;
	topicMsg << ":" << nick << "!" << user << "@" << host << " TOPIC " << channelName << " :" << newTopic << "\r\n";
	channel->broadcast(topicMsg.str());
}

