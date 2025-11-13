# Makefile for ft_irc project
# C++98 compliant IRC server

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I./include

# Directories
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

# Target executable
TARGET = ircserv

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/commands/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/commands
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean object files
clean:
	rm -rf $(OBJ_DIR)

# Full clean (objects and executable)
fclean: clean
	rm -f $(TARGET)

# Rebuild everything
re: fclean all

# Valgrind memory check
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes \
	./$(TARGET) 6667 test

.PHONY: all clean fclean re valgrind

