#include "User.h"
#include <functional>

// 基础行为实现
bool User::canReserve() const {
    return creditScore > 0;
}

void User::deductCredit(int amount) {
    if (amount <= 0) return;
    creditScore -= amount;
}

void User::restoreCredit() {
    // 恢复为各角色的初始信用分
    switch (type) {
        case UserType::Student: creditScore = 100; break;
        case UserType::Teacher: creditScore = 200; break;
        case UserType::Admin:   creditScore = 500; break;
    }
}

bool User::verifyPassword(const std::string &plain) const {
    // 演示用简单哈希对比（不安全，仅示例）
    std::hash<std::string> h;
    return h(plain) == h(passwordHash);
}

// 派生类构造：设置优先级与初始信用
Student::Student() {
    type = UserType::Student;
    priority = 1;
    creditScore = 100;
}

Teacher::Teacher() {
    type = UserType::Teacher;
    priority = 10;
    creditScore = 200;
}

Admin::Admin() {
    type = UserType::Admin;
    priority = 99;
    creditScore = 500;
}

