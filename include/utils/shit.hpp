#pragma once

#include <fstream>
#include <unordered_map>

#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <utils/base64.hpp>

#include "Admin.hpp"
#include "User.hpp"
#include "DBService/DB.hpp"
#include "Scheduler/SchedulerImpl.hpp"

using namespace TgBot;
using Json = nlohmann::json;

namespace shit {
constexpr const char* const CONFIG_FILENAME = "configs.json";
constexpr int SLOTS_KEYBOARD_ROW_SIZE = 5;
constexpr int SERIAL_ID = 0;
const std::string DEFAULT_SLOTS = "000000000000000000111111111111111111100000000000";

Json readConfig();

std::string getToken(const Json& configs);

InlineKeyboardMarkup::Ptr createStartKeyboard(Json& configs);

ReplyKeyboardMarkup::Ptr createAdminsKeyboard(const std::vector<admin::Admin>& admins);

ReplyKeyboardRemove::Ptr removeKeyBoard();

ReplyKeyboardMarkup::Ptr createSlotsKeyboard(const std::vector<std::string>& slots);

void updateAdminsCache(std::shared_ptr<db::DB> db, std::vector<admin::Admin>& admins,
std::unordered_map<std::string, size_t>& adminMap);

void updateUsersCache(std::shared_ptr<db::DB> db, std::vector<user::User>& users);

int slotTimeToInt(const std::string& slot);

std::string timestampToDate(time_t unix_timestamp);

void createWorkDaysInDB(bool scheduleCreated,
                        std::unique_ptr<scheduler::Scheduler>& scheduler,
                        std::shared_ptr<db::DB>& db,
                        const TgBot::Message::Ptr& message,
                        const std::vector<admin::Admin> admins,
                        const Json& configs);

void createUser(std::vector<user::User>& users,
                const TgBot::Message::Ptr& message,
                std::shared_ptr<db::DB>& db);

} // namespace shit