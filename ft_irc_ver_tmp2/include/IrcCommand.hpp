#ifndef IRCCOMMAND_HPP
#define IRCCOMMAND_HPP

#include <string>
#include <vector>

struct IrcCommand {
    std::string                 raw_Line;
    std::string                 prefix;
    std::string                 verb;
    std::vector<std::string>    params;
    std::string                 trailing;

    bool                        hasTrailing;
};

#endif