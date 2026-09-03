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
    return "postgresql://root:admin@localhost:5432/mydb";
}

// pqxx の行から TODO の JSON を組み立てる (schema.sql の全カラム対応・NULL安全)
static crow::json::wvalue rowToJson(const pqxx::row& row) {
    crow::json::wvalue todo;
    todo["id"] = row["id"].as<int>();
    todo["title"] = row["title"].is_null() ? "" : row["title"].as<std::string>();
    todo["content"] = row["content"].is_null() ? "" : row["content"].as<std::string>();
    todo["is_public"] = row["is_public"].is_null() ? false : row["is_public"].as<bool>();
    todo["food_orange"] = row["food_orange"].is_null() ? false : row["food_orange"].as<bool>();
    todo["food_apple"] = row["food_apple"].is_null() ? false : row["food_apple"].as<bool>();
    todo["food_banana"] = row["food_banana"].is_null() ? false : row["food_banana"].as<bool>();
    todo["pub_date"] = row["pub_date"].is_null() ? "" : row["pub_date"].as<std::string>();
    todo["qty1"] = row["qty1"].is_null() ? 0 : row["qty1"].as<int>();
    todo["qty2"] = row["qty2"].is_null() ? 0 : row["qty2"].as<int>();
    todo["qty3"] = row["qty3"].is_null() ? 0 : row["qty3"].as<int>();
    todo["created_at"] = row["created_at"].as<std::string>();
    return todo;
}

// POST用 JSON パースヘルパー (型違いに寛容)
static std::string getStringOr(const crow::json::rvalue& body, const char* key, const std::string& def) {
    if (!body.has(key)) return def;
    try { return body[key].s(); } catch (...) {}
    try { return std::to_string(body[key].i()); } catch (...) {}
    try { return body[key].b() ? "true" : "false"; } catch (...) {}
    return def;
}

static bool getBoolOr(const crow::json::rvalue& body, const char* key, bool def) {
    if (!body.has(key)) return def;
    try { return body[key].b(); } catch (...) {}
    try { return body[key].i() != 0; } catch (...) {}
    try {
        std::string s = body[key].s();
        if (s == "true" || s == "True" || s == "TRUE" || s == "1") return true;
        if (s == "false" || s == "False" || s == "FALSE" || s == "0") return false;
    } catch (...) {}
    return def;
}

static int getIntOr(const crow::json::rvalue& body, const char* key, int def) {
    if (!body.has(key)) return def;
    try { return body[key].i(); } catch (...) {}
    try { return static_cast<int>(body[key].d()); } catch (...) {}
    try { return body[key].b() ? 1 : 0; } catch (...) {}
    try { return std::stoi(body[key].s()); } catch (...) {}
    return def;
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
                "SELECT id, title, content, is_public, food_orange, food_apple, "
                "food_banana, pub_date, qty1, qty2, qty3, created_at "
                "FROM todos ORDER BY id DESC"
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
            std::string content = getStringOr(body, "content", "");
            bool is_public = getBoolOr(body, "is_public", false);
            bool food_orange = getBoolOr(body, "food_orange", false);
            bool food_apple = getBoolOr(body, "food_apple", false);
            bool food_banana = getBoolOr(body, "food_banana", false);
            std::string pub_date = getStringOr(body, "pub_date", "");
            int qty1 = getIntOr(body, "qty1", 0);
            int qty2 = getIntOr(body, "qty2", 0);
            int qty3 = getIntOr(body, "qty3", 0);

            pqxx::result r = txn.exec_params(
                "INSERT INTO todos (title, content, is_public, food_orange, food_apple, "
                "food_banana, pub_date, qty1, qty2, qty3) "
                "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) "
                "RETURNING id, title, content, is_public, food_orange, food_apple, "
                "food_banana, pub_date, qty1, qty2, qty3, created_at",
                title, content, is_public,
                food_orange, food_apple, food_banana,
                pub_date, qty1, qty2, qty3
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
