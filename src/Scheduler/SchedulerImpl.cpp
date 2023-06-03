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
    spdlog::debug("showFreeSlotDates({},{},{})", todayTimestamp, nextDaysNum, adminId);
    for(int i = 0; i <= nextDaysNum; ++i) {
        std::string date = shit::timestampToDate(todayTimestamp_ + (i + 1) * DAY_SECONDS);
        std::vector<std::string> slots = db_->getSlots(date, adminId);
        if (!slots.empty()) {
            dates.push_back(date);
        }
    }
    spdlog::debug("showFreeSlotDates({},{},{})", todayTimestamp, nextDaysNum, adminId);
    return dates;
}

void SchedulerImpl::createFutureWorkDaysFromTomorrow(int nextDaysNum, int adminId) {
    for(int i = 0; i < nextDaysNum; ++i) {
        const std::string& nextDate = shit::timestampToDate(todayTimestamp_ + (i + 1) * DAY_SECONDS);
        db_->createSlot(nextDate, adminId);
    }
}