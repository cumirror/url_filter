/*
 * Copyright (C) 2015, cumirror <tongjinam@qq.com>
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#ifndef _CUCKOO_FILTER_H_
#define _CUCKOO_FILTER_H_

// #define CUCKOO_DBG

/* Configuration */
#define ASSOC_WAY      (4)  /* 4-way association */

/* Cuckoo hash */
#define cuckoo_hash_lsb(key, count)  (((size_t *)(key))[0] & (count - 1))
#define cuckoo_hash_msb(key, count)  (((size_t *)(key))[1] & (count - 1))

enum { AVAILIBLE, OCCUPIED, DELETED, };

/* The in-memory hash buckets cache filter keys (which are assumed MD5 values)
 * via cuckoo hashing function and index in data at hash_table.
 */
struct hash_slot_cache {
        uint32_t tag : 30;  /* summary of key */
        uint32_t status : 2;  /* FSM */
        uint32_t index;  /* index in data */
};

static inline int is_pow_of_2(uint32_t x)
{
	return !(x & (x-1));
}

static inline uint32_t next_pow_of_2(uint32_t x)
{
	if (is_pow_of_2(x))
		return x;
	x |= x>>1;
	x |= x>>2;
	x |= x>>4;
	x |= x>>8;
	x |= x>>16;
	return x + 1;
}

int cuckoo_filter_init(void *s);
int cuckoo_filter_get(uint8_t *key, uint32_t *index);
void cuckoo_filter_put(uint8_t *key, uint32_t *index);
void cuckoo_filter_dump();

#endif /* _CUCKOO_FILTER_H_ */
