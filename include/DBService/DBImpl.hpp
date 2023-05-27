#pragma once

#include "DB.hpp"
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

#include <memory>
#include <mutex>

namespace db {

using Json = nlohmann::json;

class DBImpl : public DB {
public:
    DBImpl(Json configs);
    std::vector<admin::Admin> getAdmins() const override;
    std::vector<std::string> getSlots(const std::string& date, int barberId) const;
    std::vector<user::User> getUsers() const;
    int getUserIdByTelegramId(int64_t telegramId) const;

    void createUser(const std::string& name, int64_t telegramId);
    void createRecord(int barberId, int userId, const std::string& recordsDate, int timeNum);

    ~DBImpl() = default;

private:
    mutable std::mutex mutex_;
    std::unique_ptr<pqxx::connection> db_;
};

} // namespace db
