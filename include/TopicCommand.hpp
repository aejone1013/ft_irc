#ifndef TOPICCOMMAND_HPP
# define TOPICCOMMAND_HPP

# include "CommandHandler.hpp"

class TopicCommand : public CommandHandler
{
public:
	TopicCommand();
	virtual ~TopicCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

