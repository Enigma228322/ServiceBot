#pragma once
#include <string>

namespace admin {

class Admin {
public:
    Admin() = default;
    Admin(int id, std::string name, std::string photo);
    int id_;
    std::string name_;
    std::string photo_;
};

} // namespace admin
