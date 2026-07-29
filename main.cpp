#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // StaticBuffer buffer;
  // OpenRelTable cache;

  unsigned char buff1[BLOCK_SIZE];
  Disk::readBlock(buff1, 7000);

  char message[] = "Hello";
  memcpy(buff1 + 20, message, 6);
  
  Disk::writeBlock(buff1, 7000);

  unsigned char buff2[BLOCK_SIZE];
  char message2[6];

  Disk::readBlock(buff2, 7000);

  memcpy(message2, buff2 + 20, 6);

  cout << message2 << endl;

  return 0;
  // return FrontendInterface::handleFrontend(argc, argv);
}