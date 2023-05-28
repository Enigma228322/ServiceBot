#include "Session.hpp"

#include "spdlog/spdlog.h"

namespace session {

Session::Session(int64_t telegramId,
                std::mutex& cacheMutex,
                TgBot::Bot& bot,
                std::shared_ptr<db::DB> db,
                std::vector<admin::Admin>& admins,
                std::unordered_map<std::string, size_t>& adminMap
                ) : telegramId_(telegramId),  cacheMutex_(cacheMutex), bot_(bot),
                admins_(admins), adminMap_(adminMap), db_(std::move(db)) {}

void Session::continueSession(TgBot::Message::Ptr message, const Json& configs) {
    switch (stage_)
    {
    case 0:
        firstStage(std::move(message), configs);
        break;
    case 1:
        secondStage(std::move(message), configs);
        break;
    case 2:
        thirdStage(std::move(message), configs);
        break;
    
    default:
        throw std::runtime_error("broken stage");
        break;
    }
}

void Session::deleteSession() {
    // delete session from db
}

void Session::firstStage(TgBot::Message::Ptr message, const Json& configs) {
    {
        spdlog::info("{}: 1st stage", message->from->username);
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if (admins_.empty()) {
            spdlog::warn("{}: 1st stage, admins cache was empty", message->from->username);
            shit::updateAdminsCache(db_, admins_, adminMap_);
        }
        bot_.getApi().sendMessage(message->chat->id, configs["barberList"].get<std::string>(), false, 0, shit::createAdminsKeyboard(admins_), "Markdown");
        spdlog::info("{}: sent baberlist", message->from->username);
        for(const auto& admin : admins_) {
            spdlog::debug("{}: barber", admin.name_);
        }
    }
    ++stage_;
}

void Session::secondStage(TgBot::Message::Ptr message, const Json& configs) {
    if (message->text.empty()) {
        spdlog::error("{}: 2d stage empty text", message->from->username);
        stage_ = 0;
        return;
    }

    spdlog::info("{}: 2d stage", message->from->username);
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto findIt = adminMap_.find(message->text);
    if (findIt == adminMap_.end()) {
        shit::updateAdminsCache(db_, admins_, adminMap_);
        spdlog::info("{}: 2d stage, update admins cache", message->from->username);
    } else {
        int ind = findIt->second;
        if (admins_.size() <= ind) {
            spdlog::error("{}: Admins size greater than found index", message->from->username);
            return;
        }
        choosenAdmin_ = admins_[ind];
        std::string currentDate = shit::timestampToDate(message->date);
        auto times = db_->getSlots(currentDate, choosenAdmin_.id_);
        bot_.getApi().sendMessage(message->chat->id, configs["freeSlots"].get<std::string>(), false, 0, shit::createSlotsKeyboard(times), "Markdown");
        spdlog::info("{}: 2d stage, sent free slots", message->from->username);

        ++stage_;
        return;
    }
}

void Session::thirdStage(TgBot::Message::Ptr message, const Json& configs) {
    if (message->text.empty()) {
        spdlog::error("{}: 3d stage empty text", message->from->username);
        stage_ = 0;
        return;
    }
    spdlog::info("{}: 3d stage", message->from->username);
    int userId = db_->getUserIdByTelegramId(message->from->id);
    std::string date = shit::timestampToDate(message->date);
    int slotNum = shit::slotTimeToInt(message->text);

    db_->updateSlots(choosenAdmin_.id_, slotNum, date);
    spdlog::info("{}: 3d stage, updated slots", message->from->username);

    db_->createRecord(choosenAdmin_.id_, userId, date, slotNum);
    spdlog::info("{}: 3d stage, record created", message->from->username);
    bot_.getApi().sendMessage(message->chat->id, configs["endPhrase"].get<std::string>(), false, 0, shit::removeKeyBoard(), "Markdown");
    spdlog::info("{}: 3d stage, sent end phrase", message->from->username);

    stage_ = 0;
}

void Session::back() {
    stage_ = stage_ > 0 ? stage_ - 1 : 0;
}

} // namespace session
