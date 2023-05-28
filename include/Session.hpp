#pragma once

#include "Admin.hpp"
#include "utils/shit.hpp"

#include <string>
#include <mutex>

namespace session {

class Session {
public:
    Session(int64_t telegramId,
            std::mutex& cacheMutex,
            TgBot::Bot& bot,
            std::shared_ptr<db::DB> db,
            std::vector<admin::Admin>& admins,
            std::unordered_map<std::string, size_t>& adminMap);

    void continueSession(TgBot::Message::Ptr message, const Json& configs);
    void back();
    void deleteSession();

private:
    void firstStage(TgBot::Message::Ptr message, const Json& configs);
    void secondStage(TgBot::Message::Ptr message, const Json& configs);
    void thirdStage(TgBot::Message::Ptr message, const Json& configs);

    int stage_{0};
    admin::Admin choosenAdmin_;
    int64_t telegramId_;

    std::shared_ptr<db::DB> db_;
    TgBot::Bot& bot_;
    std::mutex& cacheMutex_;
    std::vector<admin::Admin>& admins_;
    std::unordered_map<std::string, size_t>& adminMap_;
};

} // namespace session
