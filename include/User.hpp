#pragma once
#include <string>

namespace user {

class User {
public:
    User(int id, std::string name, int64_t telegram_id);
    int id_;
    std::string name_;
    int64_t telegram_id_;
};

} // namespace admin
