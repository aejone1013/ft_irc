#include "PartCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>
#include <vector>

PartCommand::PartCommand()
{
}

PartCommand::~PartCommand()
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

void PartCommand::execute(Server& server, Client& client, const Message& msg)
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
		oss << ":irc.server 461 " << nick << " PART :Not enough parameters\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Parse channels
	std::vector<std::string> channels;
	splitString(msg.getParam(0), ',', channels);

	// Extract reason
	std::string reason = client.getNickname();
	if (msg.getParamCount() > 1)
	{
		reason = msg.getParam(1);
		for (size_t i = 2; i < msg.getParamCount(); ++i)
		{
			reason += " " + msg.getParam(i);
		}
	}

	// Process each channel
	for (size_t i = 0; i < channels.size(); ++i)
	{
		std::string channelName = channels[i];

		// Validate channel name
		if (!server.isValidChannelName(channelName))
		{
			std::string nick = client.getNickname();
			std::ostringstream oss;
			oss << ":irc.server 403 " << nick << " " << channelName << " :No such channel\r\n";
			server.sendReply(client, oss.str());
			continue;
		}

		// Check if channel exists
		Channel* channel = server.getChannel(channelName);
		if (channel == NULL)
		{
			std::string nick = client.getNickname();
			std::ostringstream oss;
			oss << ":irc.server 403 " << nick << " " << channelName << " :No such channel\r\n";
			server.sendReply(client, oss.str());
			continue;
		}

		// Check if client is member
		if (!channel->isMember(client.getFd()))
		{
			std::string nick = client.getNickname();
			std::ostringstream oss;
			oss << ":irc.server 442 " << nick << " " << channelName << " :You're not on that channel\r\n";
			server.sendReply(client, oss.str());
			continue;
		}

		// Build PART message
		std::string nick = client.getNickname();
		std::string user = client.getUsername();
		std::string host = client.getHostname().empty() ? "localhost" : client.getHostname();
		std::ostringstream partMsg;
		partMsg << ":" << nick << "!" << user << "@" << host << " PART " << channelName << " :" << reason << "\r\n";

		// Broadcast to channel
		channel->broadcast(partMsg.str());

		// Remove client from channel
		channel->removeMember(client.getFd());

		// If channel is now empty, remove it
		if (channel->getMemberCount() == 0)
		{
			server.removeChannel(channelName);
		}
	}
}

