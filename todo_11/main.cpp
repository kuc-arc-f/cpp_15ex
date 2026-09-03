#include <crow.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

class TodoDatabase {
private:
    sqlite3* db;

public:
    TodoDatabase(const std::string& dbPath = "todos.db") {
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db)));
        }
        initTable();
    }

    ~TodoDatabase() {
        if (db) {
            sqlite3_close(db);
        }
    }

    // JSON から安全に値を取り出すヘルパー
    static std::string getString(const crow::json::rvalue& body, const char* key, const std::string& def = "") {
        if (body.has(key) && body[key].t() == crow::json::type::String) {
            return body[key].s();
        }
        return def;
    }
    static int getInt(const crow::json::rvalue& body, const char* key, int def = 0) {
        if (body.has(key) && body[key].t() == crow::json::type::Number) {
            return static_cast<int>(body[key].i());
        }
        return def;
    }

    // テーブルが存在しない場合は作成
    void initTable() {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS todos ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    title TEXT,"
            "    content TEXT,"
            "    is_public INTEGER,"
            "    food_orange INTEGER,"
            "    food_apple INTEGER,"
            "    food_banana INTEGER,"
            "    pub_date TEXT,"
            "    qty1 INTEGER,"
            "    qty2 INTEGER,"
            "    qty3 INTEGER"
            ");";
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string err = errMsg ? errMsg : "unknown error";
            sqlite3_free(errMsg);
            throw std::runtime_error("Cannot create table: " + err);
        }
    }

    // 登録（INSERT）: テーブル全項目に対応
    bool insertTodo(const std::string& title,
                    const std::string& content,
                    int is_public,
                    int food_orange,
                    int food_apple,
                    int food_banana,
                    const std::string& pub_date,
                    int qty1, int qty2, int qty3) {
        std::string sql =
            "INSERT INTO todos (title, content, is_public, food_orange, food_apple, food_banana, pub_date, qty1, qty2, qty3) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, is_public);
        sqlite3_bind_int(stmt, 4, food_orange);
        sqlite3_bind_int(stmt, 5, food_apple);
        sqlite3_bind_int(stmt, 6, food_banana);
        sqlite3_bind_text(stmt, 7, pub_date.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 8, qty1);
        sqlite3_bind_int(stmt, 9, qty2);
        sqlite3_bind_int(stmt, 10, qty3);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    // 一覧取得（SELECT）: テーブル全項目
    std::vector<std::string> getAllTodos() {
        std::vector<std::string> todos;
        const char* sql =
            "SELECT id, title, content, is_public, food_orange, food_apple, food_banana, pub_date, qty1, qty2, qty3 "
            "FROM todos ORDER BY id DESC;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return todos;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::stringstream ss;
            ss << "ID: "          << sqlite3_column_int(stmt, 0)
               << ", Title: "     << colText(stmt, 1)
               << ", Content: "   << colText(stmt, 2)
               << ", Public: "    << (sqlite3_column_int(stmt, 3) ? "Yes" : "No")
               << ", Orange: "    << sqlite3_column_int(stmt, 4)
               << ", Apple: "     << sqlite3_column_int(stmt, 5)
               << ", Banana: "    << sqlite3_column_int(stmt, 6)
               << ", PubDate: "   << colText(stmt, 7)
               << ", Qty1: "      << sqlite3_column_int(stmt, 8)
               << ", Qty2: "      << sqlite3_column_int(stmt, 9)
               << ", Qty3: "      << sqlite3_column_int(stmt, 10);
            todos.push_back(ss.str());
        }

        sqlite3_finalize(stmt);
        return todos;
    }

    // 削除（DELETE）
    bool deleteTodo(int id) {
        std::string sql = "DELETE FROM todos WHERE id = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_int(stmt, 1, id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    // JSON形式で一覧取得（API用）: テーブル全項目
    std::string getTodosAsJSON() {
        std::stringstream json;
        json << "[";

        const char* sql =
            "SELECT id, title, content, is_public, food_orange, food_apple, food_banana, pub_date, qty1, qty2, qty3 "
            "FROM todos ORDER BY id DESC;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return "[]";
        }

        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) json << ",";
            first = false;

            json << "{"
                 << "\"id\":"        << sqlite3_column_int(stmt, 0) << ","
                 << "\"title\":\""   << jsonEscape(colText(stmt, 1)) << "\","
                 << "\"content\":\"" << jsonEscape(colText(stmt, 2)) << "\","
                 << "\"is_public\":" << sqlite3_column_int(stmt, 3) << ","
                 << "\"food_orange\":" << sqlite3_column_int(stmt, 4) << ","
                 << "\"food_apple\":"  << sqlite3_column_int(stmt, 5) << ","
                 << "\"food_banana\":" << sqlite3_column_int(stmt, 6) << ","
                 << "\"pub_date\":\"" << jsonEscape(colText(stmt, 7)) << "\","
                 << "\"qty1\":"      << sqlite3_column_int(stmt, 8) << ","
                 << "\"qty2\":"      << sqlite3_column_int(stmt, 9) << ","
                 << "\"qty3\":"      << sqlite3_column_int(stmt, 10)
                 << "}";
        }

        sqlite3_finalize(stmt);
        json << "]";
        return json.str();
    }

