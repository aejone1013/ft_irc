#ifndef MODECOMMAND_HPP
# define MODECOMMAND_HPP

# include "CommandHandler.hpp"
# include <string>
# include <vector>

struct ModeChange
{
	char sign; // '+' or '-'
	char mode; // 'i', 't', 'k', 'o', 'l'
	std::string param; // empty if no param needed
};

class ModeCommand : public CommandHandler
{
private:
	std::vector<ModeChange> parseModeString(const std::string& modeStr, const std::vector<std::string>& params);

public:
	ModeCommand();
	virtual ~ModeCommand();
	virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif

