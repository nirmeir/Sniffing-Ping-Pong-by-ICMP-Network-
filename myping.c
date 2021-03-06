// icmp.cpp
// Robert Iakobashvili for Ariel uni, license BSD/MIT/Apache
//
// Sending ICMP Echo Requests using Raw-sockets.
//

#include <stdio.h>
//  linux

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h> // gettimeofday()

// IPv4 header len without options
//#define IP4_HDRLEN 20

// ICMP header len for echo req
#define ICMP_HDRLEN 8

// Checksum algo
unsigned short calculate_checksum(unsigned short *paddress, int len);

// 1. Change SOURCE_IP and DESTINATION_IP to the relevant
//     for your computer
// 2. Compile it using MSVC compiler or g++
// 3. Run it from the account with administrative permissions,
//    since opening of a raw-socket requires elevated preveledges.
//
//    On Windows, right click the exe and select "Run as administrator"
//    On Linux, run it as a root or with sudo.
//
// 4. For debugging and development, run MS Visual Studio (MSVS) as admin by
//    right-clicking at the icon of MSVS and selecting from the right-click
//    menu "Run as administrator"
//
//  Note. You can place another IP-source address that does not belong to your
//  computer (IP-spoofing), i.e. just another IP from your subnet, and the ICMP
//  still be sent, but do not expect to see ICMP_ECHO_REPLY in most such cases
//  since anti-spoofing is wide-spread.

#define SOURCE_IP "127.0.0.1"
// i.e the gateway or ping to google.com for their ip-address
#define DESTINATION_IP "8.8.8.8"

int main()
{
  //struct ip iphdr; // IPv4 header
  struct icmp icmphdr; // ICMP-header
  char data[IP_MAXPACKET] = "This is the ping.\n";

  int datalen = strlen(data) + 1;

  
  // Message Type (8 bits): ICMP_ECHO_REQUEST
  icmphdr.icmp_type = ICMP_ECHO;

  // Message Code (8 bits): echo request
  icmphdr.icmp_code = 0;

  // Identifier (16 bits): some number to trace the response.
  // It will be copied to the response packet and used to map response to the request sent earlier.
  // Thus, it serves as a Transaction-ID when we need to make "ping"
  icmphdr.icmp_id = 18; // hai

  // Sequence Number (16 bits): starts at 0
  icmphdr.icmp_seq = 0;

  // ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
  icmphdr.icmp_cksum = 0;

  // Combine the packet
  char packet[IP_MAXPACKET];

  // First, IP header.
  //memcpy (packet, &iphdr, IP4_HDRLEN);

  // Next, ICMP header
  memcpy((packet), &icmphdr, ICMP_HDRLEN);

  // After ICMP header, add the ICMP data.
  memcpy(packet + ICMP_HDRLEN, data, datalen);

  // Calculate the ICMP header checksum
  icmphdr.icmp_cksum = calculate_checksum((unsigned short *)(packet), ICMP_HDRLEN + datalen);
  memcpy((packet), &icmphdr, ICMP_HDRLEN);

  struct sockaddr_in dest_in;
  memset(&dest_in, 0, sizeof(struct sockaddr_in));
  dest_in.sin_family = AF_INET;

  // The port is irrelant for Networking and therefore was zeroed.

  // dest_in.sin_addr.s_addr = iphdr.ip_dst.s_addr;
  dest_in.sin_addr.s_addr = inet_addr(DESTINATION_IP);

  // Create raw socket for IP-RAW (make IP-header by yourself)
  int sock = -1;
  if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
  {
    fprintf(stderr, "socket() failed with error: %d", errno);
    fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
    return -1;
  }

  // This socket option IP_HDRINCL says that we are building IPv4 header by ourselves, and
  // the networking in kernel is in charge only for Ethernet header.
  //
  // const int flagOne = 1;
  // if (setsockopt (sock, IPPROTO_IP, IP_HDRINCL,&flagOne,sizeof (flagOne)) == -1)
  // {
  //     fprintf (stderr, "setsockopt() failed with error: %d", errno);
  //     return -1;
  // }

  // Send the packet using sendto() for sending datagrams.

  struct timespec time_start, time_end;

  if (sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *)&dest_in, sizeof(dest_in)) == -1)
  {
    fprintf(stderr, "sendto() failed with error: %d", errno);
    return -1;
  }
  printf("Send ICMP ECHO REQUEST to %s with %d bytes of data\n", DESTINATION_IP, ICMP_HDRLEN + datalen);

  //revieve the pong reply

  // addr_len = sizeof(r_addr);
  bzero(&packet,sizeof(packet));
  socklen_t len = sizeof(dest_in);


  int lenByte = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *)&dest_in, &len) ;


  if (lenByte <= 0 )
  {
    printf("\nPacket receive failed!\n");
  }

  else
  {
    clock_gettime(CLOCK_REALTIME, &time_end);

    double timeElapsed = ((time_end.tv_nsec -time_start.tv_nsec)) / 1000000.0;
    printf("\t receives ICMP-ECHO-REPLY from  %s: bytes=%d time=%.3fms \n\n", DESTINATION_IP, lenByte, timeElapsed);

  
      }
    
  // Close the raw socket descriptor.
  close(sock);
  return 0;
}






// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short *paddress, int len)
{
  int nleft = len;
  int sum = 0;
  unsigned short *w = paddress;
  unsigned short answer = 0;

  while (nleft > 1)
  {
    sum += *w++;
    nleft -= 2;
  }

  if (nleft == 1)
  {
    *((unsigned char *)&answer) = *((unsigned char *)w);
    sum += answer;
  }

  // add back carry outs from top 16 bits to low 16 bits
  sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
  sum += (sum >> 16);                 // add carry
  answer = ~sum;                      // truncate to 16 bits

  return answer;
}
