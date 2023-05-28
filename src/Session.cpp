#include "Session.hpp"

namespace session {

Session::Session(int64_t telegramId,
                std::mutex& cacheMutex,
                TgBot::Bot& bot,
                std::shared_ptr<db::DB> db,
                std::vector<admin::Admin>& admins,
                std::unordered_map<std::string, size_t>& adminMap
                ) : telegramId_(telegramId),  cacheMutex_(cacheMutex), bot_(bot),
                admins_(admins), adminMap_(adminMap), db_(std::move(db)) {}

void Session::continueSession(TgBot::Message::Ptr message) {
    switch (stage_)
    {
    case 0:
        firstStage(std::move(message));
        break;
    case 1:
        secondStage(std::move(message));
        break;
    case 2:
        thirdStage(std::move(message));
        break;
    
    default:
        throw std::runtime_error("broken stage");
        break;
    }
}

void Session::deleteSession() {
    // delete session from db
}

void Session::firstStage(TgBot::Message::Ptr message) {
    {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        if (admins_.empty()) {
            shit::updateAdminsCache(db_, admins_, adminMap_);
        }
        bot_.getApi().sendMessage(message->chat->id, "Список барберов", false, 0, shit::createAdminsKeyboard(admins_), "Markdown");
    }
    ++stage_;
}

void Session::secondStage(TgBot::Message::Ptr message) {
    if (message->text.empty()) {
        stage_ = 0;
        return;
    }

    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto findIt = adminMap_.find(message->text);
    if (findIt == adminMap_.end()) {
        shit::updateAdminsCache(db_, admins_, adminMap_);
    } else {
        int ind = findIt->second;
        if (admins_.size() <= ind) {
            throw std::runtime_error("Admins size greater that found index");
        }
        choosenAdmin_ = admins_[ind];
        std::string currentDate = shit::timestampToDate(message->date);
        auto times = db_->getSlots(currentDate, choosenAdmin_.id_);
        bot_.getApi().sendMessage(message->chat->id, "Доступные слоты", false, 0, shit::createSlotsKeyboard(times), "Markdown");

        ++stage_;
        return;
    }
}

void Session::thirdStage(TgBot::Message::Ptr message) {
    if (message->text.empty()) {
        stage_ = 0;
        return;
    }
    int userId = db_->getUserIdByTelegramId(message->from->id);
    std::string date = shit::timestampToDate(message->date);
    int slotNum = shit::slotTimeToInt(message->text);

    db_->updateSlots(choosenAdmin_.id_, slotNum, date);

    db_->createRecord(choosenAdmin_.id_, userId, date, slotNum);
    bot_.getApi().sendMessage(message->chat->id, "Вы были успешно записаны", false, 0, shit::removeKeyBoard(), "Markdown");

    stage_ = 0;
}

} // namespace session
