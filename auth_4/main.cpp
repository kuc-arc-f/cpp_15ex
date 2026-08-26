// main.cpp
// Crow + libpqxx を使った TODO アプリ REST API
// 機能: 登録(POST) / 一覧(GET) / 削除(DELETE)

#include "crow.h"
#include <pqxx/pqxx>
#include <cstdlib>
#include <iostream>
#include <string>

#include "./include/auth.hpp"
#include "./include/my_login.hpp"

// 環境変数 DATABASE_URL があればそれを使用、なければデフォルト値を使用
static std::string getConnStr() {
    const char* env = std::getenv("DATABASE_URL");
    if (env != nullptr && std::string(env).size() > 0) {
        return std::string(env);
    }
    return "postgresql://postgres:admin@localhost:5432/mydb123";
}

// pqxx の行から TODO の JSON を組み立てる
static crow::json::wvalue rowToJson(const pqxx::row& row) {
    crow::json::wvalue todo;
    todo["id"] = row["id"].as<int>();
    todo["title"] = row["title"].as<std::string>();
    todo["done"] = row["done"].as<bool>();
    todo["created_at"] = row["created_at"].as<std::string>();
    return todo;
}

int main() {
    crow::SimpleApp app;

    // ヘルスチェック用
    CROW_ROUTE(app, "/health")
    ([]() {
        return crow::response(200, "OK");
    });
    // ---------- 登録 ----------
    CROW_ROUTE(app, "/signup").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("email")) {
            crow::json::wvalue err;
            err["error"] = "email フィールドは必須です";
            return crow::response(400, err);
        }
        if (!body || !body.has("password")) {
            crow::json::wvalue err;
            err["error"] = "password フィールドは必須です";
            return crow::response(400, err);
        }
        std::string email;
        std::string password;
        try {
            email = body["email"].s();
        } catch (...) {
            crow::json::wvalue err;
            err["error"] = "email は文字列で指定してください";
            return crow::response(400, err);
        }
        try {
            password = body["password"].s();
        } catch (...) {
            crow::json::wvalue err;
            err["error"] = "password , input text";
            return crow::response(400, err);
        }

        if (email.empty()) {
            crow::json::wvalue err;
            err["error"] = "email は空にできません";
            return crow::response(400, err);
        }
        if (password.empty()) {
            crow::json::wvalue err;
            err["error"] = "password , require";
            return crow::response(400, err);
        }   
        std::cout << "email=" << email << std::endl;  
        std::cout << "password=" << password << std::endl;  

        try {
            Auth auth;
            std::string password_hash = auth.hashPassword(password);  
            //std::cout << "password_hash=" << password_hash << std::endl;           

            pqxx::connection conn(getConnStr());
            pqxx::work txn(conn);
            pqxx::result r = txn.exec(
                "SELECT id, email FROM users WHERE email = $1",
                pqxx::params{email}
            );

            std::vector<crow::json::wvalue> todos;
            todos.reserve(r.size());
            int user_count = 0;
            for (const auto& row : r) {
                user_count += 1;
            }
            std::cout << "user_count=" << user_count << std::endl;
            if (user_count > 0) {
                crow::json::wvalue err;
                err["error"] = "error, user exist";
                return crow::response(400, err);
            }
            pqxx::result r2 = txn.exec_params(
                "INSERT INTO users (email, password_hash) VALUES ($1, $2) "
                "RETURNING id, email, created_at",
                email,
                password_hash
            );
            txn.commit();

            crow::json::wvalue result;
            result["message"] = "OK";
            return crow::response(200, result);            
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });

    CROW_ROUTE(app, "/login").methods("POST"_method)
    ([](const crow::request& req) {
        MyLogin lo("");
        std::string conn = getConnStr();
        auto result = lo.login(req, conn);
        return result;
    });
    
    app.port(8080).multithreaded().run();
    return 0;
}
