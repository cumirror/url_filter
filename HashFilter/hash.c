/*
 * Copyright (C) 2015, cumirror <tongjinam@qq.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "hash.h"

struct hash_item {
        uint32_t key[4];
        int id;
        struct hash_item *next;
};

struct hash_bucket {
        struct hash_item *head;
        uint32_t item_num;
};

struct hash_table {
        struct hash_bucket *buckets;
        uint32_t item_num;
        uint32_t bucket_num;
};

static int ht_size[] = {
    37, 47, 79, 97, 163, 197, 331, 397, 673, 797, 1361, 1597,
    2729, 3203, 5471, 6421, 10949, 12853, 21911, 25717, 43853,
    51437, 87719, 102877, 175447, 205759, 350899, 411527, 701819,
    823117, 1403641, 1646237, 2807303, 3292489, 5614657, 6584983,
    11229331, 13169977, 22458671, 26339969, 44917381, 52679969,
    89834777, 105359939
};

static struct hash_table hash_table;

/*
 * bob's hash function full avalanche http://burtleburtle.net/bob/hash/integer.html
 */
static uint32_t _hash(uint32_t a)
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

static uint32_t hash(uint8_t *key)
{
    uint32_t a, b, c, d;

    a = *(uint32_t *)key;
    b = *(uint32_t *)(key+4);
    c = *(uint32_t *)(key+8);
    d = *(uint32_t *)(key+12);

    return (_hash(a)^_hash(b)^_hash(c)^_hash(d)) % hash_table.bucket_num;
}

void hash_dump()
{
    printf("Hash table: buckets num %d, items num %d\n",
            hash_table.bucket_num, hash_table.item_num);

#ifdef DUMP_DETAIL
    int i;
    for (i = 0; i < hash_table.bucket_num; i++) {
        struct hash_bucket *bucket = &hash_table.buckets[i];
        struct hash_item *head = bucket->head;

        printf("\tBucket %d: item num %d\n", i, bucket->item_num);

        if (head == NULL) continue;

        while(head) {
            printf("\t\tkey %x%x%x%x id %d\n",
                    head->key[0], head->key[1], head->key[2], head->key[3], head->id);
            head = head->next;
        }
    }
#endif
}

int hash_get(uint8_t *key)
{
    uint32_t index = hash(key);
    struct hash_bucket *bucket = &hash_table.buckets[index];
    struct hash_item *head = bucket->head;

    while(head) {
        if (memcmp(head->key, key, sizeof(head->key)) == 0)
            return head->id;
        head= head->next;
    }

    return -1;
}

int hash_put(uint8_t *key, int value)
{
    uint32_t index = hash(key);
    struct hash_bucket *bucket = &hash_table.buckets[index];
    struct hash_item *head = bucket->head;
    struct hash_item *node = (struct hash_item *)malloc(sizeof(struct hash_item));

    if (node == NULL) return -1;
    memcpy(node->key, key, sizeof(node->key));
    node->id = value;
    node->next = NULL;

    while(head) {
        if (memcmp(head->key, key, sizeof(head->key)) == 0) {
            printf("id %d's is the same as %d: key %x%x%x%x \n",
                    value, head->id,
                    head->key[0], head->key[1], head->key[2], head->key[3]);
            return -1;
        }
        head= head->next;
    }

    head = bucket->head;
    if (head) node->next = head;
    bucket->head = node;

    bucket->item_num += 1;
    hash_table.item_num += 1;

    return 0;
}

int hash_init(size_t size)
{
    int i;

    for (i = 0; i < sizeof(ht_size)/sizeof(int); i++) {
        if (ht_size[i] >= size) {
            break;
        }
    }

    hash_table.item_num = 0;
    hash_table.bucket_num = ht_size[i];
    hash_table.buckets = (struct hash_bucket *)
        calloc(sizeof(struct hash_bucket) * ht_size[i], 1);

    if (hash_table.buckets == NULL)
        return -1;

    return 0;
}
