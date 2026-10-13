#include "Algebra.h"

#include <cstring>
#include <iostream>
#include <cstdio>

#include <bits/stdc++.h>
using namespace std;

bool isNumber(char *str);

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]){
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if(ret != SUCCESS) {
        return ret;
    }

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

    RelCatEntry srcRelCatEntry;

    ret = RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    if(ret != SUCCESS)
        return ret;

    int src_nAttrs = srcRelCatEntry.numAttrs;
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    for(int offset = 0; offset < src_nAttrs; offset++){

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, offset, &attrCatEntry);

        strcpy(attr_names[offset], attrCatEntry.attrName);
        attr_types[offset] = attrCatEntry.attrType;

    }

    ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if(ret != SUCCESS)
        return ret;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0 || targetRelId >= MAX_OPEN){
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    Attribute record[src_nAttrs];

    RelCacheTable::resetSearchIndex(srcRelId);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    while(BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS){
        
        ret = BlockAccess::insert(targetRelId, record);
        if(ret != SUCCESS){
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }

    }

    Schema::closeRel(targetRel);

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

int Algebra::insert(char relName[ATTR_SIZE], int numberOfAttributes, char record[][ATTR_SIZE]){

    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);
    if(relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    if(relCatEntry.numAttrs != numberOfAttributes)
        return E_NATTRMISMATCH;
    
    Attribute recordValues[numberOfAttributes];

    for(int offset = 0; offset < numberOfAttributes; offset++){

        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId, offset, &attrCatBuf);

        if(attrCatBuf.attrType == NUMBER){
            if(isNumber(record[offset]))
                recordValues[offset].nVal = atof(record[offset]);
            else
                return E_ATTRTYPEMISMATCH;
        }
        else if(attrCatBuf.attrType == STRING){
            strcpy(recordValues[offset].sVal, record[offset]);
        }

    }

    return BlockAccess::insert(relId, recordValues);

}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]){

    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);

    int numAttrs = srcRelCatEntry.numAttrs;
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    for(int offset = 0; offset < numAttrs; offset++){

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, offset, &attrCatEntry);

        strcpy(attrNames[offset], attrCatEntry.attrName);
        attrTypes[offset] = attrCatEntry.attrType;

    }

    int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);
    if(ret != SUCCESS)
        return ret;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0 || targetRelId >= MAX_OPEN){
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    Attribute record[numAttrs];

    RelCacheTable::resetSearchIndex(srcRelId);

    while(BlockAccess::project(srcRelId, record) == SUCCESS){
        
        ret = BlockAccess::insert(targetRelId, record);
        if(ret != SUCCESS){
            OpenRelTable::closeRel(targetRelId);
            Schema::deleteRel(targetRel);
            return ret;
        }

    }

    OpenRelTable::closeRel(targetRelId);

    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
    
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int numAttrs = srcRelCatEntry.numAttrs;

    int attrOffset[tar_nAttrs];
    int attrTypes[tar_nAttrs];

    for(int i = 0; i < tar_nAttrs; i++){

        AttrCatEntry attrCatEntry;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
        if(ret != SUCCESS)
            return ret;

        attrOffset[i] = attrCatEntry.offset;
        attrTypes[i] = attrCatEntry.attrType;

    }

    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attrTypes);
    if(ret != SUCCESS)
        return ret;

    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0 || targetRelId >= MAX_OPEN){
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    Attribute record[numAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        Attribute proj_record[tar_nAttrs];
        for(int i = 0; i < tar_nAttrs; i++){
           proj_record[i] = record[attrOffset[i]];
        }

        ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret != SUCCESS) {
            OpenRelTable::closeRel(targetRelId);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    OpenRelTable::closeRel(targetRelId);

    return SUCCESS;
}
