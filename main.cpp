#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <chrono>
using namespace std::chrono_literals;

const int kMaxBuf = 1600;

int getResponce(char *buf, int len, const sockaddr_in addr_in, int udp_sockfd,
                sockaddr &udp_addr, std::mutex &m_udp_sockfd);

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cout << "Usage: " << /*argv[0]*/"udp2tcp-cpp"
              << " local-port dest-ip dest-port" << std::endl;
    exit(0);
  }

  sockaddr_in udp_local_addr, tcp_remote_addr;
  udp_local_addr.sin_family = AF_INET;
  udp_local_addr.sin_port = htons(std::stoi(argv[1]));
  udp_local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  tcp_remote_addr.sin_family = AF_INET;
  tcp_remote_addr.sin_port = htons(std::stoi(argv[3]));
  tcp_remote_addr.sin_addr.s_addr = inet_addr(argv[2]);

  // make socket
  int udp_sockfd;
  std::mutex m_udp_sockfd;
  if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Error opening socket");
    exit(1);
  }
  //bind socket
  if (bind(udp_sockfd, (struct sockaddr*) &udp_local_addr,
           sizeof(udp_local_addr)) < 0) {
    perror("Error binding socket");
    exit(2);
  }

  //listen
  listen(udp_sockfd, 1);

  while (1) {
    // accept socket - not needed for DATAGRAM
    char *thread_buf = (char*) malloc(kMaxBuf);  // free in getResponce()
    if (thread_buf == nullptr) {
      perror("Error allocating memory");
      std::this_thread::sleep_for(5s);
      continue;
    }

    sockaddr udp_clnt_addr;
    socklen_t udp_clnt_len;
    int bytes = 0;
    if ((bytes = recvfrom(udp_sockfd, thread_buf, kMaxBuf, 0, &udp_clnt_addr,
                          &udp_clnt_len)) < 0) {
      perror("Error receaving data");
      continue;
    }

    std::thread tcp_thread(getResponce, thread_buf, bytes, tcp_remote_addr,
                           udp_sockfd, std::ref(udp_clnt_addr),
                           std::ref(m_udp_sockfd));
    tcp_thread.detach();
  }
  return 0;
}

/**
 * Function connects to addr_in, sends len bytes from buf,
 * receave responce into buf, and returns responce to the udp-client
 */
int getResponce(char *buf, int len, const sockaddr_in addr_in, int udp_sockfd,
                sockaddr &udp_addr, std::mutex &m_udp_sockfd) {
  int bytes;
  int sockfd;

  try {
    m_udp_sockfd.lock();
    // make socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror("getResponce(): Error opening socket");
      return -1;
    }
    // bind socket
    if (connect(sockfd, (struct sockaddr*) &addr_in, sizeof(addr_in)) < 0) {
      perror("getResponce(): Error connecting socket");
      return -2;
    }
    // send request
    if ((bytes = send(sockfd, buf, len, 0)) < 0) {
      perror("getResponce(): Error sending request");
      return -3;
    }
    // get responce
    if ((bytes = recv(sockfd, buf, kMaxBuf, 0)) < 0) {
      perror("getResponce(): Error getting response");
      return -3;
    }
    close(sockfd);

    sendto(udp_sockfd, buf, bytes, 0, &udp_addr, sizeof(udp_addr));
  } catch (...) {
    std::cout << "getResponce(): some exception" << std::endl;
  }
  free(buf);
  m_udp_sockfd.unlock();

  return bytes;
}
