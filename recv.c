#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


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

int countZeros(int vector[], int n)
{
  int i;
  int nr = 0;
  for(i = 1; i<=n; i++)
    if(vector[i] == 0)
      nr++;
  return nr;
}
// function that counts the number of zeros in the confirmation
// Array.(unrecieved frames)

int firstZeroIndex(int vector[], int n)
{
  int i;
  for(i = 1; i <= n; i++)
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

  msg r,t;
  header h;
  init(HOST,PORT);
  int fd, count;
  char outputfile[50];
  strcpy(outputfile, "recv_");
  int dimensiuneVector = -1;
  int numeFisier = -1;
  int i;
  int *confirmation;
  header *arrayOfHeaders;

  while(1)
  {
  	if (recv_message(&r) < 0){
	    perror("Receive message");
	    return -1;
    }
    // a frame has arrived
    memset(h.payload, 0, sizeof(h.payload));
    memcpy(&h, &r.payload, sizeof(header));
    int checksumprimit = h.checksum;
    h.checksum = 0;
    int checksumcalculat = checksum(h);
    if(checksumcalculat != checksumprimit)
    {
      // the frame is not ok so I will just drop it.
      continue;
    }
    if(dimensiuneVector == -1) // If its the first frame, i allocate the structures.
    {
    	dimensiuneVector = h.total_frames;
      confirmation = (int*)calloc(dimensiuneVector + 1, sizeof(int));
      arrayOfHeaders = (header*)calloc(dimensiuneVector + 1, sizeof(header));
    }
    if(numeFisier == -1)
    {
    	strcat(outputfile, h.file_name);
    	fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    	lseek(fd, 0, SEEK_SET);
    	numeFisier = 1;
    }
    if(confirmation[h.number] == 1) // If I already recieved this frame, I drop it.
    {
      continue;
    }
    confirmation[h.number] = 1; // marked the frame as recieved.
    memcpy(&arrayOfHeaders[h.number], &h, sizeof(h)); // I put the vector into my structure
    if(countZeros(confirmation, dimensiuneVector) == 0)
    {
      // I have finally recieved the last frame.
      break;
    }
    sprintf(t.payload,"%d", h.number);
    t.len = strlen(t.payload + 1);
    send_message(&t);
    // send ACK with the recieved frame
  }
  
  for(i = 1; i <= dimensiuneVector; i++)
  {
  	count = write(fd, arrayOfHeaders[i].payload, arrayOfHeaders[i].len);
  	if(count < 0)
    {
      return -1;
    }
  }
  // I write the frames in order, into the output file
  sprintf(t.payload, "DONE");
  t.len = strlen(t.payload + 1);
  send_message(&t);
  close(fd);
  return 0;
}
