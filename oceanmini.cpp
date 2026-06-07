/*
OceanMini Crow version
by Hiroaki Inomata
*/

#include "crow.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <csignal>
#include <cstdlib>
#include <unordered_map>

namespace fs = std::filesystem;

static volatile std::sig_atomic_t stop_requested = 0;
static std::string url_decode(const std::string& value);
static std::unordered_map<std::string, std::string> parse_form_body(const std::string& body);


void handle_signal(int) {
    stop_requested = 1;
}

class OceanMiniApp {
private:
    crow::SimpleApp app;
    std::thread server_thread;

    static fs::path htdocs_root() {
        if (fs::exists("htdocs")) {
            return fs::weakly_canonical("htdocs");
        }
        if (fs::exists("../htdocs")) {
            return fs::weakly_canonical("../htdocs");
        }

        return fs::absolute("htdocs");
    }

    static std::string content_type(const fs::path& path) {
        const std::string ext = path.extension().string();

        if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
        if (ext == ".css") return "text/css; charset=utf-8";
        if (ext == ".js") return "application/javascript; charset=utf-8";
        if (ext == ".json") return "application/json; charset=utf-8";
        if (ext == ".png") return "image/png";
        if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
        if (ext == ".gif") return "image/gif";
        if (ext == ".svg") return "image/svg+xml";
        if (ext == ".ico") return "image/x-icon";
        if (ext == ".wasm") return "application/wasm";
        if (ext == ".txt") return "text/plain; charset=utf-8";

        return "application/octet-stream";
    }

    static crow::response serve_static_file(std::string request_path) {
        const fs::path root = htdocs_root();

        if (request_path.empty() || request_path == "/") {
            request_path = "index.html";
        }

        while (!request_path.empty() && request_path.front() == '/') {
            request_path.erase(request_path.begin());
        }

        fs::path target_path = fs::weakly_canonical(root / request_path);

        // htdocs 外へのアクセスを防ぐ
        const fs::path relative = fs::relative(target_path, root);
        if (relative.empty() || relative.string().rfind("..", 0) == 0) {
            return crow::response(403, "Forbidden");
        }

        if (!fs::exists(target_path) || fs::is_directory(target_path)) {
            return crow::response(404, "Not Found");
        }

        std::ifstream file(target_path, std::ios::binary);
        if (!file) {
            return crow::response(500, "File open error");
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();

        crow::response res;
        res.code = 200;
        res.body = buffer.str();
        res.set_header("Content-Type", content_type(target_path));
        return res;
    }

public:
    void start_embedded_server() {
        // / で htdocs/index.html を表示
        CROW_ROUTE(app, "/")([](){
            return serve_static_file("index.html");
        });

        // Login
        CROW_ROUTE(app, "/Login")
        .methods(crow::HTTPMethod::POST)
        ([](const crow::request& req){
            auto params = parse_form_body(req.body);

            const std::string facilityid = params["facilityid"];
            const std::string userid     = params["userid"];
            const std::string password   = params["password"];

            const bool login_ok =
                facilityid == "ocean" &&
                userid == "ocean" &&
                password == "ocean";

            crow::response res;
            res.code = 303;

            if (login_ok) {
                res.set_header("Location", "/Welcome");
            } else {
                res.set_header("Location", "/?error=1");
            }

            return res;
        });

        // Welcome ページ
        CROW_ROUTE(app, "/Welcome")([](){
            crow::response res;
            res.code = 200;
            res.set_header("Content-Type", "text/html; charset=utf-8");
            res.body = R"(
<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <title>Welcome</title>
</head>
<body>
    <h1>Welcome to OceanMini</h1>
    <p>ログインしました。</p>
</body>
</html>
)";
            return res;
        });

        // htdocs 配下の静的ファイルをそのまま配信
        CROW_ROUTE(app, "/<path>")([](const std::string& path){
            return serve_static_file(path);
        });

        // 💡重要: app.run() は無限ループになってスレッドを占有するため、
        // std::thread を使ってバックグラウンド（裏側）で動かします。
        server_thread = std::thread([this]() {
            app.signal_clear();
            app.port(18080).bindaddr("0.0.0.0").multithreaded().run();
        });
        
        std::cout << "組み込みサーバーがポート 18080 で起動しました。" << std::endl;
    }

    void stop_embedded_server() {
        app.stop(); // サーバーを停止
        if (server_thread.joinable()) {
            server_thread.join(); // スレッドの終了を待つ
        }
    }

    ~OceanMiniApp() {
        stop_embedded_server();
    }
};

static void open_default_browser(const std::string& url) {
#if defined(__APPLE__)
    std::string command = "open \"" + url + "\"";
#elif defined(_WIN32)
    std::string command = "start \"\" \"" + url + "\"";
#elif defined(__linux__)
    std::string command = "xdg-open \"" + url + "\"";
#else
    std::cout << "ブラウザを自動起動できない環境です: " << url << std::endl;
    return;
#endif

    std::system(command.c_str());
}

static std::string url_decode(const std::string& value) {
    std::string result;

    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            result += ' ';
        } else if (value[i] == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            char ch = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result += ch;
            i += 2;
        } else {
            result += value[i];
        }
    }

    return result;
}

static std::unordered_map<std::string, std::string> parse_form_body(const std::string& body) {
    std::unordered_map<std::string, std::string> params;

    std::size_t start = 0;
    while (start < body.size()) {
        std::size_t end = body.find('&', start);
        if (end == std::string::npos) {
            end = body.size();
        }

        std::string pair = body.substr(start, end - start);
        std::size_t eq = pair.find('=');

        if (eq != std::string::npos) {
            std::string key = url_decode(pair.substr(0, eq));
            std::string value = url_decode(pair.substr(eq + 1));
            params[key] = value;
        }

        start = end + 1;
    }

    return params;
}

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    OceanMiniApp oceanmini;
    oceanmini.start_embedded_server();

    open_default_browser("http://localhost:18080/");

    std::cout << "メイン処理が走っています... Ctrl+C で終了します。" << std::endl;

    while (!stop_requested) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "終了処理を開始します..." << std::endl;
    oceanmini.stop_embedded_server();

    return 0;
}