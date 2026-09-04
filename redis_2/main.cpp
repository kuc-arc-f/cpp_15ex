#include <crow.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <chrono>
#include <string>
#include <thread>

#include <hiredis/hiredis.h>


int main() {
    crow::SimpleApp app;
    
    // API: 登録 (POST)
    CROW_ROUTE(app, "/redis_add")
    .methods(crow::HTTPMethod::POST)([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }
        
        std::string key = body["key"].s();
        std::string value = body["value"].s();
        
        if (key.empty()) {
            return crow::response(400, "key is required");
        }
        // Redis接続
        redisContext* c = redisConnect("127.0.0.1", 6379);

        if (c == nullptr || c->err) {
            if (c) {
                std::cerr << "Connection error: " << c->errstr << std::endl;
                redisFree(c);
            } else {
                std::cerr << "Connection error: can't allocate redis context" << std::endl;
            }
            return crow::response(500, "error , Connection error");
        } 
        // SET
        std::string set_command = "SET " + key + " " + value;
        redisReply* reply = (redisReply*)redisCommand(c, set_command.c_str());
        freeReplyObject(reply);

        // 切断
        redisFree(c);               
        
        crow::json::wvalue result;
        result["success"] = "OK";
        
        return crow::response(result);
    });
        
    app.port(8080).multithreaded().run();
    
    return 0;
}
