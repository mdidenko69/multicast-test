#include <iostream>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/process/async_system.hpp>
#include <string>

int main() {
    std::cout << "starting" << std::endl;
    boost::asio::io_context ioService;
    boost::asio::spawn(ioService, [&ioService](boost::asio::yield_context yc) {
        try {
            std::string cmd = "echo hello!1";
            boost::process::async_system(ioService, yc, cmd);
            boost::process::async_system(ioService, yc, "echo hello!2");
        } catch (const std::exception& e) {
            std::cout << "error: " << e.what() << std::endl;
        }
    });
    ioService.run();
    std::cout << "exiting" << std::endl;
    return 0;
}
