#include <iostream>

#include "DBService/DBImpl.hpp"
#include "utils/shit.hpp"

#include "spdlog/spdlog.h"

namespace {

constexpr const char* const GET_ADMIN_QUERY = "SELECT id, name, photo FROM barbers";
constexpr const char* const GET_USER_QUERY = "SELECT id, name, telegram_id FROM users";
constexpr const int SLOTS_RESERVE = 48;

/*

    0 - not working
    1 - free
    2 - busy

*/
std::vector<std::string> freeTime(const std::string& slots) {
    std::vector<std::string> time;
    time.reserve(SLOTS_RESERVE);
    for(int i = 0, j = 0; i < SLOTS_RESERVE*30; i+=30, ++j) {
        if (slots[j] == '1') {
            time.push_back(std::to_string(i/60) + " : " + (i%60 == 0 ? "00" : "30"));
        }
    }
    return time;
}

} // namespace

namespace db {

DBImpl::DBImpl(Json configs) {
    try {
        std::string connectStr;
        connectStr.reserve(1000);
        connectStr = 
        "dbname = " + configs["dbname"].get<std::string>() +
        " user = " + configs["user"].get<std::string>() +
        " password = " + configs["password"].get<std::string>() +
        " hostaddr = " + configs["hostaddr"].get<std::string>() +
        " port = " + configs["port"].get<std::string>();

        db_ = std::make_unique<pqxx::connection>(connectStr);
    } catch (const std::exception& e) {
        spdlog::error("Can't connect to DB: {}", e.what());
    }
}

std::vector<admin::Admin> DBImpl::getAdmins() const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::transaction tx{*db_};

        std::vector<admin::Admin> admins;
        admins.reserve(5);
        spdlog::debug("Get admins query: {}", GET_ADMIN_QUERY);
        for (auto const &[id, name, photo] : tx.stream<long, std::string, std::string>(GET_ADMIN_QUERY)) {
            admins.emplace_back(id, name, photo);
        }
        spdlog::debug("Admins collected successfully");
        return admins;
    } catch(const std::exception& e) {
        spdlog::error("Can't collect admins: {}", e.what());
    }
}

std::vector<std::string> DBImpl::getSlots(const std::string& date, int barberId) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn{*db_};

        std::string query = "select slots from slots where barber_id = "+std::to_string(barberId)+" and slot_date = '"+date+"'";
        spdlog::debug("get slots: {}", query);

        std::string slots = txn.query_value<std::string>(query);
        spdlog::debug("slots collected successfully");
        return freeTime(slots);
    } catch(const std::exception& e) {
        spdlog::error("Can't collect slots: {}", e.what());
    }
}

std::vector<user::User> DBImpl::getUsers() const {
    try{
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::transaction tx{*db_};

        std::vector<user::User> users;
        users.reserve(5);
        spdlog::debug("Get users query: {}", GET_USER_QUERY);
        for (auto const &[id, name, telegram_id] : tx.stream<long, std::string, int64_t>(GET_USER_QUERY)) {
            users.emplace_back(id, name, telegram_id);
        }
        spdlog::debug("Users collected successfully");
        return users;
    } catch(const std::exception& e) {
        spdlog::error("Can't collect users: {}", e.what());
    }
 }

int DBImpl::getUserIdByTelegramId(int64_t telegramId) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn{*db_};
        std::string q = "SELECT id FROM users WHERE telegram_id="+std::to_string(telegramId)+";";
        spdlog::debug("get user by telegram id: {}", q);
        return txn.query_value<int>(q);
    } catch(const std::exception& e) {
        spdlog::error("Can't get user by telegram id: {}", e.what());
    }
}

void DBImpl::createUser(const std::string& name, int64_t telegramId) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work tx{*db_};
        std::string q = "INSERT INTO users (name, telegram_id) VALUES ('" + name + "','"+std::to_string(telegramId)+"');";
        spdlog::debug("Create user: {}", q);
        tx.exec0(q);
        tx.commit();
        spdlog::debug("User created");
    } catch(const std::exception& e) {
        spdlog::error("Can't create user: {}", e.what());
    }
}

void DBImpl::createRecord(int barberId, int userId, const std::string& recordsDate, int timeNum) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work tx{*db_};
        std::string query = "INSERT INTO records (barber_id, user_id, records_date, time_num) VALUES ('"+std::to_string(barberId)+"','"+std::to_string(userId)+"','"+recordsDate+"','"+std::to_string(timeNum)+"');";
        spdlog::debug("Create record: {}", query);
        tx.exec0(query);
        tx.commit();
        spdlog::debug("Record created");
    } catch(const std::exception& e) {
        spdlog::error("Can't create record: {}", e.what());
    }
}

void DBImpl::updateSlots(int barberId, int timeNum, const std::string& date) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work txn{*db_};
        std::string q = "select slots from slots where barber_id = "+std::to_string(barberId)+" and slot_date = '"+date+"'";
        spdlog::debug("get slots: {}", q);
        std::string slots = txn.query_value<std::string>(q);
        spdlog::debug("Got slots");
    
        slots[timeNum] = '2';
        if (timeNum < slots.size() - 1) {
            slots[timeNum + 1] = '2';
        }
    
        q = "update slots SET slots='"+slots+"' where barber_id="+std::to_string(barberId)+" and slot_date='"+date+"';";
        spdlog::debug("Update slots: {}", q);
        txn.exec0(q);
        txn.commit();
        spdlog::debug("Slots updated");
    } catch(const std::exception& e) {
        spdlog::error("Can't update slots: {}", e.what());
    }
}

void DBImpl::dropRecord(int userId) const {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        pqxx::work tx{*db_};
        std::string q = "delete from records where user_id="+std::to_string(userId)+";";
        spdlog::debug("Delete records: {}", q);
        tx.exec0(q);
        tx.commit();
        spdlog::debug("Delete records");
    } catch(const std::exception& e) {
        spdlog::error("Can't create user: {}", e.what());
    }
}

void DBImpl::createSlot(const std::vector<std::string>& nextDates, int adminId) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        for(const auto& nextDate : nextDates) {
            pqxx::work tx{*db_};
            std::string q = "insert into slots (slots,slot_date,barber_id) values ('"+shit::DEFAULT_SLOTS+"','"+nextDate+"',"+std::to_string(adminId)+");";
            spdlog::debug("Create new slots: {}", q);
            tx.exec0(q);
            tx.commit();
            spdlog::debug("Slot created");
        }
    } catch(const std::exception& e) {
        spdlog::error("Can't create slots: {}", e.what());
    }
}

}
