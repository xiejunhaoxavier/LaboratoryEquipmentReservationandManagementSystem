#include "LabManager.h"
#include <algorithm>
#include <functional>

// 演示数据初始化：创建三个角色用户与若干设备，并设置默认冲突策略
// 这里展示了如何初始化系统的基础状态，包括用户对象和不同类型的设备对象
void LabManager::seed() {
    // 用户初始化：创建 Student, Teacher, Admin 三种类型的用户对象
    // 使用 std::make_shared 创建智能指针，管理用户对象的生命周期
    {
        auto u = std::make_shared<Student>();
        u->id = nextUserId++;
        u->username = "student1";
        u->passwordHash = "123456"; // 演示用简单哈希（实际应更安全）
        usersById[u->id] = u;
        usernameToId[u->username] = u->id;
    }
    {
        auto u = std::make_shared<Teacher>();
        u->id = nextUserId++;
        u->username = "teacher1";
        u->passwordHash = "123456";
        usersById[u->id] = u;
        usernameToId[u->username] = u->id;
    }
    {
        auto u = std::make_shared<Admin>();
        u->id = nextUserId++;
        u->username = "admin1";
        u->passwordHash = "123456";
        usersById[u->id] = u;
        usernameToId[u->username] = u->id;
    }

    // 设备初始化：根据规格创建不同类型的设备（ConsumableDevice, PrecisionDevice, PowerDevice）
    // 体现了多态性：不同类型的设备统一存储在 std::shared_ptr<Device> 容器中
    { auto d = std::make_shared<ConsumableDevice>(); d->id = nextDeviceId++; d->name = "3D打印机 A"; d->allowStudentReserve = true; devicesById[d->id] = d; }
    { auto d = std::make_shared<PrecisionDevice>(); d->id = nextDeviceId++; d->name = "电子显微镜"; d->allowStudentReserve = false; devicesById[d->id] = d; }
    { auto d = std::make_shared<PowerDevice>(); d->id = nextDeviceId++; d->name = "离心机 X"; d->allowStudentReserve = true; devicesById[d->id] = d; }
    { auto d = std::make_shared<PowerDevice>(); d->id = nextDeviceId++; d->name = "GPU 集群"; d->allowStudentReserve = false; devicesById[d->id] = d; }
    { auto d = std::make_shared<PowerDevice>(); d->id = nextDeviceId++; d->name = "培养箱"; d->allowStudentReserve = true; devicesById[d->id] = d; }
    { auto d = std::make_shared<ConsumableDevice>(); d->id = nextDeviceId++; d->name = "激光切割机"; d->allowStudentReserve = false; devicesById[d->id] = d; }
    { auto d = std::make_shared<PrecisionDevice>(); d->id = nextDeviceId++; d->name = "示波器"; d->allowStudentReserve = true; devicesById[d->id] = d; }
    { auto d = std::make_shared<PrecisionDevice>(); d->id = nextDeviceId++; d->name = "光谱仪"; d->allowStudentReserve = false; devicesById[d->id] = d; }
    { auto d = std::make_shared<ConsumableDevice>(); d->id = nextDeviceId++; d->name = "绘图仪"; d->allowStudentReserve = true; devicesById[d->id] = d; }
    { auto d = std::make_shared<ConsumableDevice>(); d->id = nextDeviceId++; d->name = "化学试剂分配器"; d->allowStudentReserve = false; devicesById[d->id] = d; }

    // 初始化冲突策略：使用默认策略（DefaultConflictPolicy）
    // 这里使用了策略模式，允许在未来轻松替换为其他冲突解决策略
    conflictPolicy = std::make_unique<DefaultConflictPolicy>();
}

// 用户认证：验证用户名和密码
// 返回值使用 std::optional<int>，成功时返回用户ID，失败时返回 std::nullopt
std::optional<int> LabManager::authenticate(const std::string &username, const std::string &password) {
    auto it = usernameToId.find(username);
    // 用户名不存在
    if (it == usernameToId.end()) return std::nullopt;
    
    auto u = usersById[it->second];
    // 用户对象为空（异常情况）
    if (!u) return std::nullopt;
    
    // 调用 User 对象的 verifyPassword 虚函数进行密码验证
    // 支持不同类型的用户可能有不同的验证方式
    if (u->verifyPassword(password)) return u->id;
    
    return std::nullopt;
}

