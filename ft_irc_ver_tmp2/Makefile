NAME := ircserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98

INCDIR := ./include
SRCDIR := ./srcs
OBJDIR := ./objs

SRC := \
		ChannelRegistry.cpp \
		ClientRegistry.cpp \
		IrcParser.cpp \
		Signal.cpp \
		Error.cpp \
		Fd.cpp \
		SocketMonitor.cpp \
		IrcServerInfo.cpp \
		IrcMessageBuilder.cpp \
		Utils.cpp \
		Server.cpp \
		IrcCore.cpp \
		IrcCoreSupport.cpp \
		IrcCoreProtocol.cpp \
		IrcCoreRegistration.cpp \
		IrcCoreChannel.cpp \
		main.cpp \

SRCS := $(addprefix $(SRCDIR)/, $(SRC))
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -MMD -MP -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(DEPS)
