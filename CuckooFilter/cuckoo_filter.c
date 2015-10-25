/*
 * Copyright (C) 2015, cumirror <tongjinam@qq.com>
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "cuckoo_filter.h"

typedef char MD5[16];
struct md5_table {
    int num;
    MD5 key[0];
};
#define getSize(s) (((struct md5_table*)(s))->num)
#define getKey(s, i) ((uint8_t *)&((struct md5_table*)(s))->key[i])
#define getKeyLen (sizeof(MD5))

struct hash_table {
    struct hash_slot_cache **buckets;
    struct hash_slot_cache *slots;
    uint32_t slot_num;
    uint32_t bucket_num;
    void *data;
};

static struct hash_table hash_table;

static void dump_md5_key(uint8_t *digest)
{
#ifdef CUCKOO_DBG
    int i;

    printf("MD5: ");
    for (i = 0; i < 16; i++)
        printf("%02x", digest[i]);

    printf("\n");
#endif
}

static inline int key_verify(uint8_t *key, uint32_t index)
{
    return memcmp(key, getKey(hash_table.data, index), getKeyLen) == 0;
}

static int cuckoo_hash_collide(struct hash_table *table, uint32_t *tag, uint32_t index)
{
    int i, j, k, alt_cnt;
    uint32_t old_tag[2], old_index;
    struct hash_slot_cache *slot;

    /* Kick out the old bucket and move it to the alternative bucket. */
    slot = table->buckets[tag[0]];
    old_tag[0] = tag[0];
    old_tag[1] = slot[0].tag;
    old_index = slot[0].index;
    slot[0].tag = tag[1];
    slot[0].index = index;
    i = 0 ^ 1;
    k = 0;
    alt_cnt = 0;

KICK_OUT:
    slot = table->buckets[old_tag[i]];
    for (j = 0; j < ASSOC_WAY; j++) {
        if (slot[j].status == DELETED || slot[j].status == AVAILIBLE) {
            slot[j].status = OCCUPIED;
            slot[j].tag = old_tag[i ^ 1];
            slot[j].index = old_index;
            break;
        }
    }

    if (j == ASSOC_WAY) {
        if (++alt_cnt > 512) {
            if (k == ASSOC_WAY - 1) {
                /* Hash table is almost full and needs to be resized */
                return 1;
            } else {
                k++;
            }
        }
        uint32_t tmp_tag = slot[k].tag;
        uint32_t tmp_index = slot[k].index;
        slot[k].tag = old_tag[i ^ 1];
        slot[k].index = old_index;
        old_tag[i ^ 1] = tmp_tag;
        old_index = tmp_index;
        i ^= 1;
        goto KICK_OUT;
    }

    return 0;
}

static int cuckoo_hash_get(struct hash_table *table, uint8_t *key, uint32_t *p_index)
{
    int i, j;
    uint32_t tag[2], index;
    struct hash_slot_cache *slot;

    tag[0] = cuckoo_hash_lsb(key, table->bucket_num);
    tag[1] = cuckoo_hash_msb(key, table->bucket_num);

#ifdef CUCKOO_DBG
    printf("get t0:%x t1:%x\n", tag[0], tag[1]);
#endif
    dump_md5_key(key);

    /* Filter the key and verify if it exists. */
    slot = table->buckets[tag[0]];
    for (i = 0; i < ASSOC_WAY; i++) {
        if (cuckoo_hash_msb(key, table->bucket_num) == slot[i].tag) {
            if (slot[i].status == OCCUPIED) {
                index = slot[i].index;
                if (key_verify(key, index)) {
                    if(p_index != NULL) *p_index = index; 
                    break;
                }
            } else if (slot[i].status == DELETED) {
#ifdef CUCKOO_DBG
                printf("Key has been deleted!\n");
#endif
                return DELETED;
            }
        }
    }

    if (i == ASSOC_WAY) {
        slot = table->buckets[tag[1]];
        for (j = 0; j < ASSOC_WAY; j++) {
            if (cuckoo_hash_lsb(key, table->bucket_num) == slot[j].tag) {
                if (slot[j].status == OCCUPIED) {
                    index = slot[j].index;
                    if (key_verify(key, index)) {
                        if (p_index != NULL) *p_index = index;
                        break;
                    }
                } else if (slot[j].status == DELETED) {
#ifdef CUCKOO_DBG
                    printf("Key has been deleted!\n");
#endif
                    return DELETED;
                }
            }
        }
        if (j == ASSOC_WAY) {
#ifdef CUCKOO_DBG
            printf("Key not exists!\n");
#endif
            return AVAILIBLE;
        }
    }

    return OCCUPIED;
}

