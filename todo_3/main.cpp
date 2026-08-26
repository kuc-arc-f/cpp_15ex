// main.cpp
// Crow + libpqxx を使った TODO アプリ REST API
// 機能: 登録(POST) / 一覧(GET) / 削除(DELETE)
//#include <libpq-fe.h>

#include "crow.h"
#include <pqxx/pqxx>
#include <cstdlib>
#include <iostream>
#include <string>

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

    // ---------- 一覧取得 ----------
    CROW_ROUTE(app, "/todos").methods("GET"_method)
    ([]() {
        try {
            pqxx::connection conn(getConnStr());
            pqxx::work txn(conn);
            pqxx::result r = txn.exec(
                "SELECT id, title, done, created_at FROM todos ORDER BY id DESC"
            );

            std::vector<crow::json::wvalue> todos;
            todos.reserve(r.size());
            for (const auto& row : r) {
                todos.push_back(rowToJson(row));
            }

            crow::json::wvalue result;
            result["todos"] = std::move(todos);
            return crow::response(200, result);
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });

    // ---------- 登録 ----------
    CROW_ROUTE(app, "/todos").methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("title")) {
            crow::json::wvalue err;
            err["error"] = "title フィールドは必須です";
            return crow::response(400, err);
        }

        std::string title;
        try {
            title = body["title"].s();
        } catch (...) {
            crow::json::wvalue err;
            err["error"] = "title は文字列で指定してください";
            return crow::response(400, err);
        }

        if (title.empty()) {
            crow::json::wvalue err;
            err["error"] = "title は空にできません";
            return crow::response(400, err);
        }

        try {
            pqxx::connection conn(getConnStr());
            pqxx::work txn(conn);
            pqxx::result r = txn.exec_params(
                "INSERT INTO todos (title) VALUES ($1) "
                "RETURNING id, title, done, created_at",
                title
            );
            txn.commit();

            crow::json::wvalue todo = rowToJson(r[0]);
            return crow::response(201, todo);
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });

    // ---------- 削除 ----------
    CROW_ROUTE(app, "/todos/<int>").methods("DELETE"_method)
    ([](int id) {
        try {
            pqxx::connection conn(getConnStr());
            pqxx::work txn(conn);
            pqxx::result r = txn.exec_params(
                "DELETE FROM todos WHERE id = $1",
                id
            );
            txn.commit();

            if (r.affected_rows() == 0) {
                crow::json::wvalue err;
                err["error"] = "指定された id の TODO が見つかりません";
                return crow::response(404, err);
            }

            crow::json::wvalue result;
            result["message"] = "deleted";
            result["id"] = id;
            return crow::response(200, result);
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });
    app.port(8080).multithreaded().run();
    return 0;
}
