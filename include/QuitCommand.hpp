#ifndef QUITCOMMAND_HPP
# define QUITCOMMAND_HPP

# include "CommandHandler.hpp"

class QuitCommand : public CommandHandler
{
public:
	QuitCommand();
	virtual ~QuitCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

