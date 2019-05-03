#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define MAX_LENp 1368

typedef struct {
  char file_name[19];
  char checksum;
  int number;
  int total_frames; 
  int len;
  char payload[MSGSIZE-32];
} header;
// my structure I will copy into payload

char checksum(header h)
{
  char* pointer = (char*)&h;
  int i;
  char checksum = pointer[0];
  for(i = 1; i < sizeof(h); i++)
  {
    checksum ^= pointer[i];
  }
  return checksum;
}
// the function that computes the checksum for the message

int firstZeroIndex(int vector[], int n, int start)
{
  int i;
  for(i = start; i <= n; i++)
  {
    if(vector[i] == 0)
      return i;
  }
  return -1;
}
// the function which finds the first 0 in the confirmation
// Array (searches for the first frame havent successfully
// sent)

int main(int argc,char** argv){
  init(HOST,PORT);
  msg t;
  header h;
  int i;
  printf("START\n");
  int delay = atoi(argv[3]);
  int BDP = ((atoi(argv[2]) * atoi(argv[3])) * 1000);
  // BDP
  int window_size = BDP / (sizeof(msg) * 8);
  // window size
  int fd, count, file_size;
  char buf[MAX_LENp];  
  fd = open(argv[1], O_RDONLY);
  if(fd < 0) {
    printf("Can't find %s file\n", argv[1]);
    return -1;
  }
  // I opened the input file
  file_size = lseek(fd, 0, SEEK_END);
  int total_frames = file_size / sizeof(buf) + 1; 
  // total frames to send
  lseek(fd, 0, SEEK_SET);
  memset(t.payload, 0, sizeof(t.payload));
  // Initialize the msg structure
  header *arrayOfHeaders = (header*)calloc((total_frames + 1), sizeof(h));
  // I declared the array of headers I am going to use to store the information
  int confirmation[total_frames + 1];
  int number = 1;
  while ((count = read(fd, buf, sizeof(buf))))
  {
    strcpy(h.file_name, argv[1]);
    // add file_name in the header
    h.number = number++;
    // add frame number
    memcpy(h.payload, buf, sizeof(buf));\
    // add the payload into my structure
    h.total_frames = total_frames;
    // total frames to be sent
    h.checksum = 0;
    h.len = count;
    // add the length of the payload
    h.checksum = checksum(h);
    // I compute the checksum and add it to my structure
    memcpy(&arrayOfHeaders[h.number], &h, sizeof(h));
    // copy header into msg's payload
  }
  // I create an int array with values 0,1 meaning
  // the frame with index i has been successfully
  // transmitted to reciever. 1 - ACK recieved
  // 0 - ACK not recieved

  for(i = 0; i <= total_frames + 1; i++)
  {
    confirmation[i] = 0;
  }

  memset(t.payload, 0, sizeof(t.payload));
  // Initialize the msg structure
  int nr = 0;
  // how many frames I added in window 
  int index = 1;
  // the current index of the frame I'm sending
  int framesLeft = total_frames;
  // frames left to be sent
  if(window_size > total_frames)
  {
    window_size = total_frames;
  }
  // If my window size is higher than the number of frames
  // I have to send, I will just limit its higher limit
  // to the total number of frames
  while(framesLeft > 0)
  {
    if(nr < window_size)
    {
      memcpy(&t.payload, &arrayOfHeaders[index], sizeof(h));
      // copy header into msg's payload
      t.len = strlen(t.payload) + 1;
      send_message(&t);
      nr++;
      if(firstZeroIndex(confirmation, total_frames, index + 1) > 0)
      {
          index = firstZeroIndex(confirmation, total_frames, index + 1);
      }  
      continue;
    }
    // Wait for ACKs
    if(recv_message_timeout(&t, delay) < 0)
    {
      nr = 0;
    }
    else 
    {
      if(confirmation[atoi(t.payload)] == 0) // If i havent recieved the ACK yet
      {
        confirmation[atoi(t.payload)] = 1; // mark as recieved
        framesLeft--; // One less frame to send.         
      }
      nr--;
      // I can send one more frame in the window.
    }
    if(index == total_frames)
    {
      index = firstZeroIndex(confirmation, total_frames, 1);
    }
    // If I get to the end of the Array, I start to look for missing ACKs
  }
  printf("END\n");
  close(fd);
  return 0;
}
