#pragma once
#include "Types.h"

// 冲突策略接口：用于在预约时间重叠时决定如何处理（允许、移除既有、拒绝新预约）
enum class ConflictDecision { Allow, RemoveExisting, RejectNew };

class IConflictPolicy {
public:
    virtual ~IConflictPolicy() = default;
    virtual ConflictDecision decide(UserType newUserType, UserType existingUserType, bool existingBorrowed) const = 0;
};

// 默认策略说明：
// - 已借用：一律拒绝新的预约插入（保护正在使用者）
// - 教师 vs 学生（未借用）：教师优先，移除学生的冲突预约并通知
// - 其他情况：拒绝新预约
class DefaultConflictPolicy : public IConflictPolicy {
public:
    ConflictDecision decide(UserType newUserType, UserType existingUserType, bool existingBorrowed) const override {
        if (existingBorrowed) return ConflictDecision::RejectNew;
        if (newUserType == UserType::Teacher && existingUserType == UserType::Student) {
            return ConflictDecision::RemoveExisting;
        }
        return ConflictDecision::RejectNew;
    }
};

