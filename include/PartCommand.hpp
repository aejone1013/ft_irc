#ifndef PARTCOMMAND_HPP
# define PARTCOMMAND_HPP

# include "CommandHandler.hpp"

class PartCommand : public CommandHandler
{
public:
	PartCommand();
	virtual ~PartCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

