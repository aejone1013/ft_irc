#ifndef MESSAGE_HPP
# define MESSAGE_HPP

# include <string>
# include <vector>

class Message
{
private:
	std::string _prefix;
	std::string _command;
	std::vector<std::string> _params;
	std::string _raw;

	// Orthodox Canonical Form
	Message();
	Message(const Message& other);
	Message& operator=(const Message& other);

	// Private parsing method
	void parse();

public:
	Message(const std::string& raw);
	~Message();

	// Getters
	std::string getCommand() const;
	std::vector<std::string> getParams() const;
	std::string getPrefix() const;
	std::string getParam(size_t index) const;
	size_t getParamCount() const;
};

#endif

