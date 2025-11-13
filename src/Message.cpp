#include "Message.hpp"
#include <cctype>

Message::Message(const std::string& raw)
	: _raw(raw)
{
	parse();
}

Message::~Message()
{
}

void Message::parse()
{
	if (_raw.empty())
	{
		return;
	}

	std::string message = _raw;
	std::string::size_type pos = 0;

	// Check for prefix (starts with ':')
	if (!message.empty() && message[0] == ':')
	{
		pos = message.find(' ', 1);
		if (pos != std::string::npos)
		{
			_prefix = message.substr(1, pos - 1);
			pos++; // Skip the space
		}
		else
		{
			// No space found, entire message is prefix
			_prefix = message.substr(1);
			return;
		}
	}

	// Extract command (next word)
	std::string::size_type commandStart = pos;
	std::string::size_type commandEnd = message.find(' ', commandStart);
	
	if (commandEnd == std::string::npos)
	{
		// No space found, rest is command
		_command = message.substr(commandStart);
	}
	else
	{
		_command = message.substr(commandStart, commandEnd - commandStart);
		pos = commandEnd + 1;
	}

	// Convert command to uppercase
	for (std::string::size_type i = 0; i < _command.length(); ++i)
	{
		_command[i] = std::toupper(_command[i]);
	}

	// Extract parameters
	while (pos < message.length())
	{
		// Skip leading spaces
		while (pos < message.length() && message[pos] == ' ')
		{
			pos++;
		}

		if (pos >= message.length())
		{
			break;
		}

		// Check for trailing parameter (starts with ':')
		if (message[pos] == ':')
		{
			// Rest of the line is one parameter
			std::string trailingParam = message.substr(pos + 1);
			if (!trailingParam.empty())
			{
				_params.push_back(trailingParam);
			}
			break;
		}

		// Regular parameter (until next space)
		std::string::size_type paramStart = pos;
		std::string::size_type paramEnd = message.find(' ', paramStart);
		
		if (paramEnd == std::string::npos)
		{
			// No space found, rest is parameter
			std::string param = message.substr(paramStart);
			if (!param.empty())
			{
				_params.push_back(param);
			}
			break;
		}
		else
		{
			std::string param = message.substr(paramStart, paramEnd - paramStart);
			if (!param.empty())
			{
				_params.push_back(param);
			}
			pos = paramEnd + 1;
		}
	}
}

std::string Message::getCommand() const
{
	return _command;
}

std::vector<std::string> Message::getParams() const
{
	return _params;
}

std::string Message::getPrefix() const
{
	return _prefix;
}

std::string Message::getParam(size_t index) const
{
	if (index >= _params.size())
	{
		return std::string();
	}
	return _params[index];
}

size_t Message::getParamCount() const
{
	return _params.size();
}

