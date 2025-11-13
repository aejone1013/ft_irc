#ifndef JOINCOMMAND_HPP
# define JOINCOMMAND_HPP

# include "CommandHandler.hpp"

class JoinCommand : public CommandHandler
{
public:
	JoinCommand();
	virtual ~JoinCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

