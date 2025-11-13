#include "JoinCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>
#include <vector>

JoinCommand::JoinCommand()
{
}

JoinCommand::~JoinCommand()
{
}

static void splitString(const std::string& str, char delimiter, std::vector<std::string>& result)
{
	std::string::size_type start = 0;
	std::string::size_type pos = str.find(delimiter);
	
	while (pos != std::string::npos)
	{
		result.push_back(str.substr(start, pos - start));
		start = pos + 1;
		pos = str.find(delimiter, start);
	}
	result.push_back(str.substr(start));
}

void JoinCommand::execute(Server& server, Client& client, const Message& msg)
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
		oss << ":irc.server 461 " << nick << " JOIN :Not enough parameters\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Parse channels and keys
	std::vector<std::string> channels;
	std::vector<std::string> keys;
	
	splitString(msg.getParam(0), ',', channels);
	if (msg.getParamCount() > 1)
	{
		splitString(msg.getParam(1), ',', keys);
	}

	// Process each channel
	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::string channelName = channels[i];
		std::string key = (i < keys.size()) ? keys[i] : "";

		// Validate channel name
		if (!server.isValidChannelName(channelName))
		{
			std::string nick = client.getNickname();
			std::ostringstream oss;
			oss << ":irc.server 403 " << nick << " " << channelName << " :No such channel\r\n";
			server.sendReply(client, oss.str());
			continue;
		}

		// Get or create channel
		Channel* channel = server.getChannel(channelName);
		if (channel == NULL)
		{
			// Create new channel
			channel = server.createChannel(channelName, &client);
		}
		else
		{
			// Check if already member
			if (channel->isMember(client.getFd()))
			{
				continue;
			}

			// Check invite-only
			if (channel->isInviteOnly())
			{
				if (!channel->isInvited(client.getFd()))
				{
					std::string nick = client.getNickname();
					std::ostringstream oss;
					oss << ":irc.server 473 " << nick << " " << channelName << " :Cannot join channel (+i)\r\n";
					server.sendReply(client, oss.str());
					continue;
				}
			}

			// Check key
			if (channel->hasKey() && channel->getKey() != key)
			{
				std::string nick = client.getNickname();
				std::ostringstream oss;
				oss << ":irc.server 475 " << nick << " " << channelName << " :Cannot join channel (+k)\r\n";
				server.sendReply(client, oss.str());
				continue;
			}

			// Check user limit
			if (channel->hasUserLimit() && 
				static_cast<int>(channel->getMemberCount()) >= channel->getUserLimit())
			{
				std::string nick = client.getNickname();
				std::ostringstream oss;
				oss << ":irc.server 471 " << nick << " " << channelName << " :Cannot join channel (+l)\r\n";
				server.sendReply(client, oss.str());
				continue;
			}
		}

		// Add client to channel
		channel->addMember(&client);
		
		// Remove from invite list if was invited
		if (channel->isInvited(client.getFd()))
		{
			channel->removeFromInviteList(client.getFd());
		}

		// Build JOIN message
		std::string nick = client.getNickname();
		std::string user = client.getUsername();
		std::string host = client.getHostname().empty() ? "localhost" : client.getHostname();
		std::ostringstream joinMsg;
		joinMsg << ":" << nick << "!" << user << "@" << host << " JOIN :" << channelName << "\r\n";

		// Broadcast to all channel members
		channel->broadcast(joinMsg.str());

		// Send topic or no topic
		if (!channel->getTopic().empty())
		{
			std::ostringstream topicMsg;
			topicMsg << ":irc.server 332 " << nick << " " << channelName << " :" << channel->getTopic() << "\r\n";
			server.sendReply(client, topicMsg.str());
		}
		else
		{
			std::ostringstream notopicMsg;
			notopicMsg << ":irc.server 331 " << nick << " " << channelName << " :No topic is set\r\n";
			server.sendReply(client, notopicMsg.str());
		}

		// Send names list
		std::ostringstream namesMsg;
		namesMsg << ":irc.server 353 " << nick << " = " << channelName << " :" << channel->getMembersString() << "\r\n";
		server.sendReply(client, namesMsg.str());

		std::ostringstream endNamesMsg;
		endNamesMsg << ":irc.server 366 " << nick << " " << channelName << " :End of /NAMES list\r\n";
		server.sendReply(client, endNamesMsg.str());
	}
}

