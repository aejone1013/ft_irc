#include "KickCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>

KickCommand::KickCommand()
{
}

KickCommand::~KickCommand()
{
}

void KickCommand::execute(Server& server, Client& client, const Message& msg)
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
	if (msg.getParamCount() < 2)
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 461 " << nick << " KICK :Not enough parameters\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	std::string channelName = msg.getParam(0);
	std::string targetNick = msg.getParam(1);
	std::string comment = targetNick; // Default comment

	if (msg.getParamCount() > 2)
	{
		comment = msg.getParam(2);
		for (size_t i = 3; i < msg.getParamCount(); ++i)
		{
			comment += " " + msg.getParam(i);
		}
	}

	// Validate channel exists
	Channel* channel = server.getChannel(channelName);
	if (channel == NULL)
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 403 " << nick << " " << channelName << " :No such channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check kicker is on channel
	if (!channel->isMember(client.getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 442 " << nick << " " << channelName << " :You're not on that channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check kicker is operator
	if (!channel->isOperator(client.getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 482 " << nick << " " << channelName << " :You're not channel operator\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Find target client
	Client* targetClient = server.getClientByNickname(targetNick);
	if (targetClient == NULL)
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 401 " << nick << " " << targetNick << " :No such nick/channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check target is on channel
	if (!channel->isMember(targetClient->getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 441 " << nick << " " << targetNick << " " << channelName << " :They aren't on that channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Build KICK message
	std::string kickerNick = client.getNickname();
	std::string kickerUser = client.getUsername();
	std::string kickerHost = client.getHostname().empty() ? "localhost" : client.getHostname();
	std::ostringstream kickMsg;
	kickMsg << ":" << kickerNick << "!" << kickerUser << "@" << kickerHost << " KICK " << channelName << " " << targetNick << " :" << comment << "\r\n";

	// Broadcast to all channel members
	channel->broadcast(kickMsg.str());

	// Remove target from channel
	channel->removeMember(targetClient->getFd());
	channel->removeFromInviteList(targetClient->getFd());

	// If channel is now empty, remove it
	if (channel->getMemberCount() == 0)
	{
		server.removeChannel(channelName);
	}
}

