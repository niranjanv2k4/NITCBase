#include "Schema.h"

#include <cmath>
#include <cstring>


int Schema::openRel(char relName[ATTR_SIZE]){

    int ret = OpenRelTable::openRel(relName);

    return (ret >= 0) ? SUCCESS : ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]){
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);

    if(relId == E_RELNOTOPEN)
        return E_RELNOTOPEN;

    return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]){
    if(strcmp(oldRelName, RELCAT_RELNAME) == 0 || strcmp(oldRelName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    if(strcmp(newRelName, RELCAT_RELNAME) == 0 || strcmp(newRelName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;
    
    int is_open = OpenRelTable::getRelId(oldRelName);
    if(is_open != E_RELNOTOPEN)
        return E_RELOPEN;

    return BlockAccess::renameRelation(oldRelName, newRelName);
}

int Schema::renameAttr(char* relName, char* oldAttrName, char* newAttrName){
    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    int is_open = OpenRelTable::getRelId(relName);
    if(is_open != E_RELNOTOPEN)
        return E_RELOPEN;
    
    return BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
}