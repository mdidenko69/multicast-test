#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>

#include <array>
#include <string>
#include <string_view>
#include <thread>
#include <sstream>
#include <iostream>

#define MSGBUFSIZE 256

template<typename T> 
auto myPrimaryIp() -> T;

template<>
auto myPrimaryIp<in_addr>() -> in_addr {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  assert(sock != -1);

  const char* kGoogleDnsIp = "8.8.8.8";
  uint16_t kDnsPort = 53;
  struct sockaddr_in serv;
  memset(&serv, 0, sizeof(serv));
  serv.sin_family = AF_INET;
  serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
  serv.sin_port = htons(kDnsPort);

  int err = connect(sock, (const sockaddr*) &serv, sizeof(serv));
  assert(err != -1);

  sockaddr_in name;
  socklen_t namelen = sizeof(name);
  err = getsockname(sock, (sockaddr*) &name, &namelen);
  assert(err != -1);
  close(sock);

  return name.sin_addr;
}

template<>
auto myPrimaryIp<std::string>() -> std::string {
  in_addr a = myPrimaryIp<in_addr>();
  std::array<char, 20> buffer;

  const char* p = inet_ntop(AF_INET, &a, buffer.data(), buffer.size());
  assert(p);

  return p;
}

void receive(std::string_view group, int port) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(1);
  }

  // allow multiple sockets to use the same PORT number
  //
  u_int yes = 1;
  if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0 ){
    perror("Reusing ADDR failed");
    exit(1);
  }
  // set up destination address
  struct sockaddr_in addr, peer_addr;
  memset(&addr, 0, sizeof(addr));
  memset(&peer_addr, 0, sizeof(peer_addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
  addr.sin_port = htons(port);

  // bind to receive address
  if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);    
  }

  // use setsockopt() to request that the kernel join a multicast group
  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(group.data());
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  //mreq.imr_interface.s_addr = inet_addr("192.168.1.12");

  if ( setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq) ) < 0 ){
    perror("setsockopt");
    exit(1);    
  }

  auto my_ip_s = myPrimaryIp<std::string>();
  auto my_ip = myPrimaryIp<in_addr>();

  // now just enter a read-print loop
  std::array<char,20> sender_ip;
  std::array<char,MSGBUFSIZE> msgbuf;
  while (1) {
    socklen_t addrlen = sizeof(peer_addr);
    int nbytes = recvfrom(
        fd,
        msgbuf.data(),
        msgbuf.size(),
        0,
        (struct sockaddr *) &peer_addr,
        &addrlen
    );
    if (nbytes < 0) {
        perror("recvfrom");
        exit(1);    
    }
    msgbuf[nbytes] = '\0';
    // std::cout << ntohl(my_ip.s_addr) << "  " << ntohl(peer_addr.sin_addr.s_addr) << "\n";
    if ( ntohl(my_ip.s_addr) == ntohl(peer_addr.sin_addr.s_addr) ) {
      continue;
    }
    const char* p = inet_ntop(AF_INET, &peer_addr.sin_addr, sender_ip.data(), sender_ip.size());
    assert(p);

    printf("my ip: %s, got from: %s: %s\n", my_ip_s.c_str(), sender_ip.data(), msgbuf.data());
  }
}

void send(std::string_view group, int port) {
  const int delay_secs = 2;
  const char *message = "Hello, World! ";


  std::cout << "My IP: " <<  myPrimaryIp<std::string>() << "\n";

  // create what looks like an ordinary UDP socket
  //
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(2);
  }

  // set up destination address
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(group.data());
  addr.sin_port = htons(port);

  int count = 0;
  while (1) {
    std::string msg = message + std::to_string(++count);
    int nbytes = sendto(
        fd,
        msg.data(),
        msg.size(),
        0,
        (struct sockaddr*) &addr,
        sizeof(addr)
    );
    if (nbytes < 0) {
      perror("sendto");
      exit(2);
    }
    sleep(delay_secs); // Unix sleep is seconds
  }

}

int main(int argc, char *argv[]) {

  std::string group = "239.255.255.250"; // e.g. 239.255.255.250 for SSDP
  int port = 1900; // 0 if error, which is an invalid port

  std::thread th([&]() {
    receive(group, port);
  });

  send(group, port);
}