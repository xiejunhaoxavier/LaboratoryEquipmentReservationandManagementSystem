#pragma once
// 系统通用类型枚举定义

// 用户类型枚举
enum class UserType {
    Student,
    Teacher,
    Admin
};

// 设备类型枚举
enum class DeviceType {
    Consumable,  // 耗材型设备（如 3D 打印机）
    Precision,   // 精密型设备（如显微镜）
    Power        // 动力型设备（如离心机）
};

// 设备动态状态枚举（动态计算，不持久化）
enum class DeviceStatus {
    IDLE,       // 当前不在任何预约窗口
    RESERVED,   // 当前处于预约窗口但尚未借出
    IN_USE,     // 当前处于预约窗口且已借出
    BROKEN      // 健康度为 0 或以下
};

