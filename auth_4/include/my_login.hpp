#pragma once
#include "crow.h"
#include <pqxx/pqxx>
#include <cstdlib>
#include <iostream>
#include <string>

#include "./auth.hpp"

class MyLogin {
private:
    std::string m_name;

    public:
    explicit MyLogin(std::string str){}
    ~MyLogin() {}

    auto login(const crow::request& req, std::string db_conn)
    {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("email")) {
            crow::json::wvalue err;
            err["error"] = "email , require";
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

        try{
            pqxx::connection conn(db_conn);
            pqxx::work txn(conn);
            pqxx::result r = txn.exec(
                "SELECT id, email , password_hash FROM users WHERE email = $1",
                pqxx::params{email}
            );

            std::vector<crow::json::wvalue> todos;
            todos.reserve(r.size());
            int user_count = 0;
            std::string db_pass = "";
            for (const auto& row : r) {
                db_pass = row["password_hash"].as<std::string>();
                std::cout << "id=" << row["id"].as<int>() << std::endl;
                //std::cout << "password_hash=" << db_pass << std::endl;
                user_count += 1;
            }
            std::cout << "user_count=" << user_count << std::endl;
            if (user_count == 0) {
                crow::json::wvalue err;
                err["error"] = "error, user none";
                return crow::response(400, err);
            }
            Auth auth;
            if (!auth.verifyPassword(password, db_pass)) {
                crow::json::wvalue err;
                err["error"] = "error, password NG";
                return crow::response(400, err);              
            }

            crow::json::wvalue result;
            result["message"] = "OK";
            return crow::response(200, result);            
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }

    }

};
