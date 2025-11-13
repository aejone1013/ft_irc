#ifndef PRIVMSGCOMMAND_HPP
# define PRIVMSGCOMMAND_HPP

# include "CommandHandler.hpp"

class PrivmsgCommand : public CommandHandler
{
public:
	PrivmsgCommand();
	virtual ~PrivmsgCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

