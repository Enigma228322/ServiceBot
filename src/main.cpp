#include <stdio.h>
#include <iostream>
#include <chrono>

#include "utils/shit.hpp"
#include "DBService/DBImpl.hpp"
#include "Session.hpp"
#include "spdlog/spdlog.h"

using namespace TgBot;
using Json = nlohmann::json;

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Initialization started");
    Json configs = shit::readConfig();
    std::string token = shit::getToken(configs);

    spdlog::debug("Configs parsed");
    std::shared_ptr<db::DB> db = std::make_shared<db::DBImpl>(configs);
    spdlog::debug("DB connected");
    std::mutex cacheMutex_;

    std::vector<user::User> users_;
    std::vector<admin::Admin> admins_;
    std::unordered_map<std::string, size_t> adminMap_;
    std::unordered_map<int64_t, std::unique_ptr<session::Session>> sessions_;

    TgBot::Bot bot(token);
    bot.getEvents().onCommand("start", [&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs](TgBot::Message::Ptr message) {
        spdlog::info("{}: start", message->from->username);
        bot.getApi().sendMessage(message->chat->id, configs["hello"].get<std::string>(), false, 0, shit::createStartKeyboard(configs), "Markdown");

        std::lock_guard<std::mutex> lock(cacheMutex_);
        shit::updateAdminsCache(db, admins_, adminMap_);
        shit::updateUsersCache(db, users_);
        spdlog::debug("{}: Cache updated", message->from->username);

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
        if (StringTools::startsWith(query->data, configs["back"]["command"].get<std::string>())) {
            spdlog::info("{}: pressed back", query->message->from->username);
            int64_t tgId = query->message->chat->id;
            auto sessionIt = sessions_.find(tgId);
            if (sessionIt != sessions_.end()) {
                sessions_[tgId]->back();
                sessions_[tgId]->continueSession(std::move(query->message), configs);
            } else {
                bot.getApi().sendMessage(query->message->chat->id, "Начальная точка", false, 0, shit::createStartKeyboard(configs), "Markdown");
            }
        }
    });

    bot.getEvents().onCallbackQuery([&bot,  &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs](CallbackQuery::Ptr query) {
        if (StringTools::startsWith(query->data, configs["newRecord"]["command"].get<std::string>())) {
            spdlog::info("{}: pressed new_record", query->message->from->username);
            int64_t tgId = query->message->chat->id;
            auto sessionIt = sessions_.find(tgId);
            if (sessionIt == sessions_.end()) {
                sessions_[tgId] = std::make_unique<session::Session>(query->message->chat->id, cacheMutex_, bot, db, admins_, adminMap_);
            }
            sessions_[tgId]->continueSession(std::move(query->message), configs);
        }
    });

    bot.getEvents().onAnyMessage([&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs](TgBot::Message::Ptr message) {
        if (StringTools::startsWith(message->text, "/start")
        || StringTools::startsWith(message->text, configs["back"]["command"].get<std::string>())
        || StringTools::startsWith(message->text, configs["newRecord"]["command"].get<std::string>())) {
            return;
        }
        spdlog::info("{}: pressed new_record", message->from->username);
        auto sessionIt = sessions_.find(message->chat->id);
        if (sessionIt == sessions_.end()) {
            spdlog::warn("{}: Session isn't exist, create at 'Any message' event", message->from->username);
            sessions_[message->chat->id] = std::make_unique<session::Session>(message->chat->id, cacheMutex_, bot, db, admins_, adminMap_);
        }
        sessions_[message->chat->id]->continueSession(std::move(message), configs);
    });

    try {
        spdlog::info("Bot username: {}", bot.getApi().getMe()->username);
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            spdlog::debug("Long poll started");
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }

    return 0;
}