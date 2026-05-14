#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <functional>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class ProxyServer {
private:
    std::atomic<bool> isRunning{false};
    std::function<void(const std::wstring&)> logCallback;
    std::thread serverThread;

    // Функція для безпечного відправлення логів в інтерфейс
    void log(const std::wstring& msg) {
        if (logCallback) logCallback(msg);
    }

    void handle_client(tcp::socket client_socket) {
        try {
            beast::flat_buffer buffer;
            http::request<http::string_body> req;

            // Читаємо запит
            http::read(client_socket, buffer, req);

            // Перетворюємо шлях запиту в wstring для логів
            std::wstring target(req.target().begin(), req.target().end());
            log(L"[Proxy] Перехоплено запит до: " + target);

            // ІН'ЄКЦІЯ: Вставляємо ваш токен
            req.set(http::field::authorization, "Bearer HACKED_TOKEN_9999");
            req.set(http::field::host, "httpbin.org");

            // Підключаємося до реального сервера
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream target_stream(ioc);
            auto const results = resolver.resolve("httpbin.org", "80");
            target_stream.connect(results);

            // Відправляємо і отримуємо відповідь
            http::write(target_stream, req);
            beast::flat_buffer target_buffer;
            http::response<http::dynamic_body> res;
            http::read(target_stream, target_buffer, res);

            log(L"[Server] Отримано відповідь: " + std::to_wstring(res.result_int()));

            // Віддаємо відповідь клієнту
            http::write(client_socket, res);
            beast::error_code ec;
            target_stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        } catch (std::exception const& e) {
            std::string err = e.what();
            log(L"[Помилка] " + std::wstring(err.begin(), err.end()));
        }
    }

public:
    void Start(std::function<void(const std::wstring&)> logger) {
        if (isRunning) {
            logger(L"[Система] Проксі-сервер ВЖЕ працює!");
            return;
        }

        logCallback = logger;
        isRunning = true;
        log(L"[Система] Запуск проксі-сервера на порту 8080...");

        // ЗАПУСК У ФОНОВОМУ ПОТОЦІ
        serverThread = std::thread([this]() {
            try {
                net::io_context ioc{1};
                tcp::acceptor acceptor{ioc, {net::ip::make_address("127.0.0.1"), 8080}};
                this->log(L"[Система] Успіх! Проксі слухає http://127.0.0.1:8080");

                // Нескінченний цикл прийому з'єднань
                while (isRunning) {
                    tcp::socket socket{ioc};
                    acceptor.accept(socket);

                    // Обробляємо кожен запит в окремому потоці
                    std::thread(&ProxyServer::handle_client, this, std::move(socket)).detach();
                }
            } catch (std::exception const& e) {
                std::string err = e.what();
                this->log(L"[Критична помилка] " + std::wstring(err.begin(), err.end()));
                isRunning = false;
            }
        });

        // Відкріплюємо потік, щоб інтерфейс не чекав на його завершення
        serverThread.detach();
    }
};

#endif // PROXY_SERVER_H