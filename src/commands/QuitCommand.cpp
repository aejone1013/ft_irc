#include "QuitCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>
#include <vector>

QuitCommand::QuitCommand()
{
}

QuitCommand::~QuitCommand()
{
}

void QuitCommand::execute(Server& server, Client& client, const Message& msg)
{
	// Extract quit message
	std::string quitMsg = "Client quit";
	if (msg.getParamCount() > 0)
	{
		quitMsg = msg.getParam(0);
		for (size_t i = 1; i < msg.getParamCount(); ++i)
		{
			quitMsg += " " + msg.getParam(i);
		}
	}

	// Get all channels client is member of
	std::vector<Channel*> channels = server.getChannelsForClient(client.getFd());

	// Build QUIT message
	std::string nick = client.getNickname();
	std::string user = client.getUsername();
	std::string host = client.getHostname().empty() ? "localhost" : client.getHostname();
	std::ostringstream quitBroadcast;
	quitBroadcast << ":" << nick << "!" << user << "@" << host << " QUIT :" << quitMsg << "\r\n";

	// Broadcast to all channels
	for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		(*it)->broadcast(quitBroadcast.str());
		(*it)->removeMember(client.getFd());
		
		// If channel is now empty, remove it
		if ((*it)->getMemberCount() == 0)
		{
			server.removeChannel((*it)->getName());
		}
	}

	// Send ERROR message to client
	std::ostringstream errorMsg;
	errorMsg << "ERROR :Closing Link: " << host << " (" << quitMsg << ")\r\n";
	server.sendReply(client, errorMsg.str());

	// Remove client
	server.removeClient(client.getFd());
}