private:
    static std::string colText(sqlite3_stmt* stmt, int col) {
        const unsigned char* text = sqlite3_column_text(stmt, col);
        return text ? reinterpret_cast<const char*>(text) : "";
    }

    // JSON 文字列用の簡易エスケープ
    static std::string jsonEscape(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
            }
        }
        return out;
    }
};

int main() {
    crow::SimpleApp app;
    TodoDatabase todoDb;

    // API: 一覧取得 (GET)
    CROW_ROUTE(app, "/api/todos")
    .methods(crow::HTTPMethod::GET)([&todoDb]() {
        return crow::response(todoDb.getTodosAsJSON());
    });

    // API: 登録 (POST) — テーブル全項目を受け付ける
    CROW_ROUTE(app, "/api/todos")
    .methods(crow::HTTPMethod::POST)([&todoDb](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        // JSON から各項目を取得（未指定ならデフォルト値）
        std::string title       = TodoDatabase::getString(body, "title");
        std::string content     = TodoDatabase::getString(body, "content");
        int         is_public   = TodoDatabase::getInt(body, "is_public", 0);      // 0=非公開, 1=公開
        int         food_orange = TodoDatabase::getInt(body, "food_orange", 0);
        int         food_apple  = TodoDatabase::getInt(body, "food_apple", 0);
        int         food_banana = TodoDatabase::getInt(body, "food_banana", 0);
        std::string pub_date    = TodoDatabase::getString(body, "pub_date");
        int         qty1        = TodoDatabase::getInt(body, "qty1", 0);
        int         qty2        = TodoDatabase::getInt(body, "qty2", 0);
        int         qty3        = TodoDatabase::getInt(body, "qty3", 0);

        if (title.empty()) {
            return crow::response(400, "Title is required");
        }

        bool success = todoDb.insertTodo(title, content, is_public,
                                         food_orange, food_apple, food_banana,
                                         pub_date, qty1, qty2, qty3);

        crow::json::wvalue result;
        result["success"] = success;
        result["message"] = success ? "TODO created successfully" : "Failed to create TODO";

        return crow::response(result);
    });

    // API: 削除 (DELETE)
    CROW_ROUTE(app, "/api/todos/<int>")
    .methods(crow::HTTPMethod::DELETE)([&todoDb](int id) {
        bool success = todoDb.deleteTodo(id);

        crow::json::wvalue result;
        result["success"] = success;
        result["message"] = success ? "TODO deleted successfully" : "Failed to delete TODO";

        return crow::response(result);
    });

    // コンソール用のシンプルインターフェース
    CROW_ROUTE(app, "/console")
    .methods(crow::HTTPMethod::GET)([&todoDb]() {
        auto todos = todoDb.getAllTodos();
        std::stringstream html;
        html << "<html><body><h1>TODO コンソール</h1><ul>";
        for (const auto& todo : todos) {
            html << "<li>" << todo << "</li>";
        }
        html << "</ul></body></html>";
        return crow::response(html.str());
    });

    app.port(8080).multithreaded().run();

    return 0;
}
