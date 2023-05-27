#include "DBService/DBImpl.hpp"
#include <iostream>

namespace {

constexpr const char* const GET_ADMIN_QUERY = "SELECT id, name, photo FROM barbers";
constexpr const char* const GET_USER_QUERY = "SELECT id, name, telegram_id FROM users";
constexpr const char* const GET_SLOTS_QUERY = "select slots from slots where barber_id = 1 and slot_date = '2023-05-28'";
constexpr const int SLOTS_RESERVE = 48;

/*

    0 - not working
    1 - free
    2 - busy

*/
std::vector<std::string> freeTime(const std::string& slots) {
    std::vector<std::string> time;
    time.reserve(slots.size());
    for(int i = 0, j = 0; i <= slots.size()*30; i+=30, ++j) {
        if (slots[j] == '1') {
            time.push_back(std::to_string(i/60) + " : " + (i%60 == 0 ? "00" : "30"));
        }
    }
    return time;
}

} // namespace

namespace db {

DBImpl::DBImpl(Json configs) {
    std::string connectStr;
    connectStr.reserve(1000);
    connectStr = 
    "dbname = " + std::string(configs["dbname"]) +
    " user = " + std::string(configs["user"]) +
    " password = " + std::string(configs["password"]) +
    " hostaddr = " + std::string(configs["hostaddr"]) +
    " port = " + std::string(configs["port"]);

    db_ = std::make_unique<pqxx::connection>(connectStr);
}

std::vector<admin::Admin> DBImpl::getAdmins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::transaction tx{*db_};

    std::vector<admin::Admin> admins;
    admins.reserve(5);
    for (auto const &[id, name, photo] : tx.stream<long, std::string, std::string>(GET_ADMIN_QUERY)) {
        admins.emplace_back(id, name, photo);
    }
    return admins;
}

std::vector<std::string> DBImpl::getSlots(const std::string& date, int barberId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::transaction tx{*db_};

    const auto& [slotsStr] = *tx.stream<std::string>("select slots from slots where barber_id = "+std::to_string(barberId)+" and slot_date = '"+date+"'").begin();
    return freeTime(slotsStr);
}

std::vector<user::User> DBImpl::getUsers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::transaction tx{*db_};

    std::vector<user::User> users;
    users.reserve(5);
    for (auto const &[id, name, telegram_id] : tx.stream<long, std::string, int64_t>(GET_USER_QUERY)) {
        users.emplace_back(id, name, telegram_id);
    }
    return users;
}

int DBImpl::getUserIdByTelegramId(int64_t telegramId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work txn{*db_};
    return txn.query_value<int>("SELECT id FROM users WHERE telegram_id="+std::to_string(telegramId)+";");
}

void DBImpl::createUser(const std::string& name, int64_t telegramId) {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work tx{*db_};
    tx.exec0("INSERT INTO users (name, telegram_id) VALUES ('" + name + "','"+std::to_string(telegramId)+"');");
    tx.commit();
}

void DBImpl::createRecord(int barberId, int userId, const std::string& recordsDate, int timeNum) {
    std::lock_guard<std::mutex> lock(mutex_);
    pqxx::work tx{*db_};
    std::string query = "INSERT INTO records (barber_id, user_id, records_date, time_num) VALUES ('"+std::to_string(barberId)+"','"+std::to_string(userId)+"','"+recordsDate+"','"+std::to_string(timeNum)+"');";
    tx.exec0(query);
    tx.commit();
}

}
