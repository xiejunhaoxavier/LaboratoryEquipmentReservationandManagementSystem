// HTTP 服务器入口（替代 main.cpp）：提供设备与预约相关的 REST 接口
#include <iostream>
#include <string>
#include <ctime>
#include <fstream>
#include "httplib.h"    // 引入 cpp-httplib 单头文件库（外部依赖）
#include "json.hpp"     // 引入 nlohmann/json 单头文件（外部依赖）

#include "LabManager.h"

using json = nlohmann::json;

int main() {
    LabManager mgr;
    mgr.seed();

    httplib::Server svr;
    svr.Get("/", [&](const httplib::Request &req, httplib::Response &res) {
        std::ifstream f("index.html", std::ios::binary);
        if (!f) {
            res.status = 404;
            res.set_content("index.html not found", "text/plain; charset=UTF-8");
            return;
        }
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        res.set_content(content, "text/html; charset=UTF-8");
    });

    auto add_cors = [&](httplib::Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    };

    // 登录接口
    svr.Options("/api/login", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/login", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            std::cout << "[LOGIN] body=" << req.body << std::endl;
            auto body = json::parse(req.body);
            std::string username = body.value("username", "");
            std::string password = body.value("password", "");
            auto uidOpt = mgr.authenticate(username, password);
            if (!uidOpt.has_value()) {
                std::cout << "[LOGIN] failed user=" << username << std::endl;
                res.status = 401;
                res.set_content(json({{"ok", false}, {"message", "登录失败"}}).dump(), "application/json");
                add_cors(res);
                return;
            }
            int uid = uidOpt.value();
            auto u = mgr.getUser(uid);
            json out{{"ok", true}, {"userId", uid}, {"username", u->username}, {"credit", u->creditScore}, {"priority", u->priority}, {"type", (int)u->type}};
            res.set_content(out.dump(), "application/json");
            add_cors(res);
        } catch (...) {
            std::cout << "[LOGIN] exception while parsing body" << std::endl;
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 设备列表（含动态状态）
    svr.Options("/api/devices", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Get("/api/devices", [&](const httplib::Request &req, httplib::Response &res) {
        std::time_t now = std::time(nullptr);
        json arr = json::array();
        for (const auto &kv : mgr.devicesById) {
            const auto &d = kv.second;
            json dev{{"id", d->id}, {"name", d->name}, {"type", (int)d->type}, {"health", d->health}, {"status", (int)d->getDynamicStatus(now)}, {"allowStudent", d->allowStudentReserve}};
            // 附加派生设备状态
            if (d->type == DeviceType::Consumable) {
                dev["materialLevel"] = static_cast<ConsumableDevice*>(d.get())->materialLevel;
            } else if (d->type == DeviceType::Precision) {
                dev["calibration"] = static_cast<PrecisionDevice*>(d.get())->calibration;
            } else if (d->type == DeviceType::Power) {
                dev["temperature"] = static_cast<PowerDevice*>(d.get())->temperature;
            }
            // 预约概览
            json rs = json::array();
            for (const auto &r : d->reservations) {
                rs.push_back({{"userId", r.userId}, {"startTime", (long long)r.startTime}, {"endTime", (long long)r.endTime}, {"borrowed", r.borrowed}});
            }
            dev["reservations"] = rs;
            arr.push_back(dev);
        }
        res.set_content(json({{"ok", true}, {"devices", arr}}).dump(), "application/json");
        add_cors(res);
    });

    // 预约
    svr.Options("/api/reserve", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/reserve", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int userId = body.at("userId").get<int>();
            int deviceId = body.at("deviceId").get<int>();
            long long start = body.at("startTime").get<long long>();
            long long end = body.at("endTime").get<long long>();
            // 后端允许开始时间略早于当前（在 LabManager 中处理）
            bool ok = mgr.reserve(userId, deviceId, static_cast<std::time_t>(start), static_cast<std::time_t>(end));
            std::string message = "";
            auto it = mgr.devicesById.find(deviceId);
            if (!ok && it != mgr.devicesById.end()) {
                // 借用中提示更明确
                if (it->second->getDynamicStatus(std::time(nullptr)) == DeviceStatus::IN_USE) message = "设备正在使用，无法预约";
            }
            res.set_content(json({{"ok", ok}, {"message", message}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 借用
    svr.Options("/api/borrow", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/borrow", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int userId = body.at("userId").get<int>();
            int deviceId = body.at("deviceId").get<int>();
            std::time_t now = std::time(nullptr);
            bool ok = mgr.borrow(userId, deviceId, now);
            res.set_content(json({{"ok", ok}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 归还
    svr.Options("/api/return", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/return", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int userId = body.at("userId").get<int>();
            int deviceId = body.at("deviceId").get<int>();
            std::time_t now = std::time(nullptr);
            bool ok = mgr.returnDevice(userId, deviceId, now);
            auto u = mgr.getUser(userId);
            int credit = u ? u->creditScore : 0;
            res.set_content(json({{"ok", ok}, {"credit", credit}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 延长
    svr.Options("/api/extend", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/extend", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int userId = body.at("userId").get<int>();
            int deviceId = body.at("deviceId").get<int>();
            std::time_t newEnd = body.at("endTime").get<long long>();
            bool ok = mgr.extend(userId, deviceId, newEnd);
            auto u = mgr.getUser(userId);
            int credit = u ? u->creditScore : 0;
            res.set_content(json({{"ok", ok}, {"credit", credit}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 管理员接口——新增设备
    svr.Options("/api/admin/add", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/admin/add", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int type = body.at("type").get<int>();
            std::string name = body.at("name").get<std::string>();
            bool allowStudent = body.value("allowStudent", true);
            int id = mgr.addDevice(static_cast<DeviceType>(type), name, allowStudent);
            res.set_content(json({{"ok", true}, {"deviceId", id}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    svr.Options("/api/apply", [&](const httplib::Request &req, httplib::Response &res) { add_cors(res); res.status = 200; });
    svr.Post("/api/apply", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int userId = body.at("userId").get<int>();
            int deviceId = body.at("deviceId").get<int>();
            long long start = body.at("startTime").get<long long>();
            long long end   = body.at("endTime").get<long long>();
            std::string reason = body.value("reason", "");
            int appId = mgr.apply(userId, deviceId, static_cast<std::time_t>(start), static_cast<std::time_t>(end), reason);
            res.set_content(json({{"ok", true}, {"applicationId", appId}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    svr.Options("/api/admin/applications", [&](const httplib::Request &req, httplib::Response &res) { add_cors(res); res.status = 200; });
    svr.Get("/api/admin/applications", [&](const httplib::Request &req, httplib::Response &res) {
        json arr = json::array();
        for (const auto &a : mgr.applications) {
            arr.push_back({{"id", a.id}, {"userId", a.userId}, {"deviceId", a.deviceId}, {"startTime", (long long)a.start}, {"endTime", (long long)a.end}, {"reason", a.reason}});
        }
        res.set_content(json({{"ok", true}, {"applications", arr}}).dump(), "application/json");
        add_cors(res);
    });

    svr.Options("/api/admin/applications/approve", [&](const httplib::Request &req, httplib::Response &res) { add_cors(res); res.status = 200; });
    svr.Post("/api/admin/applications/approve", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int appId = body.at("appId").get<int>();
            bool ok = mgr.approveApplication(appId);
            res.set_content(json({{"ok", ok}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 管理员接口——删除设备
    svr.Options("/api/admin/delete", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/admin/delete", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int deviceId = body.at("deviceId").get<int>();
            bool ok = mgr.deleteDevice(deviceId);
            res.set_content(json({{"ok", ok}, {"message", ok?"":"设备正在借用，无法删除"}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 管理员接口——维护设备
    svr.Options("/api/admin/maintain", [&](const httplib::Request &req, httplib::Response &res) {
        add_cors(res);
        res.status = 200;
    });
    svr.Post("/api/admin/maintain", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto body = json::parse(req.body);
            int deviceId = body.at("deviceId").get<int>();
            bool ok = mgr.maintainDevice(deviceId);
            res.set_content(json({{"ok", ok}, {"message", ok?"":"设备正在借用，无法维护"}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    // 学生通知：弹出并清除
    svr.Get("/api/notifications", [&](const httplib::Request &req, httplib::Response &res) {
        try {
            auto q = req.params.find("userId");
            if (q == req.params.end()) {
                res.set_content(json({{"ok", false}, {"message", "缺少userId"}}).dump(), "application/json"); add_cors(res); return; }
            int userId = std::stoi(q->second);
            auto list = mgr.popNotifications(userId);
            json arr = json::array();
            for (const auto &n : list) arr.push_back({{"id", n.id}, {"message", n.message}, {"createdAt", (long long)n.createdAt}});
            res.set_content(json({{"ok", true}, {"notifications", arr}}).dump(), "application/json");
            add_cors(res);
        } catch (...) {
            res.status = 400;
            res.set_content(json({{"ok", false}, {"message", "请求格式错误"}}).dump(), "application/json");
            add_cors(res);
        }
    });

    std::cout << "Server listening on http://localhost:8080" << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
