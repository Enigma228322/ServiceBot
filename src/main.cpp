#include <stdio.h>
#include <iostream>
#include <chrono>

#include "utils/shit.hpp"
#include "DBService/DBImpl.hpp"
#include "Scheduler/SchedulerImpl.hpp"
#include "Session.hpp"

#include "spdlog/spdlog.h"

using namespace TgBot;
using Json = nlohmann::json;

namespace {

void handleAnyMessage(const TgBot::Message::Ptr& message,
                std::mutex& cacheMutex,
                TgBot::Bot& bot,
                std::shared_ptr<scheduler::Scheduler>& scheduler,
                std::shared_ptr<db::DB>& db,
                const Json& configs,
                std::vector<admin::Admin>& admins,
                std::unordered_map<std::string, size_t>& adminMap,
                std::unordered_map<int64_t, std::unique_ptr<session::Session>>& sessions) {
    auto sessionIt = sessions.find(message->chat->id);
    if (sessionIt == sessions.end()) {
        spdlog::warn("{}: Session isn't exist, create at 'Any message' event", message->from->username);
        sessions[message->chat->id] = std::make_unique<session::Session>(message->chat->id, cacheMutex, bot, scheduler, db, admins, adminMap);
    }
    sessions[message->chat->id]->continueSession(std::move(message), configs);
}

} // namespace

int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("Initialization started");
    Json configs = shit::readConfig();
    std::string token = shit::getToken(configs);

    spdlog::debug("Configs parsed");
    std::shared_ptr<db::DB> db = std::make_shared<db::DBImpl>(configs);
    spdlog::debug("DB connected");
    std::mutex cacheMutex_;

    std::shared_ptr<scheduler::Scheduler> scheduler;
    bool scheduleCreated = false;

    std::vector<user::User> users_;
    std::vector<admin::Admin> admins_;
    std::unordered_map<std::string, size_t> adminMap_;
    std::unordered_map<int64_t, std::unique_ptr<session::Session>> sessions_;

    TgBot::Bot bot(token);
    bot.getEvents().onCommand("start", [&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs, &scheduler, &scheduleCreated](TgBot::Message::Ptr message) {
        spdlog::info("{}: start", message->from->username);
        bot.getApi().sendMessage(message->chat->id, configs["hello"].get<std::string>(), false, 0, shit::createStartKeyboard(configs), "Markdown");

        std::lock_guard<std::mutex> lock(cacheMutex_);
        shit::updateAdminsCache(db, admins_, adminMap_);
        shit::createWorkDaysInDB(scheduleCreated, scheduler, db, message, admins_, configs);
        shit::updateUsersCache(db, users_);
        spdlog::debug("{}: Cache updated", message->from->username);

        shit::createUser(users_, message, db);
    });

    bot.getEvents().onCallbackQuery([&bot, &sessions_, &configs](CallbackQuery::Ptr query) {
        if (StringTools::startsWith(query->data, configs["back"]["command"].get<std::string>())) {
        }
    });

    bot.getEvents().onCallbackQuery([&bot,  &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs, &scheduler](CallbackQuery::Ptr query) {
        if (StringTools::startsWith(query->data, configs["newRecord"]["command"].get<std::string>())) {
            spdlog::info("{}: pressed new_record", query->message->from->username);
            handleAnyMessage(query->message, cacheMutex_, bot, scheduler, db, configs, admins_, adminMap_, sessions_);
        }
    });

    bot.getEvents().onAnyMessage([&bot, &db, &admins_, &adminMap_, &users_, &cacheMutex_, &sessions_, &configs, &scheduler](TgBot::Message::Ptr message) {
        if (StringTools::startsWith(message->text, "/start")
        || StringTools::startsWith(message->text, configs["back"]["command"].get<std::string>())
        || StringTools::startsWith(message->text, configs["newRecord"]["command"].get<std::string>())) {
            return;
        }
        spdlog::info("{}: pressed new_record", message->from->username);
        handleAnyMessage(message, cacheMutex_, bot, scheduler, db, configs, admins_, adminMap_, sessions_);
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