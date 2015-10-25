/**
 * file name  : hashes.h
 * authors    : cumirror
 * created    : Oct 25, 2015
 *
 * modifications:
 * Date:         Name:            Description:
 * ------------  ---------------  ----------------------------------------------
 * Oct 25, 2015  cumirror         Creation
 */

#ifndef __HASHES_H_INCLUDED__
#define __HASHES_H_INCLUDED__


/*
 * bob's hash function: full avalanche
 * http://burtleburtle.net/bob/hash/integer.html
 */
int __hash(unsigned int a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

static unsigned int hash1(const char *key)
{
    return __hash(*(unsigned int*)key);
}

static unsigned int hash2(const char *key)
{
    return __hash(*(unsigned int*)(key+4));
}

static unsigned int hash3(const char *key)
{
    return __hash(*(unsigned int*)(key+8));
}

static unsigned int hash4(const char *key)
{
    return __hash(*(unsigned int*)(key+12));
}

#endif // __HASHES_H_INCLUDED__
