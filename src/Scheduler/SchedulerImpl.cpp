#include "Scheduler/SchedulerImpl.hpp"
#include "utils/shit.hpp"

#include "spdlog/spdlog.h"

using namespace scheduler;

namespace {

constexpr time_t DAY_SECONDS = 86400;

} // namespace

SchedulerImpl::SchedulerImpl(std::shared_ptr<db::DB> db, time_t todayTimestamp) : db_(std::move(db)), todayTimestamp_(todayTimestamp) {}

std::vector<std::string> SchedulerImpl::showFreeSlotDates(time_t todayTimestamp, int nextDaysNum, int adminId) {
    std::vector<std::string> dates;
    dates.reserve(nextDaysNum + 1);
    for(int i = 0; i <= nextDaysNum; ++i) {
        std::string date = shit::timestampToDate(todayTimestamp_ + (i + 1) * DAY_SECONDS);
        if (!db_->getSlots(date, adminId).empty()) {
            dates.push_back(date);
        }
    }
    return dates;
}

void SchedulerImpl::createFutureWorkDaysFromTomorrow(int nextDaysNum, int adminId) {
    std::vector<std::string> nextDates(nextDaysNum);
    for(int i = 0; i < nextDaysNum; ++i) {
        nextDates[i] = shit::timestampToDate(todayTimestamp_ + (i + 1) * DAY_SECONDS);
        spdlog::debug("date {}: {}", i, nextDates[i]);
    }
    db_->createSlot(nextDates, adminId);
}