#pragma once

#include "DBService/DB.hpp"

namespace scheduler {

class Scheduler {
public:
    virtual std::vector<std::string> showFreeSlotDates(time_t todayTimestamp, int nextDaysNum, int adminId) = 0;
    virtual void createFutureWorkDaysFromTomorrow(int nextDaysNum, int adminId) = 0;
};

} // namespace scheduler
