#include "app.h"
#include <restinio/all.hpp>

using namespace restinio;

int main(int argc, char *argv[]) {
    // restinio::run(
    //     restinio::on_this_thread()
    //     .port(8080)
    //     .address("localhost")
    //     .request_handler([](auto req) {
    //         return req->create_response().set_body("Hello, World!").done();
    //     }));

  App app;
  return app.exec(argc, argv);
}
