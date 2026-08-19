#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include "define/constants.h"
#include "Buffer/BlockBuffer.h"

#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  for(int i = 0; i < 2; i++){
    RelCatEntry relCatEntry;

    RelCacheTable::getRelCatEntry(i, &relCatEntry);
    printf("Relation: %s\n", relCatEntry.relName);

    for(int j = 0; j < relCatEntry.numAttrs; j++){

      AttrCatEntry attrCatEntry;
      AttrCacheTable::getAttrCatEntry(i, j, &attrCatEntry);

      printf("\t%s %s\n", attrCatEntry.attrName, attrCatEntry.attrType == NUMBER ? "NUM" : "STR");
    }

  }
  
  return 0;
}