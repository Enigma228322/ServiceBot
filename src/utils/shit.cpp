#include "utils/shit.hpp"

#include "spdlog/spdlog.h"

namespace shit {

Json readConfig() {
    std::ifstream in(CONFIG_FILENAME);
    if (!in.is_open()) {
        throw std::runtime_error("Cant open configs file");
    }
    return Json::parse(in);
}

std::string getToken(const Json& configs) {
    std::vector<BYTE> tokenTemp = base64_decode(configs["token"].get<std::string>());
    return std::string(tokenTemp.begin(), tokenTemp.end());
}

InlineKeyboardMarkup::Ptr createStartKeyboard(Json& configs) {
    InlineKeyboardMarkup::Ptr keyboard = std::make_shared<InlineKeyboardMarkup>();
    std::vector<std::vector<std::string>> startButtons = {
        {configs["newRecord"]["text"].get<std::string>(),
        configs["newRecord"]["command"].get<std::string>()}
    };
    for(std::vector<std::string>& startButton : startButtons) {
        InlineKeyboardButton::Ptr button = std::make_shared<InlineKeyboardButton>();
        button->text = std::move(startButton[0]);
        button->callbackData = std::move(startButton[1]);
        keyboard->inlineKeyboard.push_back({button});
    }
    return keyboard;
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

ReplyKeyboardRemove::Ptr removeKeyBoard() {
    ReplyKeyboardRemove::Ptr remove = std::make_shared<ReplyKeyboardRemove>();
    return remove;
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

void updateAdminsCache(std::shared_ptr<db::DB> db, std::vector<admin::Admin>& admins,
std::unordered_map<std::string, size_t>& adminMap) {
    admins = db->getAdmins();
    adminMap.reserve(admins.size());
    for(size_t i = 0; i < admins.size(); ++i) {
        adminMap[admins[i].name_] = i;
    }
}

void updateUsersCache(std::shared_ptr<db::DB> db, std::vector<user::User>& users) {
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

void createWorkDaysInDB(bool scheduleCreated,
                        std::shared_ptr<scheduler::Scheduler>& scheduler,
                        std::shared_ptr<db::DB>& db,
                        const TgBot::Message::Ptr& message,
                        const std::vector<admin::Admin> admins,
                        const Json& configs) {
    if (scheduleCreated) {
        return;
    }
    scheduler = std::make_shared<scheduler::SchedulerImpl>(db, message->date);
    spdlog::info("Initialize scheduler");
    for(const auto& admin : admins) {
        scheduler->createFutureWorkDaysFromTomorrow(configs["createNextWorkDaysNumber"].get<int>(), admin.id_);
    }
    scheduleCreated = true;
}

void createUser(std::vector<user::User>& users,
                const TgBot::Message::Ptr& message,
                std::shared_ptr<db::DB>& db) {
    auto it = std::find_if(users.begin(), users.end(), [mid = message->chat->id](const user::User& user){
        return user.telegram_id_ == mid;
    });
    if (it == users.end()) {
        user::User user(shit::SERIAL_ID, message->chat->firstName+' '+message->chat->lastName, message->chat->id);
        db->createUser(user.name_, user.telegram_id_);
        users.push_back(std::move(user));
    }
}

} // namespace shit