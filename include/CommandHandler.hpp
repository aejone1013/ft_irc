#ifndef COMMANDHANDLER_HPP
# define COMMANDHANDLER_HPP

# include <string>
# include <vector>

class Server;
class Client;
class Message;

class CommandHandler
{
protected:
	bool validateParamCount(const Message& msg, size_t requiredCount) const;

public:
	virtual ~CommandHandler();
	virtual void execute(Server& server, Client& client, const Message& msg) = 0;
};

#endif

