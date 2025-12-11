#include "Device.h"
#include <algorithm>

// 设备状态按需实时计算：优先检查健康度，其次判断当前时间命中预约区间与借用标记
DeviceStatus Device::getDynamicStatus(std::time_t now) const {
    if (health <= 0) return DeviceStatus::BROKEN;
    for (const auto &r : reservations) {
        if (now >= r.startTime && now <= r.endTime) {
            return r.borrowed ? DeviceStatus::IN_USE : DeviceStatus::RESERVED;
        }
    }
    return DeviceStatus::IDLE;
}

std::string Device::getStatusDetails(std::time_t now) const {
    if (health <= 0) return "BROKEN";
    for (const auto &r : reservations) {
        if (now >= r.startTime && now <= r.endTime) {
            char buf[32];
            std::tm *tm = std::localtime(&r.endTime);
            std::snprintf(buf, sizeof(buf), "%02d:%02d", tm->tm_hour, tm->tm_min);
            return std::string("until ") + buf;
        }
    }
    return "IDLE";
}

// 维护基础行为：健康度恢复到满值 100
void Device::maintain() {
    health = 100;
}

// 查找当前时间窗口内指定用户的预约索引
std::optional<size_t> Device::findActiveReservationIndex(std::time_t now, int userId) const {
    for (size_t i = 0; i < reservations.size(); ++i) {
        const auto &r = reservations[i];
        if (r.userId == userId && now >= r.startTime && now <= r.endTime) {
            return i;
        }
    }
    return std::nullopt;
}

// 查找当前正在进行的预约索引（任意用户）
std::optional<size_t> Device::findActiveReservationIndex(std::time_t now) const {
    for (size_t i = 0; i < reservations.size(); ++i) {
        const auto &r = reservations[i];
        if (now >= r.startTime && now <= r.endTime) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<size_t> Device::findBorrowedReservationIndexByUser(int userId) const {
    for (size_t i = 0; i < reservations.size(); ++i) {
        const auto &r = reservations[i];
        if (r.userId == userId && r.borrowed) {
            return i;
        }
    }
    return std::nullopt;
}

bool Device::canMaintain(std::time_t now) const {
    // 借用中不可维护：避免干扰正在使用的用户
    if (getDynamicStatus(now) == DeviceStatus::IN_USE) return false;
    for (const auto &r : reservations) if (r.borrowed) return false;
    return true;
}

bool Device::canDelete(std::time_t now) const {
    // 借用中不可删除：维护数据一致性与用户体验
    if (getDynamicStatus(now) == DeviceStatus::IN_USE) return false;
    for (const auto &r : reservations) if (r.borrowed) return false;
    return true;
}

// 耗材设备实现：材料与健康随使用时长线性下降
// 这段代码体现了面向对象中的**继承**和**多态**特性。
// - **继承**：ConsumableDevice 继承自 Device 基类，获得了 Device 的所有属性和方法。
// - **多态**：ConsumableDevice 重写 (override) 了 Device 的纯虚函数 applyWearAndTear 和虚函数 maintain，
//           实现了针对耗材设备特有的磨损和维护逻辑。在运行时，通过基类指针调用这些方法时，
//           会根据实际对象的类型调用到 ConsumableDevice 的具体实现。
ConsumableDevice::ConsumableDevice() {
    type = DeviceType::Consumable;
}

void ConsumableDevice::applyWearAndTear(std::time_t durationSeconds) {
    // 耗材按使用时长消耗材料与健康
    double hours = durationSeconds / 3600.0;
    materialLevel -= hours * 5.0;     // 每小时消耗约 5%
    health -= static_cast<int>(hours * 2.0); // 健康每小时降低约 2
    if (materialLevel < 0) materialLevel = 0;
    if (health < 0) health = 0;
}

void ConsumableDevice::maintain() {
    Device::maintain();
    materialLevel = 100.0;
}

// 精密设备实现：以校准度下降为主，健康下降较缓
// 这段代码同样体现了面向对象中的**继承**和**多态**特性。
// - **继承**：PrecisionDevice 继承自 Device 基类。
// - **多态**：PrecisionDevice 重写 (override) 了 Device 的纯虚函数 applyWearAndTear 和虚函数 maintain，
//           实现了针对精密设备特有的磨损和维护逻辑。
PrecisionDevice::PrecisionDevice() {
    type = DeviceType::Precision;
}

void PrecisionDevice::applyWearAndTear(std::time_t durationSeconds) {
    // 校准度随使用时长下降
    double hours = durationSeconds / 3600.0;
    calibration -= hours * 8.0;          // 每小时校准度下降约 8%
    health -= static_cast<int>(hours * 1.0); // 健康每小时降低约 1
    if (calibration < 0) calibration = 0;
    if (health < 0) health = 0;
}

void PrecisionDevice::maintain() {
    Device::maintain();
    calibration = 100.0;
}

// 动力设备实现：使用会显著升温并影响健康
PowerDevice::PowerDevice() {
    type = DeviceType::Power;
}

void PowerDevice::applyWearAndTear(std::time_t durationSeconds) {
    // 升温显著，同时造成健康下降
    double hours = durationSeconds / 3600.0;
    temperature += hours * 10.0;           // 每小时升温约 10℃
    health -= static_cast<int>(hours * 3.0); // 健康每小时降低约 3
    if (health < 0) health = 0;
}

void PowerDevice::maintain() {
    Device::maintain();
    temperature = 25.0; // 复位至常温
}

