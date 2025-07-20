#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <future>

#include <gtest/gtest.h>
#include <fmt/format.h>

#include <nghttp2/asio_http2_server.h>
#include <nghttp2/asio_http2_client.h>

namespace net = boost::asio;
using boost::asio::ip::tcp;
using namespace nghttp2::asio_http2;
using namespace std::chrono_literals;

// https://stackoverflow.com/questions/74745728/how-do-i-return-a-boostasiodeferred-from-a-virtual-function

net::awaitable<void> test()
{
  
}

TEST(Server, Coroutine)
{
  boost::asio::io_context ctx;

  co_spawn(ctx, test(), net::detached);
}

TEST(Server, StartStop)
{
  for (size_t i = 0; i < 10'000; ++i)
  {
    boost::system::error_code ec;
    server::http2 server;

    server.handle("/", [](const server::request &req, const server::response &res) {
      res.write_head(200);
      res.end("hello, world\n");
    });

    server.num_threads(1);
    EXPECT_FALSE(server.listen_and_serve(ec, "127.0.0.1", "0", /* asynchronous */ true)) << ec.message();
    EXPECT_EQ(server.ports().size(), 1);

#if 0
    server.stop();
#else
    boost::asio::post(*server.io_services()[0], [&](){
      server.stop();
    });
#endif
    server.join();
  }
}

TEST(Server, Hello)
{
  boost::system::error_code ec;
  server::http2 server;

  server.handle("/", [](const server::request &req, const server::response &res) {
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
  boost::asio::io_context& io_service = *server.io_services()[0];
  client::session sess(io_service, "127.0.0.1", std::to_string(port));

  std::promise<void> promise;
  sess.on_connect([&](tcp::endpoint endpoint) {
    boost::system::error_code ec;

    auto req = sess.submit(ec, "GET", fmt::format("http://127.0.0.1:{}/", port));

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
      sess.shutdown();
      promise.set_value();
    });
  });

  sess.on_error([&](const boost::system::error_code &ec) {
    std::cerr << "error: " << ec.message() << std::endl;
  });

  promise.get_future().wait_for(1s);

  boost::asio::post(io_service, [&]{
    server.stop();
  });

  server.join();
}
