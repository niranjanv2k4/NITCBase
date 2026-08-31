#include "Algebra.h"

#include <cstring>
#include <iostream>
#include <cstdio>

using namespace std;

bool isNumber(char *str);

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]){
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if(ret != SUCCESS)
        return E_ATTRNOTEXIST;

    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if (type == NUMBER) {
        if (isNumber(strVal)) {       // the isNumber() function is implemented below
            attrVal.nVal = atof(strVal);
        } else {
            return E_ATTRTYPEMISMATCH;
        }
    } else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;

    ret = RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    if(ret != SUCCESS)
        return ret;

    cout << "|";
    for(int i = 0; i < relCatEntry.numAttrs; i++){

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        printf(" %s |", attrCatEntry.attrName);
    }

    cout << endl;

    while(true){
        RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

        if(searchRes.block != -1 && searchRes.slot != -1){
            RecBuffer currBlockBuff(searchRes.block);

            Attribute record[relCatEntry.numAttrs];
            currBlockBuff.getRecord(record, searchRes.slot);

            for(int i = 0; i < relCatEntry.numAttrs; i++){
                AttrCatEntry attrCatEntry;
                AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
                
                if (attrCatEntry.attrType == NUMBER)
                    cout << " " << record[i].nVal << "| ";
                else
                    cout << " " << record[i].sVal << "| ";
            }
            cout << endl;
        }
        else {
            break;
        }
    }

    return SUCCESS;
}

bool isNumber(char *str) {
  int len;
  float ignore;
  /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}