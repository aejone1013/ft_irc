#ifndef KICKCOMMAND_HPP
# define KICKCOMMAND_HPP

# include "CommandHandler.hpp"

class KickCommand : public CommandHandler
{
public:
	KickCommand();
	virtual ~KickCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

