#include "crow.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <mysql/mysql.h>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

// JSON用エイリアス
using json = nlohmann::json;

struct TodoItem {
    int id;
    std::string title;
    std::string created_at;
};
// これ一行で、struct <=> json の変換可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TodoItem, id, title, created_at)

// ---------------------------------------------------------
// DB接続情報
// 環境変数から取得（未設定の場合はデフォルト値を使用）
//   TODO_DB_HOST, TODO_DB_PORT, TODO_DB_USER, TODO_DB_PASS, TODO_DB_NAME
// ---------------------------------------------------------
struct DbConfig {
    std::string host;
    unsigned int port;
    std::string user;
    std::string pass;
    std::string name;
};

static std::string getEnvOr(const char *key, const std::string &def) {
    const char *v = std::getenv(key);
    return (v != nullptr) ? std::string(v) : def;
}

static DbConfig loadConfig() {
    DbConfig cfg;
    cfg.host = getEnvOr("TODO_DB_HOST", "127.0.0.1");
    cfg.port = static_cast<unsigned int>(std::stoi(getEnvOr("TODO_DB_PORT", "3306")));
    cfg.user = getEnvOr("TODO_DB_USER", "root");
    cfg.pass = getEnvOr("TODO_DB_PASS", "");
    cfg.name = getEnvOr("TODO_DB_NAME", "todo_db");
    return cfg;
}

// ---------------------------------------------------------
// MySQL接続ラッパー
// ---------------------------------------------------------
class MysqlConnection {
private:
    MYSQL *conn_;
    std::mutex mtx;

public:
    MysqlConnection() : conn_(mysql_init(nullptr)) {}

    ~MysqlConnection() {
        if (conn_ != nullptr) {
            mysql_close(conn_);
        }
    }

    bool connect(const DbConfig &cfg) {
        if (conn_ == nullptr) {
            std::cerr << "mysql_init に失敗しました" << std::endl;
            return false;
        }
        MYSQL *ret = mysql_real_connect(
            conn_,
            cfg.host.c_str(),
            cfg.user.c_str(),
            cfg.pass.c_str(),
            cfg.name.c_str(),
            cfg.port,
            nullptr,
            0);
        if (ret == nullptr) {
            std::cerr << "DB接続エラー: " << mysql_error(conn_) << std::endl;
            return false;
        }
        // 文字コードをUTF-8に設定
        mysql_set_character_set(conn_, "utf8mb4");
        return true;
    }

    MYSQL *raw() { return conn_; }

    // ---------------------------------------------------------
    // エスケープ処理付きでクエリを実行するヘルパー
    // ---------------------------------------------------------
    static std::string escapeString(MYSQL *conn, const std::string &src) {
        std::vector<char> buf(src.size() * 2 + 1);
        unsigned long len = mysql_real_escape_string(conn, buf.data(), src.c_str(), src.size());
        return std::string(buf.data(), len);
    }

    // ---------------------------------------------------------
    // 機能: 登録 (add)
    // ---------------------------------------------------------
    bool addTodo(MYSQL *conn, const std::string &title) {
        std::lock_guard<std::mutex> lock(mtx);
        if (title.empty()) {
            std::cerr << "エラー: タイトルが空です" << std::endl;
            return false;
        }
        std::string escaped = escapeString(conn, title);
        std::string query = "INSERT INTO todos (title) VALUES ('" + escaped + "')";

        if (mysql_query(conn, query.c_str()) != 0) {
            std::cerr << "登録エラー: " << mysql_error(conn) << std::endl;
            return false;
        }

        unsigned long long newId = mysql_insert_id(conn);
        //std::cout << "登録しました (id=" << newId << "): " << title << std::endl;
        return true;
    }

