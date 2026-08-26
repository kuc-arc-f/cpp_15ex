#include <crow.h>
#include <sqlite3.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "crow/middlewares/cors.h"

#include "./include/my_ssr.hpp"
#include "./include/my_type.hpp"

class TodoDatabase {
private:
    sqlite3* db;
    
public:
    TodoDatabase(const std::string& dbPath = "todos.db") {
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db)));
        }
        createTable();
    }
    
    ~TodoDatabase() {
        if (db) {
            sqlite3_close(db);
        }
    }
    
    void createTable() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS todos (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                title TEXT NOT NULL,
                description TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                completed BOOLEAN DEFAULT 0
            );
        )";
        
        char* errMsg = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::string error = errMsg;
            sqlite3_free(errMsg);
            throw std::runtime_error("Failed to create table: " + error);
        }
    }
    
    // 登録（INSERT）
    bool insertTodo(const std::string& title, const std::string& description = "") {
        std::string sql = "INSERT INTO todos (title, description) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);
        
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }
    
    // 一覧取得（SELECT）
    std::vector<std::string> getAllTodos() {
        std::vector<std::string> todos;
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return todos;
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            
            std::stringstream ss;
            ss << "ID: " << id 
               << ", Title: " << (title ? reinterpret_cast<const char*>(title) : "")
               << ", Description: " << (description ? reinterpret_cast<const char*>(description) : "")
               << ", Created: " << (created_at ? reinterpret_cast<const char*>(created_at) : "")
               << ", Completed: " << (completed ? "Yes" : "No");
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
    
    // JSON形式で一覧取得（API用）
    std::string getTodosAsJSON() {
        std::stringstream json;
        json << "[";
        
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return "[]";
        }
        
        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) json << ",";
            first = false;
            
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            
            json << "{"
                 << "\"id\":" << id << ","
                 << "\"title\":\"" << (title ? reinterpret_cast<const char*>(title) : "") << "\","
                 << "\"description\":\"" << (description ? reinterpret_cast<const char*>(description) : "") << "\","
                 << "\"created_at\":\"" << (created_at ? reinterpret_cast<const char*>(created_at) : "") << "\","
                 << "\"completed\":" << (completed ? "true" : "false")
                 << "}";
        }
        
        sqlite3_finalize(stmt);
        json << "]";
        return json.str();
    }

    std::vector<Todo> getTodosList() {
        std::stringstream json;
        std::vector<Todo> ret;
        json << "[";
        
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return ret;
        }
        
        bool first = true;
        std::vector<Todo> rows;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            std::string title_str = "";
            if(title){ title_str = reinterpret_cast<const char*>(title); }
            else{ title_str= ""; }
            Todo item;
            item.id = id;
            item.title = title_str;

            rows.push_back(item);
        }
        
        sqlite3_finalize(stmt);
        return rows;
    }    
};

int main() {
    crow::SimpleApp app;
    TodoDatabase todoDb;

    CROW_ROUTE(app, "/").methods(crow::HTTPMethod::GET)([]() 
    {
        MySsr ss("");
        std::string htm = ss.ssr_htm_top();
        return crow::response(htm);
    });
    
    // API: 一覧取得 (GET)
    CROW_ROUTE(app, "/api/todos/list")
    .methods(crow::HTTPMethod::GET)([&todoDb]() {
        auto todos = todoDb.getTodosList();
        MySsr sLib("");
        std::string htm = sLib.renderTodoList(todos);
        return crow::response(htm);
    });
    
    // API: 登録 (POST)
    CROW_ROUTE(app, "/api/todos/create")
    .methods(crow::HTTPMethod::POST)([&todoDb](const crow::request& req) {
        auto body = req.get_body_params();
        std::string title_str = body.get("title") ? body.get("title") : "ゲスト";
        if (title_str.empty()) {
            return crow::response(400, "Title is required");
        }
        bool success = todoDb.insertTodo(title_str, "");        
        auto todos = todoDb.getTodosList();
        MySsr sLib("");
        std::string htm = sLib.renderTodoList(todos);
        return crow::response(htm);
    });
    CROW_ROUTE(app, "/api/todos/delete")
    .methods(crow::HTTPMethod::POST)([&todoDb](const crow::request& req) {
        auto body = req.get_body_params();
        std::string id_str = body.get("id") ? body.get("id") : "";
        if (id_str.empty()) {
            return crow::response(400, "Title is required");
        }
        bool success = todoDb.deleteTodo(std::stoi(id_str));
        auto todos = todoDb.getTodosList();
        MySsr sLib("");
        std::string htm = sLib.renderTodoList(todos);
        return crow::response(htm);
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

    //files
     CROW_ROUTE(app, "/js/todo.js")
    ([](const crow::request& req, crow::response& res){
        res.set_static_file_info("static/todo.js");
        // Content-Typeを明確に指定したい場合（通常は自動判別されます）
        res.add_header("Content-Type", "application/javascript");        
        res.end();
    }); 
    
    app.port(8080).multithreaded().run();    
    return 0;
}
