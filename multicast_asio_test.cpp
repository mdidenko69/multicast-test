#include <array>
#include <iostream>
#include <string>
#include <boost/asio.hpp>

class receiver
{
public:
  receiver(boost::asio::io_context& io_context,
      const boost::asio::ip::address& multicast_address,
      short port)
    : socket_(io_context)
  {
    auto any_addr = boost::asio::ip::make_address("0.0.0.0");
    // Create the socket so that multiple may be bound to the same address.
    boost::asio::ip::udp::endpoint listen_endpoint(any_addr, port);
    socket_.open(listen_endpoint.protocol());
    socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
    socket_.bind(listen_endpoint);

    // Join the multicast group.
    socket_.set_option(
        boost::asio::ip::multicast::join_group(multicast_address));

    do_receive();
  }

private:
  void do_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(data_), sender_endpoint_,
        [this](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            std::cout << sender_endpoint_.address() << " ";
            std::cout.write(data_.data(), length);
            std::cout << std::endl;

            do_receive();
          }
        });
  }

  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint sender_endpoint_;
  std::array<char, 1024> data_;
};

class sender
{
public:
  sender(boost::asio::io_context& io_context,
      const boost::asio::ip::address& multicast_address, short port)
    : endpoint_(multicast_address, port),
      socket_(io_context, endpoint_.protocol()),
      timer_(io_context),
      message_count_(0)
  {
    do_send();
  }

private:
  void do_send()
  {
    std::ostringstream os;
    os << "Message " << message_count_++;
    message_ = os.str();

    socket_.async_send_to(
        boost::asio::buffer(message_), endpoint_,
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec) { // && message_count_ < max_message_count)
            do_timeout();
          }
        });
  }

  void do_timeout()
  {
    timer_.expires_after(std::chrono::seconds(2));
    timer_.async_wait(
        [this](boost::system::error_code ec)
        {
          if (!ec)
            do_send();
        });
  }

private:
  boost::asio::ip::udp::endpoint endpoint_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::steady_timer timer_;
  int message_count_;
  std::string message_;
};


int main(int argc, char* argv[])
{
  short multicast_port = 1900;
  std::string group = "239.255.255.250";
  try {

    boost::asio::io_context io_context;
    receiver r(io_context,
        boost::asio::ip::make_address(group),
        multicast_port);
    sender s(io_context,
        boost::asio::ip::make_address(group),
        multicast_port);
    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}