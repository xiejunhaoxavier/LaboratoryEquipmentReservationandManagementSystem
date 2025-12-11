#pragma once
// 设备类层次结构定义，提供多态接口与通用属性；不同设备类型通过覆写行为体现差异

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <ctime>

#include "Types.h"
#include "Reservation.h"

class Device {
public:
    // 虚析构函数：确保通过基类指针删除派生类对象时，能正确调用到派生类的析构函数，避免内存泄漏。
    // `= default` 表示使用编译器生成的默认实现。
    virtual ~Device() = default;

    // 通用属性
    int id{0};
    std::string name;
    int health{100}; // 健康度范围 0-100，归零视为损坏
    DeviceType type{DeviceType::Consumable};
    bool allowStudentReserve{true};
    std::vector<Reservation> reservations; // 预约记录列表，包含借用标记与实际开始时间

    // 根据当前时间与健康度实时计算设备状态（不依赖持久化状态）
    DeviceStatus getDynamicStatus(std::time_t now) const;

    // 返回当前活动预约的结束时间文本（便于前端展示“until HH:MM”）
    std::string getStatusDetails(std::time_t now) const;

    // 应用磨损：根据使用时长对设备状态进行衰减，各派生类实现具体逻辑
    virtual void applyWearAndTear(std::time_t durationSeconds) = 0;

    // 设备维护：基础行为为健康度恢复至100，派生类可在此基础上重置自身特有状态
    virtual void maintain();

    // 获取当前时间窗口内属于指定用户的预约索引（若存在）
    std::optional<size_t> findActiveReservationIndex(std::time_t now, int userId) const;
    // 获取当前时间正在进行的预约索引（任意用户）
    std::optional<size_t> findActiveReservationIndex(std::time_t now) const;

    // 查找该用户的“已借出”预约索引（可能已过期但仍标记为借用）
    std::optional<size_t> findBorrowedReservationIndexByUser(int userId) const;

    // 维护/删除的可行性判断：默认规则为“借用中不可维护或删除”；子类可根据设备特性扩展
    virtual bool canMaintain(std::time_t now) const;
    virtual bool canDelete(std::time_t now) const;
};

// 耗材型设备（如 3D 打印机）：材料消耗明显，维护会补充材料
class ConsumableDevice : public Device {
public:
    double materialLevel{100.0}; // 材料剩余百分比
    ConsumableDevice();
    void applyWearAndTear(std::time_t durationSeconds) override;
    void maintain() override;
};

// 精密型设备（如显微镜）：校准度随使用下降，维护会重置校准
class PrecisionDevice : public Device {
public:
    double calibration{100.0}; // 校准度百分比
    PrecisionDevice();
    void applyWearAndTear(std::time_t durationSeconds) override;
    void maintain() override;
};

// 动力型设备（如离心机）：使用会升温且影响健康，维护恢复常温
class PowerDevice : public Device {
public:
    double temperature{25.0}; // 摄氏温度
    PowerDevice();
    void applyWearAndTear(std::time_t durationSeconds) override;
    void maintain() override;
};

