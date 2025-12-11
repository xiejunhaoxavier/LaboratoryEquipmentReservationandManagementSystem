# 🧪 实验室设备管理系统 (Lab Equipment Management System)

> 一个基于 C++17 开发的实验室设备全生命周期管理系统，采用 B/S 架构，深度应用面向对象设计原则。

## 📖 项目简介

本项目旨在解决高校或研究机构中实验室设备资源分配不均、预约冲突频发以及设备维护管理困难等问题。系统通过精细化的角色权限控制、智能的预约冲突解决策略以及模拟真实的设备物理特性（磨损与维护），实现了从设备预约、借用、归还到维护的全流程闭环管理。

## ✨ 核心功能

*   **👥 多角色权限体系**：
    *   **学生**：预约开放设备、提交特殊申请、查看个人信用分。
    *   **教师**：享有更高优先级，可抢占学生预约，拥有更多设备访问权限。
    *   **管理员**：设备增删改查、审批学生申请、全局系统维护。

*   **📅 智能预约与冲突解决**：
    *   内置策略引擎，自动处理时间重叠的预约请求。
    *   支持基于角色的抢占机制（如教师优先于学生）。
    *   支持申请审批流程，灵活处理特殊需求。

*   **🔧 设备全生命周期模拟**：
    *   **动态状态**：实时计算设备状态（空闲、预约中、使用中、故障）。
    *   **物理磨损**：模拟不同类型设备的损耗逻辑（耗材消耗、精度下降、过热）。
    *   **维护机制**：故障设备必须维护后方可重新上架。

*   **🔔 实时通知系统**：
    *   预约被抢占或申请通过时，自动向用户发送通知。

## 🛠️ 技术架构

*   **后端**：C++17
    *   Web 服务器：`cpp-httplib` (轻量级 HTTP 库)
    *   JSON 处理：`nlohmann/json`
    *   设计模式：策略模式 (Strategy Pattern)、工厂模式思想
*   **前端**：原生 HTML5 / CSS3 / JavaScript (ES6+)
    *   通信：Fetch API (RESTful 风格)
    *   UI：响应式布局，无需复杂框架依赖

## 🧬 面向对象设计亮点

本项目严格遵循 OOP 设计原则，核心代码结构如下：

1.  **多态 (Polymorphism)**：
    *   **统一接口**：`Device` 基类定义了 `applyWearAndTear`（磨损）和 `maintain`（维护）虚函数。
    *   **差异实现**：
        *   `ConsumableDevice`（耗材型）：扣减材料。
        *   `PrecisionDevice`（精密型）：降低校准度。
        *   `PowerDevice`（动力型）：升高温度。
    *   **优势**：控制器层 (`LabManager`) 无需关心设备具体类型，统一调用接口，符合开闭原则。

2.  **策略模式 (Strategy Pattern)**：
    *   将预约冲突解决逻辑抽象为 `IConflictPolicy` 接口。
    *   当前实现 `DefaultConflictPolicy`（教师优先），未来可轻松扩展其他策略（如信用分优先），无需修改核心业务代码。

3.  **封装 (Encapsulation)**：
    *   `LabManager` 作为核心控制器，对外隐藏了用户和设备容器的具体实现，仅暴露业务操作接口。

## 🚀 快速开始

### 环境要求
*   C++ 编译器 (支持 C++17 标准，如 GCC, Clang, MSVC)
*   Windows / Linux / macOS 均可

### 编译与运行

1.  **克隆仓库**
    ```bash
    git clone https://github.com/your-username/lab-management-system.git
    cd lab-management-system
    ```

2.  **编译**
    ```bash
    # 使用 g++
    g++ -std=c++17 -o main main.cpp LabManager.cpp Device.cpp User.cpp Server.cpp -lpthread -lws2_32
    # 注意：Windows下需要链接 ws2_32 库
    ```

3.  **运行**
    ```bash
    ./main
    ```

4.  **访问**
    打开浏览器访问 `http://localhost:8080`

### 默认测试账号
*   **学生**: `student1` / `123456`
*   **教师**: `teacher1` / `123456`
*   **管理员**: `admin1` / `123456`

## 📂 文件结构说明

*   `Server.cpp`: HTTP 服务器入口，路由分发。
*   `LabManager.h/cpp`: 核心业务逻辑控制器。
*   `Device.h/cpp`: 设备类定义与多态实现。
*   `User.h/cpp`: 用户类定义与继承体系。
*   `ConflictPolicy.h`: 冲突策略接口与实现。
*   `index.html`: 前端单页应用入口。


© 2023 Lab Management System Project
