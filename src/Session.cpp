#include "Session.hpp"
#include "Scheduler/SchedulerImpl.hpp"

#include "spdlog/spdlog.h"

namespace session {

Session::Session(int64_t telegramId,
                std::mutex& cacheMutex,
                TgBot::Bot& bot,
                std::shared_ptr<scheduler::Scheduler> scheduler,
                std::shared_ptr<db::DB> db,
                std::vector<admin::Admin>& admins,
                std::unordered_map<std::string, size_t>& adminMap
                ) : telegramId_(telegramId),  cacheMutex_(cacheMutex), bot_(bot),
                admins_(admins), adminMap_(adminMap), scheduler_(std::move(scheduler)), db_(std::move(db)) {}

void Session::continueSession(TgBot::Message::Ptr message, const Json& configs) {
    switch (stage_)
    {
    case 0:
        zeroStage(std::move(message), configs);
        break;
    case 1:
        firstStage(std::move(message), configs);
        break;
    case 2:
        secondStage(std::move(message), configs);
        break;
    case 3:
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

void Session::zeroStage(TgBot::Message::Ptr message, const Json& configs) {
    try {
        spdlog::info("{}: {} stage", message->from->username, stage_);
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if (admins_.empty()) {
            spdlog::warn("{}: {} stage, admins cache was empty", message->from->username, stage_);
            shit::updateAdminsCache(db_, admins_, adminMap_);
        }
        bot_.getApi().sendMessage(message->chat->id, configs["barberList"].get<std::string>(), false, 0, shit::createAdminsKeyboard(admins_), "Markdown");
        spdlog::info("{}: {} stage, sent baberlist", message->from->username, stage_);
        for(const auto& admin : admins_) {
            spdlog::debug("{}: {} stage, barber", admin.name_, stage_);
        }
        ++stage_;
    } catch(const std::exception& e) {
        spdlog::error("{} stage: {}", e.what(), stage_);
        stage_ = DEFAULT_STAGE;
    }
}


void Session::firstStage(TgBot::Message::Ptr message, const Json& configs) {
    try {
        if (message->text.empty()) {
            spdlog::error("{}: {} stage empty text", message->from->username, stage_);
            stage_ = DEFAULT_STAGE;
            return;
        }

        spdlog::info("{}: {} stage", message->from->username, stage_);
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto findIt = adminMap_.find(message->text);
        if (findIt == adminMap_.end()) {
            shit::updateAdminsCache(db_, admins_, adminMap_);
            spdlog::info("{}: {} stage, update admins cache", message->from->username, stage_);
        }

        int ind = findIt->second;
        if (admins_.size() <= ind) {
            spdlog::error("{}: Admins size greater than found index", message->from->username);
            return;
        }
        spdlog::info("{}: {} stage, check", message->from->username, stage_);
        choosenAdmin_ = admins_[ind];
        spdlog::info("{}: {} stage, choosen admin - {}", message->from->username, stage_, admins_[ind].name_);

        if (!scheduler_) {
            throw std::runtime_error("scheduler_ is not initialized");
        }
        std::vector<std::string> dates = scheduler_->showFreeSlotDates(message->date, configs["nextWorkDays"].get<int>(), choosenAdmin_.id_);
        bot_.getApi().sendMessage(message->chat->id, configs["freeSlots"].get<std::string>(), false, 0, shit::createSlotsKeyboard(dates), "Markdown");
        spdlog::info("{}: {} stage, sent free slots", message->from->username, stage_);

        ++stage_;
    } catch(const std::exception& e) {
        spdlog::error("{} stage: {}", e.what(), stage_);
        stage_ = DEFAULT_STAGE;
    }
}

void Session::secondStage(TgBot::Message::Ptr message, const Json& configs) {
    try {
        if (message->text.empty()) {
            spdlog::error("{}: {} stage empty text", message->from->username, stage_);
            stage_ = DEFAULT_STAGE;
            return;
        }

        spdlog::info("{}: {} stage", message->from->username, stage_);
        std::lock_guard<std::mutex> lock(cacheMutex_);
        currentDate_ = message->text;

        auto times = db_->getSlots(currentDate_, choosenAdmin_.id_);
        bot_.getApi().sendMessage(message->chat->id, configs["freeSlots"].get<std::string>(), false, 0, shit::createSlotsKeyboard(times), "Markdown");
        spdlog::info("{}: {} stage, sent free slots", message->from->username, stage_);

        ++stage_;
    } catch(const std::exception& e) {
        spdlog::error("{} stage: {}", e.what(), stage_);
        stage_ = DEFAULT_STAGE;
    }
}

void Session::thirdStage(TgBot::Message::Ptr message, const Json& configs) {
    try {
        if (message->text.empty()) {
            spdlog::error("{}: {} stage empty text", message->from->username, stage_);
            stage_ = DEFAULT_STAGE;
            return;
        }
        spdlog::info("{}: {} stage", message->from->username, stage_);
        int userId = db_->getUserIdByTelegramId(message->from->id);
        int slotNum = shit::slotTimeToInt(message->text);

        db_->updateSlots(choosenAdmin_.id_, slotNum, currentDate_);
        spdlog::info("{}: {} stage, updated slots", message->from->username, stage_);

        db_->createRecord(choosenAdmin_.id_, userId, currentDate_, slotNum);
        spdlog::info("{}: {} stage, record created", message->from->username, stage_);
        bot_.getApi().sendMessage(message->chat->id, configs["endPhrase"].get<std::string>(), false, 0, shit::removeKeyBoard(), "Markdown");
        spdlog::info("{}: {} stage, sent end phrase", message->from->username, stage_);

        stage_ = DEFAULT_STAGE;
    } catch(const std::exception& e) {
        spdlog::error("{} stage: {}", e.what(), stage_);
        stage_ = DEFAULT_STAGE;
    }
}

void Session::back() {
    stage_ = stage_ > DEFAULT_STAGE ? stage_ - 1 : DEFAULT_STAGE;
}

} // namespace session
