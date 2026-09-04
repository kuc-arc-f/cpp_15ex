#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <vector>
#include <thread>

#include "http_client.hpp"

// JSON用エイリアス
using json = nlohmann::json;

std::string API_BASE_URL = "http://localhost:8080";

struct QueryReq {
    std::string key;
    std::string value;
};
// これ一行で、QueryReq <=> json の変換が魔法のように可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, key, value)

std::vector<QueryReq> SendArr1;
std::vector<QueryReq> SendArr2;
std::vector<QueryReq> SendArr3;
std::vector<QueryReq> SendArr4;
std::vector<QueryReq> SendArr5;
std::vector<QueryReq> SendArr6;
std::vector<QueryReq> SendArr7;
std::vector<QueryReq> SendArr8;
std::vector<QueryReq> SendArr9;
std::vector<QueryReq> SendArr10;

// ─────────────────────────────────────────────
// ユーティリティ：レスポンス表示
// ─────────────────────────────────────────────
static void print_response(const std::string& label, const HttpResponse& resp)
{
    std::cout << "\n===== " << label << " =====\n";
    if (!resp.error.empty()) {
        std::cerr << "[ERROR] " << resp.error << "\n";
        return;
    }
    std::cout << "Status : " << resp.status_code << "\n";
    std::cout << "Body   :\n" << resp.body << "\n";
}

void make_arr(){
    std::vector<QueryReq> vec;
    for(int i = 0; i < 1000; i++) {
        int target = i + 1;
        int div_num = target % 10;
        switch(div_num) {
            case 0:
                SendArr1.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 1:
                SendArr2.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 2:
                SendArr3.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 3:
                SendArr4.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 4:
                SendArr5.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;

            case 5:
                SendArr6.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 6:
                SendArr7.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 7:
                SendArr8.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 8:
                SendArr9.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;
            case 9:
                SendArr10.push_back(QueryReq{"k:" + std::to_string(target), "hello"});
                break;

        }
    }
}

void send_arr(std::vector<QueryReq> arr){
    std::string url = API_BASE_URL + "/redis_add";
    HttpClient client(30 /*timeout*/, true /*verify_ssl*/);
    for(int i = 0; i < arr.size(); i++) {
        QueryReq req = arr[i];
        json j = req;
        std::string json_str = j.dump();
        auto resp = client.post_json(url , json_str);
        if (!resp.error.empty()) {
            std::cerr << L"[ERROR] \n";
            return;
        }
    }        
}

int main()
{
    std::cout << "#start:" << std::endl;
    try{
        make_arr();

        json j1 = SendArr1;
        json j2 = SendArr2;
        //std::string json1 = j1.dump();
        //std::string json2 = j2.dump();
        std::cout << "json1=" << SendArr1.size() << std::endl;
        std::cout << "json2=" << SendArr2.size() << std::endl;
        std::cout << "json3=" << SendArr3.size() << std::endl;
        std::cout << "json4=" << SendArr4.size() << std::endl;
        std::cout << "json5=" << SendArr5.size() << std::endl;
        std::cout << "json6=" << SendArr6.size() << std::endl;
        std::cout << "json7=" << SendArr7.size() << std::endl;
        std::cout << "json8=" << SendArr8.size() << std::endl;
        std::cout << "json9=" << SendArr9.size() << std::endl;
        std::cout << "json10=" << SendArr10.size() << std::endl;

        // 開始時刻
        auto start = std::chrono::high_resolution_clock::now();

        std::thread t1(send_arr, SendArr1); //各スレッドの定義
        std::thread t2(send_arr, SendArr2);
        std::thread t3(send_arr, SendArr3);
        std::thread t4(send_arr, SendArr4);
        std::thread t5(send_arr, SendArr5);
        std::thread t6(send_arr, SendArr6);
        std::thread t7(send_arr, SendArr7);
        std::thread t8(send_arr, SendArr8);
        std::thread t9(send_arr, SendArr9);
        std::thread t10(send_arr, SendArr10);

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();
        t6.join();
        t7.join();
        t8.join();
        t9.join();
        t10.join();
        // 終了時刻
        auto end = std::chrono::high_resolution_clock::now();
        // 差分（ミリ秒）
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "time: " << duration.count() << " ms" << std::endl;        
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return 0;
    }   
}
