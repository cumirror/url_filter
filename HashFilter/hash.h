/*
 * Copyright (C) 2015, cumirror <tongjin@qq.com>
 */

#ifndef _CUMIRROR_HASH_H_
#define _CUMIRROR_HASH_H_

int hash_init(size_t size);
int hash_get(uint8_t *key);
int hash_put(uint8_t *key, int value);
void hash_dump();

#endif /* _CUMIRROR_HASH_H_ */
