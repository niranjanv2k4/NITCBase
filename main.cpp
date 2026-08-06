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
  
  RecBuffer relCatBuffer(RELCAT_BLOCK);

  HeadInfo relCatHeader;

  relCatBuffer.getHeader(&relCatHeader);
  
  for(int i = 0; i < relCatHeader.numEntries; i++){

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int currBlock = ATTRCAT_BLOCK;

    while(currBlock != -1){
    
      RecBuffer attrCatBuffer(currBlock);
      HeadInfo attrCatHeader;

      attrCatBuffer.getHeader(&attrCatHeader);

      for(int j = 0; j < attrCatHeader.numEntries; j++){

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        attrCatBuffer.getRecord(attrCatRecord, j);

        if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0){
          if(strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0){
            strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");
          }
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf(" %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);

        }

      }
      currBlock = attrCatHeader.rblock;

    }

    printf("\n");
  }
  
  return 0;
}