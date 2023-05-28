#include <stdio.h>
#include <iostream>
#include <chrono>

#include "utils/shit.hpp"
#include "DBService/DBImpl.hpp"
#include "Session.hpp"

using namespace TgBot;
using Json = nlohmann::json;

int main() {

    Json configs = shit::readConfig();
    std::string token = shit::getToken(configs);

    std::shared_ptr<db::DB> db = std::make_shared<db::DBImpl>(configs);
    std::mutex cacheMutex_;

    std::vector<user::User> users_;
    std::vector<admin::Admin> admins_;
    std::unordered_map<std::string, size_t> adminMap_;
    std::unordered_map<int64_t, std::unique_ptr<session::Session>> sessions_;

    TgBot::Bot bot(token);
    bot.getEvents().onCommand("start", [&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");

        std::lock_guard<std::mutex> lock(cacheMutex_);
        shit::updateAdminsCache(db, admins_, adminMap_);
        shit::updateUsersCache(db, users_);
        auto it = std::find_if(users_.begin(), users_.end(), [mid = message->from->id](const user::User& user){
            return user.telegram_id_ == mid;
        });
        if (it == users_.end()) {
            user::User user(shit::SERIAL_ID, message->from->firstName+' '+message->from->lastName, message->from->id);
            db->createUser(user.name_, user.telegram_id_);
            users_.push_back(std::move(user));
        }

        auto sessionIt = sessions_.find(message->from->id);
        if (sessionIt == sessions_.end()) {
            sessions_[message->from->id] = std::make_unique<session::Session>(message->from->id, cacheMutex_, bot, db, admins_, adminMap_);
        }
    });

    bot.getEvents().onAnyMessage([&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_](TgBot::Message::Ptr message) {
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        auto sessionIt = sessions_.find(message->from->id);
        if (sessionIt == sessions_.end()) {
            sessions_[message->from->id] = std::make_unique<session::Session>(message->from->id, cacheMutex_, bot, db, admins_, adminMap_);
        }
        sessions_[message->from->id]->continueSession(std::move(message));
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