#pragma once

#include "Admin.hpp"
#include "User.hpp"

#include <vector>

namespace db {

class DB {
public:
    virtual std::vector<admin::Admin> getAdmins() const = 0;
    virtual std::vector<user::User> getUsers() const = 0;
    virtual std::vector<std::string> getSlots(const std::string& date, int barberId) const = 0;
    virtual int getUserIdByTelegramId(int64_t telegramId) const = 0;

    virtual void createUser(const std::string& name, int64_t telegramId) = 0;
    virtual void createRecord(int barberId, int userId, const std::string& recordsDate, int timeNum) = 0;
};

} //namespace db
