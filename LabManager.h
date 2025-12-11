#pragma once
// 系统核心控制器：封装用户、设备与预约的业务流程，提供统一的服务接口

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <ctime>

#include "User.h"
#include "Device.h"
#include "ConflictPolicy.h"

class LabManager {
public:
    // 用户与设备存储
    std::unordered_map<int, std::shared_ptr<User>> usersById;
    std::unordered_map<std::string, int> usernameToId;
    std::unordered_map<int, std::shared_ptr<Device>> devicesById;

    // 自增ID计数器
    int nextUserId{1};
    int nextDeviceId{1};

    // 初始化演示数据：创建默认用户与设备
    void seed();

    // 鉴权登录：返回用户ID或空（失败）
    std::optional<int> authenticate(const std::string &username, const std::string &password);

    // 用户查询
    std::shared_ptr<User> getUser(int userId);

    // 设备管理
    int addDevice(DeviceType type, const std::string &name, bool allowStudent);
    bool deleteDevice(int deviceId);
    bool maintainDevice(int deviceId);

    // 预约相关
    bool reserve(int userId, int deviceId, std::time_t start, std::time_t end);
    bool reserve(int userId, int deviceId, std::time_t start, std::time_t end, bool bypassStudentRule);
    bool borrow(int userId, int deviceId, std::time_t now);
    bool returnDevice(int userId, int deviceId, std::time_t now);
    bool extend(int userId, int deviceId, std::time_t newEnd);

    struct Application { int id; int userId; int deviceId; std::time_t start; std::time_t end; std::string reason; };
    int nextApplicationId{1};
    std::vector<Application> applications;
    int apply(int userId, int deviceId, std::time_t start, std::time_t end, const std::string &reason);
    bool approveApplication(int appId);

    struct Notification { int id; int userId; std::string message; std::time_t createdAt; };
    int nextNotificationId{1};
    std::vector<Notification> notifications;
    std::vector<Notification> popNotifications(int userId);

    // 面向对象：冲突策略
    std::unique_ptr<IConflictPolicy> conflictPolicy;

    // 工具方法：区间重叠判断
    static bool isOverlap(std::time_t s1, std::time_t e1, std::time_t s2, std::time_t e2);
};

