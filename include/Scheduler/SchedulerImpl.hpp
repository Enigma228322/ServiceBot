#pragma once

#include <memory>

#include "DBService/DB.hpp"
#include "Scheduler/Scheduler.hpp"

namespace scheduler {

class SchedulerImpl : public Scheduler {
public:
    SchedulerImpl(std::shared_ptr<db::DB> db, time_t todayTimestamp_);
    std::vector<std::string> showFreeSlotDates(time_t todayTimestamp, int nextDaysNum, int adminId) override;
    void createFutureWorkDaysFromTomorrow(int nextDaysNum, int adminId);

private:

    std::shared_ptr<db::DB> db_;
    // std::vector<DaySchedule> workDays_;
    time_t todayTimestamp_;
};

} // namespace scheduler