    std::string listJson(MYSQL *conn) {
        std::string ret = "";
        const char *query = "SELECT id, title, created_at FROM todos ORDER BY id ASC";
        if (mysql_query(conn, query) != 0) {
            std::cerr << "一覧取得エラー: " << mysql_error(conn) << std::endl;
            return ret;
        }

        MYSQL_RES *result = mysql_store_result(conn);
        if (result == nullptr) {
            std::cerr << "結果取得エラー: " << mysql_error(conn) << std::endl;
            return ret;
        }

        unsigned long long rowCount = mysql_num_rows(result);
        if (rowCount == 0) {
            std::cout << "TODOはまだ登録されていません。" << std::endl;
            mysql_free_result(result);
            return ret;
        }

        std::vector<TodoItem> vec;
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)) != nullptr) {
            std::string id = row[0] ? row[0] : "";
            std::string title = row[1] ? row[1] : "";
            std::string createdAt = row[2] ? row[2] : "";
            TodoItem item;
            item.id = std::stoi(id);
            item.title = title;
            item.created_at = createdAt;
            vec.push_back(item);        
        }
        mysql_free_result(result);
        json j1 = vec;
        std::string json_str = j1.dump();        
        ret = json_str;
        return ret;
    }
    // ---------------------------------------------------------
    // 機能: 一覧 (list)
    // ---------------------------------------------------------
    bool listTodos(MYSQL *conn) {
        const char *query = "SELECT id, title, created_at FROM todos ORDER BY id ASC";
        if (mysql_query(conn, query) != 0) {
            std::cerr << "一覧取得エラー: " << mysql_error(conn) << std::endl;
            return false;
        }

        MYSQL_RES *result = mysql_store_result(conn);
        if (result == nullptr) {
            std::cerr << "結果取得エラー: " << mysql_error(conn) << std::endl;
            return false;
        }

        unsigned long long rowCount = mysql_num_rows(result);
        if (rowCount == 0) {
            std::cout << "TODOはまだ登録されていません。" << std::endl;
            mysql_free_result(result);
            return true;
        }

        std::cout << "----------------------------------------------------" << std::endl;
        std::cout << "ID\tタイトル\t\t作成日時" << std::endl;
        std::cout << "----------------------------------------------------" << std::endl;

        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result)) != nullptr) {
            std::string id = row[0] ? row[0] : "";
            std::string title = row[1] ? row[1] : "";
            std::string createdAt = row[2] ? row[2] : "";
            std::cout << id << "\t" << title << "\t\t" << createdAt << std::endl;
        }
        std::cout << "----------------------------------------------------" << std::endl;

        mysql_free_result(result);
        return true;
    }    

    // ---------------------------------------------------------
    // 機能: 削除 (delete)
    // ---------------------------------------------------------
    bool deleteTodo(MYSQL *conn, unsigned long long id) {
        std::string query = "DELETE FROM todos WHERE id = " + std::to_string(id);
        if (mysql_query(conn, query.c_str()) != 0) {
            std::cerr << "削除エラー: " << mysql_error(conn) << std::endl;
            return false;
        }

        my_ulonglong affected = mysql_affected_rows(conn);
        if (affected == 0) {
            std::cout << "id=" << id << " のTODOは見つかりませんでした。" << std::endl;
        } else {
            std::cout << "削除しました (id=" << id << ")" << std::endl;
        }
        return true;
    }    

};

int main() {
    crow::SimpleApp app;

    DbConfig cfg = loadConfig();
    MysqlConnection todoDb;
    if (!todoDb.connect(cfg)) {
        return -1;
    }

    // ヘルスチェック用
    CROW_ROUTE(app, "/health")
    ([]() {
        return crow::response(200, "OK");
    });

    CROW_ROUTE(app, "/todos").methods("GET"_method)
    ([&todoDb]() {
        try {
            std::string json1 = todoDb.listJson(todoDb.raw());

            crow::json::wvalue result;
            result["todos"] = json1;
            return crow::response(200, result);
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });

    // ---------- 登録 ----------
    CROW_ROUTE(app, "/todos").methods("POST"_method)
    ([&todoDb](const crow::request& req) {
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
        bool ok = false;

        try {
            ok = todoDb.addTodo(todoDb.raw(), title);
            crow::json::wvalue todo;
            todo["title"] = "";

            return crow::response(201, todo);
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });

    CROW_ROUTE(app, "/todos/<int>").methods("DELETE"_method)
    ([&todoDb](int id) {
        try {
            bool ok = todoDb.deleteTodo(todoDb.raw(), id);
            crow::json::wvalue result;
            result["message"] = "deleted";
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
