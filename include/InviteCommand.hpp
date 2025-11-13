#ifndef INVITECOMMAND_HPP
# define INVITECOMMAND_HPP

# include "CommandHandler.hpp"

class InviteCommand : public CommandHandler
{
public:
	InviteCommand();
	virtual ~InviteCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

