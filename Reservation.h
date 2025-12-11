#pragma once
// 预约结构体定义：描述某用户在某设备上的时间占用与借用状态

#include <ctime>

struct Reservation {
    // 预约开始与结束时间（Unix 时间戳，单位秒）
    std::time_t startTime{0};
    std::time_t endTime{0};
    // 发起预约的用户 ID
    int userId{0};
    // 是否已借出（借用中）
    bool borrowed{false};
    // 实际开始使用时间（在借出时记录，用于计算使用时长）
    std::time_t actualStartTime{0};
};