static int cuckoo_hash_put(struct hash_table *table, uint8_t *key, uint32_t index)
{
    int i, j;
    uint32_t tag[2];
    struct hash_slot_cache *slot;

    tag[0] = cuckoo_hash_lsb(key, table->bucket_num);
    tag[1] = cuckoo_hash_msb(key, table->bucket_num);

#ifdef CUCKOO_DBG
    printf("put index:%x t0:%x t1:%x\n", index, tag[0], tag[1]);
#endif

    /* Insert new key into hash buckets. */
    slot = table->buckets[tag[0]];
    for (i = 0; i < ASSOC_WAY; i++) {
        if (slot[i].status == DELETED || slot[i].status == AVAILIBLE) {
            slot[i].status = OCCUPIED;
            slot[i].tag = cuckoo_hash_msb(key, table->bucket_num);
            slot[i].index = index;
            break;
        }
    }

    if (i == ASSOC_WAY) {
        slot = table->buckets[tag[1]];
        for (j = 0; j < ASSOC_WAY; j++) {
            if (slot[j].status == DELETED || slot[j].status == AVAILIBLE) {
                slot[j].status = OCCUPIED;
                slot[j].tag = cuckoo_hash_lsb(key, table->bucket_num);
                slot[j].index = index;
                break;
            }
        }

        if (j == ASSOC_WAY) {
            if (cuckoo_hash_collide(table, tag, index)) {
#ifdef CUCKOO_DBG
                printf("Hash table collision!\n");
#endif
                return -1;
            }
        }
    }

    return 0;
}

static void cuckoo_hash_status_set(struct hash_table *table, uint8_t *key, int status)
{
    uint32_t i, j, tag[2];
    struct hash_slot_cache *slot;

    tag[0] = cuckoo_hash_lsb(key, table->bucket_num);
    tag[1] = cuckoo_hash_msb(key, table->bucket_num);

#ifdef CUCKOO_DBG
    printf("set status:%d t0:%x t1:%x\n", status, tag[0], tag[1]);
#endif
    dump_md5_key(key);

    /* Insert new key into hash buckets. */
    slot = table->buckets[tag[0]];
    for (i = 0; i < ASSOC_WAY; i++) {
        if (cuckoo_hash_msb(key, table->bucket_num) == slot[i].tag) {
            slot[i].status = status;
            return;
        }
    }

    if (i == ASSOC_WAY) {
        slot = table->buckets[tag[1]];
        for (j = 0; j < ASSOC_WAY; j++) {
            if (cuckoo_hash_lsb(key, table->bucket_num) == slot[j].tag) {
                slot[j].status = status;
                return;
            }
        }

        if (j == ASSOC_WAY) {
#ifdef CUCKOO_DBG
            printf("Key not exists!\n");
#endif
        }
    }
}

static void cuckoo_hash_delete(struct hash_table *table, uint8_t *key)
{
    cuckoo_hash_status_set(table, key, DELETED);
}

static void cuckoo_hash_recover(struct hash_table *table, uint8_t *key)
{
    cuckoo_hash_status_set(table, key, OCCUPIED);
}

