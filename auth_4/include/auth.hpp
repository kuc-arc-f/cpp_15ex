#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <sodium.h>
#include <stdexcept>
#include <regex>

class Auth {
public:
    Auth() {
        // libsodiumの初期化
        if (sodium_init() < 0) {
            throw std::runtime_error("libsodium initialization failed");
        }
    }
    ~Auth() = default;
    
    // パスワードハッシュ化（libsodium使用）
    //static std::string hashPassword(const std::string& password);
    std::string hashPassword(const std::string& password) {
        char hash[crypto_pwhash_STRBYTES];
        
        // パスワードをハッシュ化
        if (crypto_pwhash_str(
                hash,
                password.c_str(),
                password.length(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE
            ) != 0) {
            throw std::runtime_error("Password hashing failed");
        }
        
        return std::string(hash);
    }    
    bool verifyPassword(const std::string& password, const std::string& hash) {
        // パスワード検証
        return crypto_pwhash_str_verify(hash.c_str(), password.c_str(), password.length()) == 0;
    }

    // 入力バリデーション
    //static bool validateEmail(const std::string& email);
    bool validateEmail(const std::string& email) {
        // 簡易的なメールアドレスバリデーション
        const std::regex email_pattern(
            R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
        );
        
        if (email.length() > 255 || email.length() < 5) {
            return false;
        }
        
        return std::regex_match(email, email_pattern);
    }

    bool validatePassword(const std::string& password) {
        // パスワード要件: 8文字以上、大文字・小文字・数字を含む
        if (password.length() < 4 || password.length() > 100) {
            return false;
        }
        
        bool has_upper = true;
        bool has_lower = true;
        bool has_digit = true;
        /*
        for (char c : password) {
            if (std::isupper(c)) has_upper = true;
            if (std::islower(c)) has_lower = true;
            if (std::isdigit(c)) has_digit = true;
        }
        */
        
        return has_upper && has_lower && has_digit;
    }  

private:
    static constexpr size_t HASH_LENGTH = crypto_pwhash_STRBYTES;
};