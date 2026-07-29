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

  unsigned char buff[BLOCK_SIZE];
  
  for(int i = 0; i < 4; i++){
    Disk::readBlock(buff, i);

    for (int j = 0; j < BLOCK_SIZE; j++){
      cout << (int)buff[j] << " ";
    }

  }
  
  return 0;
  // return FrontendInterface::handleFrontend(argc, argv);
}