// 获取用户对象：根据用户ID查找
// 返回 std::shared_ptr<User>，若不存在则返回 nullptr
std::shared_ptr<User> LabManager::getUser(int userId) {
    auto it = usersById.find(userId);
    if (it == usersById.end()) return nullptr;
    return it->second;
}

// 添加设备：根据类型参数创建特定的设备对象（工厂模式思想）
// 参数：type-设备类型, name-设备名称, allowStudent-是否允许学生预约
int LabManager::addDevice(DeviceType type, const std::string &name, bool allowStudent) {
    std::shared_ptr<Device> d;
    // 根据类型创建具体的派生类对象
    switch (type) {
        case DeviceType::Consumable: d = std::make_shared<ConsumableDevice>(); break;
        case DeviceType::Precision:  d = std::make_shared<PrecisionDevice>();  break;
        case DeviceType::Power:      d = std::make_shared<PowerDevice>();      break;
    }
    d->id = nextDeviceId++;
    d->name = name;
    d->allowStudentReserve = allowStudent;
    // 存储基类指针：利用多态性统一管理不同类型的设备
    devicesById[d->id] = d;
    return d->id;
}

// 删除设备：在删除前检查设备是否处于可删除状态
// 返回 true 表示删除成功，false 表示失败（如设备正在使用中）
bool LabManager::deleteDevice(int deviceId) {
    auto it = devicesById.find(deviceId);
    if (it == devicesById.end()) return false;
    std::time_t now = std::time(nullptr);
    // 调用虚函数 canDelete：不同设备可能有不同的删除条件（如是否有未完成的预约）
    // 体现多态：运行时根据实际设备类型调用对应的检查逻辑
    if (!it->second->canDelete(now)) return false;
    return devicesById.erase(deviceId) > 0;
}

// 维护设备：对设备进行维护操作（如重置健康度、补充材料等）
// 返回 true 表示维护成功，false 表示失败（如设备正在使用中）
bool LabManager::maintainDevice(int deviceId) {
    auto it = devicesById.find(deviceId);
    if (it == devicesById.end()) return false;
    std::time_t now = std::time(nullptr);
    // 调用虚函数 canMaintain：检查设备当前是否可维护
    if (!it->second->canMaintain(now)) return false;
    // 调用虚函数 maintain：执行具体的维护操作
    // 体现多态：不同设备执行不同的维护逻辑（如ConsumableDevice补充材料，PrecisionDevice校准）
    it->second->maintain();
    return true;
}

// 辅助函数：检查两个时间段 [s1, e1) 和 [s2, e2) 是否重叠
// 原理：如果一个时间段的开始时间小于另一个时间段的结束时间，且反之亦然，则重叠
bool LabManager::isOverlap(std::time_t s1, std::time_t e1, std::time_t s2, std::time_t e2) {
    return std::max(s1, s2) < std::min(e1, e2);
}

// 预约设备（简化版）：默认必须遵守学生预约规则
bool LabManager::reserve(int userId, int deviceId, std::time_t start, std::time_t end) {
    return reserve(userId, deviceId, start, end, false);
}

