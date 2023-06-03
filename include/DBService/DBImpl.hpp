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
    std::vector<std::string> getSlots(const std::string& date, int barberId) const override;
    std::vector<user::User> getUsers() const override;
    int getUserIdByTelegramId(int64_t telegramId) const override;

    void createUser(const std::string& name, int64_t telegramId) override;
    void createRecord(int barberId, int userId, const std::string& recordsDate, int timeNum) override;
    void createSlot(const std::vector<std::string>& nextDates, int adminId) override;

    void updateSlots(int barberId, int timeNum, const std::string& date) override;

    void dropRecord(int userId) const override;

    ~DBImpl() = default;

private:
    mutable std::mutex mutex_;
    std::unique_ptr<pqxx::connection> db_;
};

} // namespace db
