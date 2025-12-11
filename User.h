#pragma once
// 用户类层次结构定义：基础用户行为接口，学生/教师/管理员通过派生类区分

#include <string>
#include "Types.h"

class User {
public:
    // 虚析构以支持多态删除
    virtual ~User() = default;

    // 基础属性
    int id{0};
    std::string username;
    std::string passwordHash; // 演示用哈希（非生产安全方案）
    int creditScore{0};
    int priority{0};
    UserType type{UserType::Student};

    // 行为接口

    // 检查用户是否具有预约权限（如检查信用分），虚函数，支持不同用户类型的特定规则
    virtual bool canReserve() const;

    // 扣除信用分（如违约时调用），虚函数，支持多态行为
    virtual void deductCredit(int amount);

    // 恢复信用分至初始值，虚函数
    virtual void restoreCredit();

    // 验证登录密码，虚函数
    virtual bool verifyPassword(const std::string &plain) const;
};

// 学生用户
class Student : public User {
public:
    Student();
};

// 教师用户
class Teacher : public User {
public:
    Teacher();
};

// 管理员用户
class Admin : public User {
public:
    Admin();
};