// 预约设备（完整版）：处理预约请求，包括权限检查、设备状态验证和冲突解决
// 参数：bypassStudentRule - 是否绕过“学生不可预约特定设备”的规则（如管理员审批后的申请）
bool LabManager::reserve(int userId, int deviceId, std::time_t start, std::time_t end, bool bypassStudentRule) {
    if (start >= end) return false;
    std::time_t now = std::time(nullptr);
    // 允许开始时间略早于当前，容忍 120 秒，用于前端选择误差
    std::time_t adjStart = start;
    if (adjStart < now - 120) adjStart = now - 120;
    
    // 1. 用户检查：是否存在且有预约权限（调用虚函数 canReserve）
    auto u = getUser(userId);
    if (!u || !u->canReserve()) return false;
    
    // 2. 设备检查：是否存在
    auto it = devicesById.find(deviceId);
    if (it == devicesById.end()) return false;
    auto &dev = it->second;
    
    // 3. 规则检查：除非显式绕过，否则检查学生是否被允许预约此设备
    if (!bypassStudentRule) {
        if (u->type == UserType::Student && !dev->allowStudentReserve) return false;
    }
    
    // 4. 健康度检查：设备损坏不可预约
    if (dev->health <= 0) return false;

    // 5. 冲突处理：使用策略模式解决时间重叠
    // 遍历设备当前的所有预约，检查是否有时间重叠
    std::vector<size_t> toRemove;
    for (size_t i = 0; i < dev->reservations.size(); ++i) {
        const auto &r = dev->reservations[i];
        // 重叠判断条件：!(新结束 <= 旧开始 || 新开始 >= 旧结束)
        bool overlap = !(end <= r.startTime || adjStart >= r.endTime);
        if (overlap) {
            auto ru = getUser(r.userId);
            if (!ru) return false;
            
            // 核心逻辑：调用冲突策略对象 (IConflictPolicy) 决定如何处理
            // 传入新用户类型、既有用户类型、既有预约是否已借出
            ConflictDecision d = conflictPolicy ? conflictPolicy->decide(u->type, ru->type, r.borrowed) : ConflictDecision::RejectNew;
            
            if (d == ConflictDecision::RejectNew) return false; // 策略决定拒绝新预约
            if (d == ConflictDecision::RemoveExisting) {
                // 策略决定移除既有预约（例如教师优先于学生），记录并通知被移除的用户
                notifications.push_back(Notification{ nextNotificationId++, r.userId, "您的预约已被教师优先占用，该设备对您暂不可用", std::time(nullptr) });
                toRemove.push_back(i);
            }
        }
    }
    
    // 执行删除操作（从后向前删除，避免索引失效）
    std::sort(toRemove.begin(), toRemove.end());
    for (int i = static_cast<int>(toRemove.size()) - 1; i >= 0; --i) {
        dev->reservations.erase(dev->reservations.begin() + toRemove[i]);
    }

    // 6. 成功预约：添加新的预约记录
    Reservation nr; nr.userId = userId; nr.startTime = adjStart; nr.endTime = end; nr.borrowed = false; nr.actualStartTime = 0;
    dev->reservations.push_back(nr);
    return true;
}

// 借用设备：用户开始使用已预约的设备
// 作用：标记预约状态为“已借出”，并记录实际开始使用时间
bool LabManager::borrow(int userId, int deviceId, std::time_t now) {
    auto it = devicesById.find(deviceId);
    if (it == devicesById.end()) return false;
    auto &dev = it->second;
    if (dev->health <= 0) return false;

    // 查找当前时间对应的预约记录
    auto idxOpt = dev->findActiveReservationIndex(now, userId);
    if (!idxOpt.has_value()) {
        // 如果没有当前预约，尝试查找“已借出但未归还”的记录（防止重复借用逻辑出错）
        idxOpt = dev->findBorrowedReservationIndexByUser(userId);
        if (!idxOpt.has_value()) return false;
    }
    auto &r = dev->reservations[idxOpt.value()];
    
    // 更新预约状态
    r.borrowed = true;
    r.actualStartTime = now;
    return true;
}

