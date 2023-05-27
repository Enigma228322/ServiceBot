#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <chrono>

#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <utils/base64.hpp>

#include "Admin.hpp"
#include "User.hpp"
#include "DBService/DB.hpp"
#include "DBService/DBImpl.hpp"

using namespace TgBot;
using Json = nlohmann::json;

namespace {
constexpr const char* const CONFIG_FILENAME = "configs.json";
constexpr int SLOTS_KEYBOARD_ROW_SIZE = 5;
constexpr int SERIAL_ID = 0;

Json readConfig() {
    std::ifstream in(CONFIG_FILENAME);
    if (!in.is_open()) {
        throw std::runtime_error("Cant open configs file");
    }
    return Json::parse(in);
}

std::string getToken(const Json& configs) {
    std::string encodedToken = configs["token"];
    std::vector<BYTE> tokenTemp = base64_decode(encodedToken);
    return std::string(tokenTemp.begin(), tokenTemp.end());
}

ReplyKeyboardMarkup::Ptr createAdminsKeyboard(const std::vector<admin::Admin>& admins) {
    ReplyKeyboardMarkup::Ptr keyboard = std::make_shared<ReplyKeyboardMarkup>();
    std::vector<KeyboardButton::Ptr> row;
    for(const auto& admin : admins) {
        KeyboardButton::Ptr button = std::make_shared<KeyboardButton>();
        button->text = admin.name_;
        row.push_back(std::move(button));
    }
    keyboard->oneTimeKeyboard = true;
    keyboard->keyboard.push_back(row);
    return keyboard;
}

ReplyKeyboardMarkup::Ptr createSlotsKeyboard(const std::vector<std::string>& slots) {
    ReplyKeyboardMarkup::Ptr keyboard = std::make_shared<ReplyKeyboardMarkup>();
    std::vector<KeyboardButton::Ptr> row;
    for(size_t i = 0; i < slots.size(); ++i) {
        if (i % SLOTS_KEYBOARD_ROW_SIZE == 0 && !row.empty()) {
            keyboard->keyboard.push_back(row);
            row.clear();
            row.reserve(SLOTS_KEYBOARD_ROW_SIZE);
        }
         KeyboardButton::Ptr button = std::make_shared<KeyboardButton>();
        button->text = slots[i];
        row.push_back(std::move(button));
    }
    keyboard->oneTimeKeyboard = true;
    keyboard->keyboard.push_back(row);
    return keyboard;
}

void updateAdminsCache(std::unique_ptr<db::DB>& db, std::vector<admin::Admin>& admins,
std::unordered_map<std::string, size_t>& adminMap) {
    admins = db->getAdmins();
    adminMap.reserve(admins.size());
    for(size_t i = 0; i < admins.size(); ++i) {
        adminMap[admins[i].name_] = i;
    }
}

void updateUsersCache(std::unique_ptr<db::DB>& db, std::vector<user::User>& users) {
    users = db->getUsers();
}

int slotTimeToInt(const std::string& slot) {
    if (slot.size() == 7) {
        return 2*(((slot[0]-48) * 10) + (slot[1]-48)) + (slot[5] == '3' ? 1 : 0);
    } else {
        return 2*(slot[0]-48) + (slot[4] == '3' ? 1 : 0);
    }
}

std::string timestampToDate(time_t unix_timestamp)
{
    char time_buf[80];
    struct tm ts;
    ts = *localtime(&unix_timestamp);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d", &ts);

    return time_buf;
}

} // namespace

int main() {

    Json configs = readConfig();
    std::string token = getToken(configs);

    std::unique_ptr<db::DB> db = std::make_unique<db::DBImpl>(configs);

    std::vector<user::User> users;
    std::vector<admin::Admin> admins;
    std::unordered_map<std::string, size_t> adminMap;

    /*
        0 - список барберов
        1 - доступное время
        2 - бронирование
    */
    int stage = 0;
    int barberId = 0;

    TgBot::Bot bot(token);
    bot.getEvents().onCommand("start", [&bot, &db, &admins, &adminMap, &users](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
        updateAdminsCache(db, admins, adminMap);
        updateUsersCache(db, users);
        auto it = std::find_if(users.begin(), users.end(), [mid = message->from->id](const user::User& user){
            return user.telegram_id_ == mid;
        });
        if (it == users.end()) {
            user::User user(SERIAL_ID, message->from->firstName+' '+message->from->lastName, message->from->id);
            db->createUser(user.name_, user.telegram_id_);
            users.push_back(std::move(user));
        }
    });

    bot.getEvents().onAnyMessage([&bot, &db, &admins, &adminMap, &stage, &barberId, &users](TgBot::Message::Ptr message) {
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }

        if (stage == 0) {
            if (admins.empty()) {
                updateAdminsCache(db, admins, adminMap);
            }
            std::cout << "stage 0 - 1\n";
            bot.getApi().sendMessage(message->chat->id, "Список барберов", false, 0, createAdminsKeyboard(admins), "Markdown");
            std::cout << "stage 0 - 2\n";
            ++stage;
        } else if (stage == 1) {
            if (message->text.empty()) {
                stage = 0;
                return;
            }
            auto findIt = adminMap.find(message->text);
            if (findIt == adminMap.end()) {
                updateAdminsCache(db, admins, adminMap);
            } else {
                int ind = findIt->second;
                if (admins.size() <= ind) {
                    throw std::runtime_error("Admins size greater that found index");
                }
                const admin::Admin& admin = admins[ind];
                barberId = admin.id_;
                std::string currentDate = timestampToDate(message->date);
                auto times = db->getSlots(currentDate, 2);
                std::cout << "stage 1 - 1\n";
                bot.getApi().sendMessage(message->chat->id, "Доступные слоты", false, 0, createSlotsKeyboard(times), "Markdown");
                std::cout << "stage 1 - 2\n";
                ++stage;
                return;
            }
        } else if (stage == 2) {
            if (message->text.empty()) {
                stage = 0;
                return;
            }
            int userId = db->getUserIdByTelegramId(message->from->id);
            std::string date = timestampToDate(message->date);
            int slotNum = slotTimeToInt(message->text);
            db->createRecord(barberId, userId, date, slotNum);
            std::cout << "stage 2 - 1\n";
            bot.getApi().sendMessage(message->chat->id, "Вы были успешно записаны");
            std::cout << "stage 2 - 2\n";

            stage = 0;
        } else {
            throw std::runtime_error("broken stage");
        }
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}