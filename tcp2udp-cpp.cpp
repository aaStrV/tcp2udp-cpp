#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXBUF 1600
//#define	PORT		5678
//#define REMOTE_IP 	"127.0.0.1"
//#define REMOTE_PORT	1514

using namespace std;

int getResponce(char*, int, struct sockaddr_in*);

int main(int argc, char **argv) {
  char buf[MAXBUF];
  struct sockaddr_in serv_addr_local, clnt_addr_in;
  struct sockaddr_in serv_addr_remote;
  struct sockaddr clnt_addr;
  socklen_t clnt_len;
  int sockfd;
  int bytes;

  if (argc != 4) {
    cout << "Usage: " << /*argv[0]*/"udp2tcp-cpp"
         << " local-port dest-ip dest-port" << endl;
    exit(0);
  }

  serv_addr_local.sin_family = AF_INET;
  serv_addr_local.sin_port = htons(stoi(argv[1]));
  serv_addr_local.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr_remote.sin_family = AF_INET;
  serv_addr_remote.sin_port = htons(stoi(argv[3]));
  serv_addr_remote.sin_addr.s_addr = inet_addr(argv[2]);

  // make socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Error opening socket");
    exit(1);
  }
  //bind socket
  if (bind(sockfd, (struct sockaddr*) &serv_addr_local, sizeof(serv_addr_local))
      < 0) {
    perror("Error binding socket");
    exit(2);
  }

  //listen
  listen(sockfd, 1);

  while (1) {
    // accept socket - not needed for DATAGRAM

    if ((bytes = recvfrom(sockfd, buf, MAXBUF, 0, &clnt_addr, &clnt_len)) < 0) {
      perror("Error receaving data");
      continue;
    }

    // fork
    if (fork() == 0) {   // chield
      //			cout << "Chield: started" << endl;
      if (fork() == 0) {
        if ((bytes = getResponce(buf, bytes, &serv_addr_remote)) < 0) {
          cerr << "Error getting responce" << endl;
        } else
          sendto(sockfd, buf, bytes, 0, &clnt_addr, sizeof(clnt_addr));

        //				cout << "Chield-chield: finished" << endl;
        exit(0);
      } else {
        exit(0);
      }
    } else {  // parent
      int st;
      wait(&st);
      if (st == 0) {
        //				cout << "Chield finished successfully" << endl;
      } else {
        //				cout << "Chield finished with errorcode: " << st << endl;
      }
    }
  }

  //	cout << "Finished" << endl;
  return 0;
}

/**
 * Function connects to addr_in, sends len bytes from buf,
 * receave responce into buf, and returns responce length
 */
int getResponce(char *buf, int len, struct sockaddr_in *addr_in) {
  int bytes;
  int sockfd;

  // make socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("getResponce(): Error opening socket");
    return -1;
  }
  // bind socket
  if (connect(sockfd, (struct sockaddr*) addr_in, sizeof(*addr_in)) < 0) {
    perror("getResponce(): Error connecting socket");
    return -2;
  }
  // send request
  if ((bytes = send(sockfd, buf, len, 0)) < 0) {
    perror("getResponce(): Error sending request");
    return -3;
  }
  // get responce
  if ((bytes = recv(sockfd, buf, MAXBUF, 0)) < 0) {
    perror("getResponce(): Error sending request");
    return -3;
  }
  close(sockfd);
  return bytes;
}
