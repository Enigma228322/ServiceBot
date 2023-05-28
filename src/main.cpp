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
    bot.getEvents().onCommand("start", [&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!", false, 0, shit::createStartKeyboard(configs), "Markdown");

        std::lock_guard<std::mutex> lock(cacheMutex_);
        shit::updateAdminsCache(db, admins_, adminMap_);
        shit::updateUsersCache(db, users_);
        auto it = std::find_if(users_.begin(), users_.end(), [mid = message->chat->id](const user::User& user){
            return user.telegram_id_ == mid;
        });
        if (it == users_.end()) {
            user::User user(shit::SERIAL_ID, message->chat->firstName+' '+message->chat->lastName, message->chat->id);
            db->createUser(user.name_, user.telegram_id_);
            users_.push_back(std::move(user));
        }
    });

    bot.getEvents().onCallbackQuery([&bot, &sessions_, &configs](CallbackQuery::Ptr query) {
        if (StringTools::startsWith(query->data, "back")) {
            int64_t tgId = query->message->chat->id;
            auto sessionIt = sessions_.find(tgId);
            if (sessionIt != sessions_.end()) {
                sessions_[tgId]->back();
                sessions_[tgId]->continueSession(std::move(query->message));
            } else {
                bot.getApi().sendMessage(query->message->chat->id, "Начальная точка", false, 0, shit::createStartKeyboard(configs), "Markdown");
            }
        }
    });

    bot.getEvents().onCallbackQuery([&bot,  &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs](CallbackQuery::Ptr query) {
        if (StringTools::startsWith(query->data, "new_record")) {
            int64_t tgId = query->message->chat->id;
            auto sessionIt = sessions_.find(tgId);
            if (sessionIt == sessions_.end()) {
                sessions_[tgId] = std::make_unique<session::Session>(query->message->chat->id, cacheMutex_, bot, db, admins_, adminMap_);
            }
            sessions_[tgId]->continueSession(std::move(query->message));
        }
    });

    bot.getEvents().onAnyMessage([&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_](TgBot::Message::Ptr message) {
        if (StringTools::startsWith(message->text, "/start") || StringTools::startsWith(message->text, "back") || StringTools::startsWith(message->text, "new_record")) {
            return;
        }
        auto sessionIt = sessions_.find(message->chat->id);
        if (sessionIt == sessions_.end()) {
            sessions_[message->chat->id] = std::make_unique<session::Session>(message->chat->id, cacheMutex_, bot, db, admins_, adminMap_);
        }
        sessions_[message->chat->id]->continueSession(std::move(message));
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