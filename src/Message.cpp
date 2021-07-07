#include "../include/BullyAlgo/Message.h"

bool
operator<(const NodeInfo& lhs, const NodeInfo& rhs)
{
    return lhs.uuid < rhs.uuid;
}

std::ostream&
operator<<(std::ostream& os, const NodeInfo& info)
{
    std::string ip(info.ip, info.ip + strlen(info.ip)), port(info.port, info.port + strlen(info.port));
    os << ip << " " << port << " " << info.uuid << " ";
    return os;
}

std::istream&
operator>>(std::istream& is, NodeInfo& info)
{
    memset(info.ip, '\0', sizeof(info.ip));
    memset(info.port, '\0', sizeof(info.port));
    is >> info.ip >> info.port >> info.uuid;
    // if (!info.isValid()) { is.setstate(std::ios::failbit); }
    return is;
}