static void cuckoo_rehash(struct hash_table *table)
{
    int i, j;
    struct hash_table old_table;

    /* Reallocate hash slots */
    old_table.slots = table->slots;
    old_table.slot_num = table->slot_num;
    table->slot_num *= 2;
    table->slots = calloc(table->slot_num, sizeof(struct hash_slot_cache));
    if (table->slots == NULL) {
        table->slots = old_table.slots;
        return;
    }

    /* Reallocate hash buckets associated with slots */
    old_table.buckets = table->buckets;
    old_table.bucket_num = table->bucket_num;
    table->bucket_num *= 2;
    table->buckets = malloc(table->bucket_num * sizeof(struct hash_slot_cache *));
    if (table->buckets == NULL) {
        free(table->slots);
        table->slots = old_table.slots;
        table->buckets = old_table.buckets;
        return;
    }
    for (i = 0; i < table->bucket_num; i++) {
        table->buckets[i] = &table->slots[i * ASSOC_WAY];
    }

    for (i = 0; i < old_table.bucket_num; i++) {
        struct hash_slot_cache *slot = old_table.buckets[i];
        for (j = 0; j < ASSOC_WAY; j++) {
            uint32_t idx;
            uint8_t *key;
            if (slot[i].status == OCCUPIED) {
                /* Duplicated keys in hash table which can cause eternal
                 * hashing collision! Be careful of that!
                 */
                idx = slot[i].index;
                key = getKey(table->data, idx);
                assert(!cuckoo_hash_put(table, key, idx));
            }
        }
    }

    free(old_table.slots);
    free(old_table.buckets);
}

void cuckoo_filter_dump()
{
    printf("Cuckoo table: buckets num %d, slot num %d\n",
            hash_table.bucket_num, hash_table.slot_num);

#ifdef DUMP_DETAIL
    int i, j;
    struct hash_table *table = &hash_table;

    printf("List all keys in hash table (tag/status/index):\n");
    for (i = 0; i < table->bucket_num; i++) {
        printf("bucket[%04x]:", i);
        struct hash_slot_cache *slot = table->buckets[i];
        for (j = 0; j < ASSOC_WAY; j++) {
            printf("\t%04x/%x/%04x", slot[j].tag, slot[j].status, slot[j].index);
        }
        printf("\n");
    }
#endif
}

int cuckoo_filter_get(uint8_t *key, uint32_t *index)
{
    return cuckoo_hash_get(&hash_table, key, index) != OCCUPIED ? -1 : 0;
}

void cuckoo_filter_put(uint8_t *key, uint32_t *index)
{
    if (index != NULL) {
        /* Important: Reject duplicated keys keeping from eternal collision */
        int status = cuckoo_hash_get(&hash_table, key, NULL);
        if (status == OCCUPIED) {
            return;
        } else if (status == DELETED) {
            cuckoo_hash_recover(&hash_table, key);
        } else {
            /* Insert into hash slots */
            if (cuckoo_hash_put(&hash_table, key, *index) == -1) {
                cuckoo_rehash(&hash_table);
                cuckoo_hash_put(&hash_table, key, *index);
            }
        }
    } else {
        /* Delete at the hash slot */
        cuckoo_hash_delete(&hash_table, key);
    }
}

int cuckoo_filter_init(void *s)
{
    int i, hash_size;

    hash_size = next_pow_of_2(getSize(s));

    /* Allocate hash slots */
    hash_table.slot_num = hash_size;
    hash_table.slots = calloc(hash_table.slot_num, sizeof(struct hash_slot_cache));
    if (hash_table.slots == NULL) {
        return -1;
    }

    /* Allocate hash buckets associated with slots */
    hash_table.bucket_num = hash_table.slot_num / ASSOC_WAY;
    hash_table.buckets = malloc(hash_table.bucket_num * sizeof(struct hash_slot_cache *));
    if (hash_table.buckets == NULL) {
        free(hash_table.slots);
        return -1;
    }
    for (i = 0; i < hash_table.bucket_num; i++) {
        hash_table.buckets[i] = &hash_table.slots[i * ASSOC_WAY];
    }

    hash_table.data = s;

    return 0;
}
