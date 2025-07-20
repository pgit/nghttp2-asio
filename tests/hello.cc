#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <future>

#include <gtest/gtest.h>
#include <fmt/format.h>

#include <nghttp2/asio_http2_server.h>
#include <nghttp2/asio_http2_client.h>
#include <thread>

using boost::asio::ip::tcp;
using namespace nghttp2::asio_http2;
using namespace std::chrono_literals;

TEST(Server, StartStop) {
    boost::system::error_code ec;
    server::http2 server;

    server.handle("/",
                  [](const server::request &req, const server::response &res) {
                    res.write_head(200);
                    res.end("hello, world\n");
                  });

    server.num_threads(1);
    EXPECT_FALSE(server.listen_and_serve(ec, "127.0.0.1", "3000",
                                         /* asynchronous */ true))
        << ec.message();
    EXPECT_EQ(server.ports().size(), 1);

    boost::asio::post(*server.io_services()[0], [&]() { server.stop(); });
    server.join();
}

TEST(Server, Hello) {
  boost::system::error_code ec;
  server::http2 server;

  server.handle("/",
                [](const server::request &req, const server::response &res) {
                  res.write_head(200);
                  res.end("hello, world\n");
                });

  if (server.listen_and_serve(ec, "127.0.0.1", "0", /* asynchronous */ true)) {
    std::cerr << "error: " << ec.message() << std::endl;
  }

  ASSERT_EQ(server.ports().size(), 1);
  auto port = server.ports()[0];
  std::cout << "listening on port: " << port << std::endl;

  // connect to localhost:3000
  boost::asio::io_context io_service;
  auto sess = std::make_shared<client::session>(io_service, "127.0.0.1",
                                                std::to_string(port));

  constexpr size_t COUNT = 1000;
  std::atomic<size_t> count = COUNT;
  sess->on_connect([&](tcp::endpoint endpoint) {
    boost::system::error_code ec;

    for (size_t i = 0; i < COUNT; ++i) {
      auto req =
          sess->submit(ec, "GET", fmt::format("http://127.0.0.1:{}/", port));

      req->on_response([](const client::response &res) {
        // print status code and response header fields.
        std::cerr << "HTTP/2 " << res.status_code() << std::endl;
        for (auto &kv : res.header()) {
          std::cerr << kv.first << ": " << kv.second.value << "\n";
        }
        std::cerr << std::endl;

        res.on_data([](const uint8_t *data, std::size_t len) {
          std::cerr.write(reinterpret_cast<const char *>(data), len);
          std::cerr << std::endl;
        });
      });

      req->on_close([&](uint32_t error_code) {
        // shutdown session after first request was done.
        if (--count == 0)
        {
            sess->shutdown();
            sess.reset();
        }
        std::cout << count << std::endl;
      });
    }
  });

  sess->on_error([&](const boost::system::error_code &ec) {
    std::cerr << "error: " << ec.message() << std::endl;
  });

  io_service.run();

  boost::asio::post(server.io_services()[0]->get_executor(),
                    [&]() { server.stop(); });

  server.join();
}
