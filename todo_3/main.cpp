#include "crow.h"
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>
#include <libpq-fe.h>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

// JSON用エイリアス
using json = nlohmann::json;

std::string conn_str = "host=localhost port=5432 dbname=mydb user=root password=admin";

struct TodoItem {
    int id;
    std::string title;
    std::string created_at;
};
// これ一行で、struct <=> json の変換可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TodoItem, id, title, created_at)

// 環境変数 DATABASE_URL があればそれを使用、なければデフォルト値を使用
static std::string getConnStr() {
    const char* env = std::getenv("DATABASE_URL");
    if (env != nullptr && std::string(env).size() > 0) {
        return std::string(env);
    }
    return "postgresql://postgres:admin@localhost:5432/mydb123";
}

class TodoApp {
private:
    PGconn* conn;
    std::mutex mtx; // 共有リソースを守るためのmutex

    struct Todo {
        int id;
        std::string title;
        std::string description;
        bool completed;
        std::string created_at;
    };

public:
    TodoApp(const std::string& conn_str) {
        conn = PQconnectdb(conn_str.c_str());
        if (PQstatus(conn) != CONNECTION_OK) {
            throw std::runtime_error("Connection failed: " + std::string(PQerrorMessage(conn)));
        }
        std::cout << "Connected to PostgreSQL successfully!\n";
        initTable();
    }

    ~TodoApp() {
        PQfinish(conn);
    }

    void initTable() {
        const char* create_table = 
            "CREATE TABLE IF NOT EXISTS todos ("
            "id SERIAL PRIMARY KEY, "
            "title TEXT NOT NULL, "
            "description TEXT, "
            "completed BOOLEAN DEFAULT FALSE, "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");";
        
        PGresult* res = PQexec(conn, create_table);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            PQclear(res);
            throw std::runtime_error("Table creation failed: " + std::string(PQerrorMessage(conn)));
        }
        PQclear(res);
        std::cout << "Table initialized successfully!\n";
    }

    // 登録機能
    void addTodo(const std::string& title, const std::string& description = "") {
        std::lock_guard<std::mutex> lock(mtx);
        if (title.empty()) {
            std::cout << "Error: Title cannot be empty!\n";
            return;
        }

        std::string query = "INSERT INTO todos (title) VALUES ($1);";
        const char* params[1] = {title.c_str()};
        PGresult* res = PQexecParams(conn, query.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
        
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cout << "Error adding todo: " << PQerrorMessage(conn) << "\n";
        }
        PQclear(res);
    }

    std::string listJson(bool showCompleted = false) {
        std::string ret = "";
        std::string query = "SELECT id, title, description, completed, created_at FROM todos";
        if (!showCompleted) {
            query += " WHERE completed = false";
        }
        query += " ORDER BY created_at DESC;";
        
        PGresult* res = PQexec(conn, query.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::cout << "Error listing todos: " << PQerrorMessage(conn) << "\n";
            PQclear(res);
            return ret;
        }

        int rows = PQntuples(res);
        if (rows == 0) {
            std::cout << "No todos found.\n";
            PQclear(res);
            return ret;
        }

        std::vector<TodoItem> vec;
        for (int i = 0; i < rows; i++) {
            TodoItem row;
            row.id = atoi(PQgetvalue(res, i, 0));
            row.title = PQgetvalue(res, i, 1);
            row.created_at = PQgetvalue(res, i, 4);
            vec.push_back(row);        
        }
        PQclear(res);
        json j1 = vec;
        std::string json_str = j1.dump();        
        ret = json_str;
        return ret;
    }

    // 削除機能
    void deleteTodo(int id) {
        if (id <= 0) {
            std::cout << "Error: Invalid ID!\n";
            return;
        }

        std::string query = "DELETE FROM todos WHERE id = $1;";
        const char* params[1] = {std::to_string(id).c_str()};
        PGresult* res = PQexecParams(conn, query.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
        
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cout << "Error deleting todo: " << PQerrorMessage(conn) << "\n";
        } else {
            int affected = atoi(PQcmdTuples(res));
            if (affected > 0) {
                std::cout << "Todo " << id << " deleted successfully!\n";
            } else {
                std::cout << "Todo " << id << " not found!\n";
            }
        }
        PQclear(res);
    }

    // 完了マーク機能（追加機能）
    void completeTodo(int id) {
        if (id <= 0) {
            std::cout << "Error: Invalid ID!\n";
            return;
        }

        std::string query = "UPDATE todos SET completed = true WHERE id = $1;";
        const char* params[1] = {std::to_string(id).c_str()};
        PGresult* res = PQexecParams(conn, query.c_str(), 1, nullptr, params, nullptr, nullptr, 0);
        
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::cout << "Error completing todo: " << PQerrorMessage(conn) << "\n";
        } else {
            int affected = atoi(PQcmdTuples(res));
            if (affected > 0) {
                std::cout << "Todo " << id << " marked as completed!\n";
            } else {
                std::cout << "Todo " << id << " not found!\n";
            }
        }
        PQclear(res);
    }
};

int main() {
    crow::SimpleApp app;
    TodoApp todoDb(conn_str);

    // ヘルスチェック用
    CROW_ROUTE(app, "/health")
    ([]() {
        return crow::response(200, "OK");
    });

    // ---------- 一覧取得 ----------
    CROW_ROUTE(app, "/todos").methods("GET"_method)
    ([&todoDb]() {
        try {
            auto todos = todoDb.listJson(true);

            crow::json::wvalue result;
            result["todos"] = todos;
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

        try {
            todoDb.addTodo(title, "");
            crow::json::wvalue todo;
            todo["title"] = title;

            return crow::response(201, todo);
        } catch (const std::exception& e) {
            crow::json::wvalue err;
            err["error"] = std::string("DB error: ") + e.what();
            return crow::response(500, err);
        }
    });

    // ---------- 削除 ----------
    CROW_ROUTE(app, "/todos/<int>").methods("DELETE"_method)
    ([&todoDb](int id) {
        try {
            todoDb.deleteTodo(id);
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
