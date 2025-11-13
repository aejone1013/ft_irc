#include "ModeCommand.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include <sstream>
#include <vector>
#include <cctype>
#include <cstdlib>

ModeCommand::ModeCommand()
{
}

ModeCommand::~ModeCommand()
{
}

std::vector<ModeChange> ModeCommand::parseModeString(const std::string& modeStr, const std::vector<std::string>& params)
{
	std::vector<ModeChange> changes;
	char currentSign = '+';
	size_t paramIndex = 0;

	for (size_t i = 0; i < modeStr.length(); ++i)
	{
		char c = modeStr[i];
		if (c == '+' || c == '-')
		{
			currentSign = c;
		}
		else
		{
			ModeChange change;
			change.sign = currentSign;
			change.mode = c;
			change.param = "";

			// If mode needs param, consume from params vector
			if ((c == 'k' && currentSign == '+') ||
				(c == 'l' && currentSign == '+') ||
				c == 'o')
			{
				if (paramIndex < params.size())
				{
					change.param = params[paramIndex++];
				}
			}

			changes.push_back(change);
		}
	}

	return changes;
}

void ModeCommand::execute(Server& server, Client& client, const Message& msg)
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
		oss << ":irc.server 461 " << nick << " MODE :Not enough parameters\r\n";
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

	// Query mode (1 parameter)
	if (msg.getParamCount() == 1)
	{
		std::string nick = client.getNickname();
		std::string modeStr = channel->getModeString();
		std::ostringstream oss;
		oss << ":irc.server 324 " << nick << " " << channelName << " " << modeStr;
		if (channel->hasKey())
		{
			oss << " " << channel->getKey();
		}
		if (channel->hasUserLimit())
		{
			oss << " " << channel->getUserLimit();
		}
		oss << "\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Set mode (2+ parameters)
	// Check operator privilege
	if (!channel->isOperator(client.getFd()))
	{
		std::string nick = client.getNickname();
		std::ostringstream oss;
		oss << ":irc.server 482 " << nick << " " << channelName << " :You're not channel operator\r\n";
		server.sendReply(client, oss.str());
		return;
	}

	// Parse mode string and parameters
	std::string modeStr = msg.getParam(1);
	std::vector<std::string> modeParams;
	for (size_t i = 2; i < msg.getParamCount(); ++i)
	{
		modeParams.push_back(msg.getParam(i));
	}

	std::vector<ModeChange> changes = parseModeString(modeStr, modeParams);

	// Build response strings
	std::ostringstream appliedModes;
	std::ostringstream appliedParams;
	char lastSign = '\0';

	// Process each mode change
	for (size_t i = 0; i < changes.size(); ++i)
	{
		ModeChange& change = changes[i];
		bool applied = false;
		std::string errorMsg = "";

		// Handle each mode
		switch (change.mode)
		{
			case 'i': // Invite-only
				if (change.sign == '+')
				{
					channel->setInviteOnly(true);
					applied = true;
				}
				else
				{
					channel->setInviteOnly(false);
					applied = true;
				}
				break;

			case 't': // Topic restriction
				if (change.sign == '+')
				{
					channel->setTopicRestricted(true);
					applied = true;
				}
				else
				{
					channel->setTopicRestricted(false);
					applied = true;
				}
				break;

			case 'k': // Channel key
				if (change.sign == '+')
				{
					if (change.param.empty())
					{
						std::string nick = client.getNickname();
						std::ostringstream oss;
						oss << ":irc.server 461 " << nick << " MODE :Not enough parameters\r\n";
						server.sendReply(client, oss.str());
						continue;
					}
					channel->setKey(change.param);
					channel->setHasKey(true);
					applied = true;
				}
				else
				{
					channel->setKey("");
					channel->setHasKey(false);
					applied = true;
				}
				break;

			case 'l': // User limit
				if (change.sign == '+')
				{
					if (change.param.empty())
					{
						std::string nick = client.getNickname();
						std::ostringstream oss;
						oss << ":irc.server 461 " << nick << " MODE :Not enough parameters\r\n";
						server.sendReply(client, oss.str());
						continue;
					}
					int limit = std::atoi(change.param.c_str());
					if (limit > 0)
					{
						channel->setUserLimit(limit);
						channel->setHasUserLimit(true);
						applied = true;
					}
				}
				else
				{
					channel->setUserLimit(0);
					channel->setHasUserLimit(false);
					applied = true;
				}
				break;

			case 'o': // Operator privilege
			{
				if (change.param.empty())
				{
					std::string nick = client.getNickname();
					std::ostringstream oss;
					oss << ":irc.server 461 " << nick << " MODE :Not enough parameters\r\n";
					server.sendReply(client, oss.str());
					continue;
				}
				Client* targetClient = server.getClientByNickname(change.param);
				if (targetClient == NULL)
				{
					std::string nick = client.getNickname();
					std::ostringstream oss;
					oss << ":irc.server 401 " << nick << " " << change.param << " :No such nick/channel\r\n";
					server.sendReply(client, oss.str());
					continue;
				}
				if (!channel->isMember(targetClient->getFd()))
				{
					std::string nick = client.getNickname();
					std::ostringstream oss;
					oss << ":irc.server 441 " << nick << " " << change.param << " " << channelName << " :They aren't on that channel\r\n";
					server.sendReply(client, oss.str());
					continue;
				}
				if (change.sign == '+')
				{
					channel->addOperator(targetClient->getFd());
					applied = true;
				}
				else
				{
					channel->removeOperator(targetClient->getFd());
					applied = true;
				}
				break;
			}

			default:
				// Unknown mode
				std::string nick = client.getNickname();
				std::ostringstream oss;
				oss << ":irc.server 472 " << nick << " " << change.mode << " :is unknown mode char to me\r\n";
				server.sendReply(client, oss.str());
				continue;
		}

		// Build response if applied
		if (applied)
		{
			if (lastSign != change.sign)
			{
				appliedModes << change.sign;
				lastSign = change.sign;
			}
			appliedModes << change.mode;

			if (!change.param.empty() && (change.mode == 'k' || change.mode == 'l' || change.mode == 'o'))
			{
				if (!appliedParams.str().empty())
					appliedParams << " ";
				appliedParams << change.param;
			}
		}
	}

	// Broadcast MODE change if any modes were applied
	if (!appliedModes.str().empty())
	{
		std::string nick = client.getNickname();
		std::string user = client.getUsername();
		std::string host = client.getHostname().empty() ? "localhost" : client.getHostname();
		std::ostringstream modeMsg;
		modeMsg << ":" << nick << "!" << user << "@" << host << " MODE " << channelName << " " << appliedModes.str();
		if (!appliedParams.str().empty())
		{
			modeMsg << " " << appliedParams.str();
		}
		modeMsg << "\r\n";
		channel->broadcast(modeMsg.str());
	}
}

