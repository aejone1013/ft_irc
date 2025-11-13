#include "PrivmsgCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>
#include <vector>

PrivmsgCommand::PrivmsgCommand()
{
}

PrivmsgCommand::~PrivmsgCommand()
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

void PrivmsgCommand::execute(Server& server, Client& client, const Message& msg)
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
		oss << ":irc.server 411 " << nick << " :No recipient given (PRIVMSG)\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	if (msg.getParamCount() == 1)
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 412 " << nick << " :No text to send\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Parse targets
	std::vector<std::string> targets;
	splitString(msg.getParam(0), ',', targets);

	// Build message
	std::string message = msg.getParam(1);
	for (size_t i = 2; i < msg.getParamCount(); ++i)
	{
		message += " " + msg.getParam(i);
	}

	// Build sender prefix
	std::string nick = client.getNickname();
	std::string user = client.getUsername();
	std::string host = client.getHostname().empty() ? "localhost" : client.getHostname();

	// Process each target
	for (size_t i = 0; i < targets.size(); ++i)
	{
		std::string target = targets[i];

		if (target[0] == '#')
		{
			// Channel message
			Channel* channel = server.getChannel(target);
			if (channel == NULL)
			{
				std::ostringstream oss;
				oss << ":irc.server 401 " << nick << " " << target << " :No such nick/channel\r\n";
				server.sendReply(client, oss.str());
				continue;
			}

			// Check if sender is member
			if (!channel->isMember(client.getFd()))
			{
				std::ostringstream oss;
				oss << ":irc.server 404 " << nick << " " << target << " :Cannot send to channel\r\n";
				server.sendReply(client, oss.str());
				continue;
			}

			// Broadcast to channel excluding sender
			std::ostringstream privmsgMsg;
			privmsgMsg << ":" << nick << "!" << user << "@" << host << " PRIVMSG " << target << " :" << message << "\r\n";
			channel->broadcast(privmsgMsg.str(), client.getFd());
		}
		else
		{
			// Private message
			Client* targetClient = server.getClientByNickname(target);
			if (targetClient == NULL)
			{
				std::ostringstream oss;
				oss << ":irc.server 401 " << nick << " " << target << " :No such nick/channel\r\n";
				server.sendReply(client, oss.str());
				continue;
			}

			// Send to target client
			std::ostringstream privmsgMsg;
			privmsgMsg << ":" << nick << "!" << user << "@" << host << " PRIVMSG " << target << " :" << message << "\r\n";
			server.sendReply(*targetClient, privmsgMsg.str());
		}
	}
}

