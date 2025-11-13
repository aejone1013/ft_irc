#include "InviteCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>

InviteCommand::InviteCommand()
{
}

InviteCommand::~InviteCommand()
{
}

void InviteCommand::execute(Server& server, Client& client, const Message& msg)
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
		oss << ":irc.server 461 " << nick << " INVITE :Not enough parameters\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	std::string targetNick = msg.getParam(0);
	std::string channelName = msg.getParam(1);

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

	// Check inviter is on channel
	if (!channel->isMember(client.getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 442 " << nick << " " << channelName << " :You're not on that channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check inviter is operator
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

	// Check target NOT already on channel
	if (channel->isMember(targetClient->getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 443 " << nick << " " << targetNick << " " << channelName << " :is already on channel\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Add to invite list
	channel->addToInviteList(targetClient->getFd());

	// Send RPL_INVITING to inviter
	std::string nick = client.getNickname();
	std::ostringstream oss;
	oss << ":irc.server 341 " << nick << " " << targetNick << " " << channelName << "\r\n";
	server.sendReply(client, oss.str());

	// Send INVITE message to target
	std::string inviterNick = client.getNickname();
	std::string inviterUser = client.getUsername();
	std::string inviterHost = client.getHostname().empty() ? "localhost" : client.getHostname();
	std::ostringstream inviteMsg;
	inviteMsg << ":" << inviterNick << "!" << inviterUser << "@" << inviterHost << " INVITE " << targetNick << " :" << channelName << "\r\n";
	server.sendReply(*targetClient, inviteMsg.str());
}