// 归还设备：用户结束使用
// 作用：计算使用时长，应用磨损，处理逾期，并移除预约记录
bool LabManager::returnDevice(int userId, int deviceId, std::time_t now) {
    auto it = devicesById.find(deviceId);
    if (it == devicesById.end()) return false;
    auto &dev = it->second;

    // 找到当前用户的进行中预约（若无则尝试已借用但未归还的记录）
    auto idxOpt = dev->findActiveReservationIndex(now, userId);
    if (!idxOpt.has_value()) {
        idxOpt = dev->findBorrowedReservationIndexByUser(userId);
        if (!idxOpt.has_value()) return false;
    }
    auto idx = idxOpt.value();
    auto &r = dev->reservations[idx];
    
    // 确保该预约确实处于借用状态
    if (!r.borrowed || r.actualStartTime == 0) return false;

    // 1. 应用磨损：计算实际使用时长并调用多态方法 applyWearAndTear
    std::time_t duration = now - r.actualStartTime;
    if (duration < 0) duration = 0;
    // 多态调用：不同设备根据自身特性（耗材消耗、精度下降等）更新健康度
    dev->applyWearAndTear(duration);

    // 2. 逾期处理：若当前时间超过预约结束时间，扣除用户信用分
    auto u = getUser(userId);
    if (!u) return false;
    if (now > r.endTime) { u->deductCredit(10); }

    // 3. 结束流程：归还后删除该预约记录，释放时间段
    r.borrowed = false;
    r.actualStartTime = 0;
    dev->reservations.erase(dev->reservations.begin() + idx);
    return true;
}

// 延长预约：在设备使用过程中申请延长结束时间
// 限制：只能延长不能缩短，且延长的时间段不能与其他人的预约冲突
bool LabManager::extend(int userId, int deviceId, std::time_t newEnd) {
    auto it = devicesById.find(deviceId);
    if (it == devicesById.end()) return false;
    auto &dev = it->second;

    // 遍历查找该用户在此设备上的预约（假设同时最多一个）
    for (size_t i = 0; i < dev->reservations.size(); ++i) {
        auto &r = dev->reservations[i];
        if (r.userId == userId) {
            if (newEnd <= r.endTime) return false; // 只能延长，不能缩短
            
            // 冲突检查：检查延长后的新时间段是否与后续其他预约重叠
            for (size_t j = 0; j < dev->reservations.size(); ++j) {
                if (j == i) continue; // 跳过自己
                const auto &next = dev->reservations[j];
                // 冲突条件：延长后的区间与其他预约重叠
                if (isOverlap(r.startTime, newEnd, next.startTime, next.endTime)) {
                    return false;
                }
            }
            
            // 检查当前是否已逾期（在延长操作之前）
            std::time_t now = std::time(nullptr);
            bool overdueBeforeExtend = now > r.endTime;
            
            // 执行延长
            r.endTime = newEnd;
            
            // 如果在已逾期的情况下才延长，仍需扣除一定的信用分作为惩罚
            if (overdueBeforeExtend) {
                auto u = getUser(userId);
                if (u) { u->deductCredit(5); }
            }
            return true;
        }
    }
    return false;
}

// 提交特殊申请：当直接预约不满足条件时（如学生想预约限制设备），提交申请由管理员审批
// 返回生成的申请ID
int LabManager::apply(int userId, int deviceId, std::time_t start, std::time_t end, const std::string &reason) {
    int id = nextApplicationId++;
    applications.push_back(Application{ id, userId, deviceId, start, end, reason });
    return id;
}

// 审批申请：管理员同意申请
// 成功审批后，将自动创建预约记录（bypassStudentRule=true，绕过学生限制规则）
bool LabManager::approveApplication(int appId) {
    for (size_t i = 0; i < applications.size(); ++i) {
        const auto &a = applications[i];
        if (a.id == appId) {
            // 调用 reserve 函数，并设置 bypassStudentRule 为 true
            bool ok = reserve(a.userId, a.deviceId, a.start, a.end, true);
            if (ok) {
                // 审批成功且预约成功后，从申请列表中移除
                applications.erase(applications.begin() + i);
            }
            return ok;
        }
    }
    return false;
}

// 获取并弹出通知：读取用户的通知消息，读取后即从系统中删除
// 用于前端轮询获取消息（如预约被移除的通知）
std::vector<LabManager::Notification> LabManager::popNotifications(int userId) {
    std::vector<Notification> out;
    // 收集属于该用户的通知
    for (size_t i = 0; i < notifications.size(); ++i) {
        if (notifications[i].userId == userId) out.push_back(notifications[i]);
    }
    // 移除已弹出的通知：保持通知列表干净，避免重复显示
    notifications.erase(std::remove_if(notifications.begin(), notifications.end(), [&](const Notification &n){ return n.userId == userId; }), notifications.end());
    return out;
}
