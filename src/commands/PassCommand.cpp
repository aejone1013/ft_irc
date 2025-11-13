#include "PassCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Message.hpp"
#include <sstream>

PassCommand::PassCommand()
{
}

PassCommand::~PassCommand()
{
}

void PassCommand::execute(Server& server, Client& client, const Message& msg)
{
	// Check if client is already registered
	if (client.isRegistered())
	{
		std::string nick = client.getNickname().empty() ? "*" : client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 462 " << nick << " :You may not reregister\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Check parameter count
	if (!validateParamCount(msg, 1))
	{
		std::ostringstream oss;
		oss << ":irc.server 461 * PASS :Not enough parameters\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Validate password
	std::string providedPassword = msg.getParam(0);
	if (providedPassword == server.getPassword())
	{
		client.setAuthenticated(true);
	}
	else
	{
		std::ostringstream oss;
		oss << ":irc.server 464 * :Password incorrect\r\n";
		server.sendReply(client, oss.str());
	}
}

