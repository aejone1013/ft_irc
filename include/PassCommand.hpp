#ifndef PASSCOMMAND_HPP
# define PASSCOMMAND_HPP

# include "CommandHandler.hpp"

class PassCommand : public CommandHandler
{
public:
	PassCommand();
	virtual ~PassCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

