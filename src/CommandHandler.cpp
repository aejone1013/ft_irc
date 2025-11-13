#include "CommandHandler.hpp"
#include "Message.hpp"

CommandHandler::~CommandHandler()
{
}

bool CommandHandler::validateParamCount(const Message& msg, size_t requiredCount) const
{
	return msg.getParamCount() >= requiredCount;
}

