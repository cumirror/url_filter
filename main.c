#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "md5/md5.h"
#include "HashFilter/hash.h"
#include "CuckooFilter/cuckoo_filter.h"
#include "BloomFilter/bloom.h"

#define MAX_URL_NUM 655350
#define MAX_URL_LENGTH 64

typedef char URL[MAX_URL_LENGTH];
struct url_table {
    int num;
    URL urls[0];
};

typedef char MD5[16];
struct md5_table {
    int num;
    MD5 key[0];
};

static void getHashKey(char *string, uint8_t *digest)
{
    MD5_CTX context;
    unsigned int len = strlen(string);

    MD5Init(&context);
    MD5Update(&context, (unsigned char *)string, len);
    MD5Final(digest, &context);
}

static struct url_table *load_urldb(char *file)
{
    FILE *pf = NULL;;
    char buf[MAX_URL_LENGTH] = {0};
    struct url_table *url = (struct url_table*)
        malloc(sizeof(struct url_table) + sizeof(URL) * MAX_URL_NUM);

    if (url == NULL) {
        printf("malloc URL table failed\n");
        return NULL;
    }

    if ((pf = fopen(file, "r")) == NULL) {
        printf("open URL db %s failed\n", file);
        free(url);
        return NULL;
    }

    url->num = 0;
    while(fgets(buf, MAX_URL_LENGTH, pf) != NULL) {
        if (url->num >= MAX_URL_NUM) break;
        buf[strlen(buf)-1] = '\0';
        memcpy(url->urls + url->num, buf, MAX_URL_LENGTH);
        url->num += 1;
    }

    if ((MAX_URL_NUM - url->num) > 1024)
        url = (struct url_table*)
            realloc(url, sizeof(struct url_table) + sizeof(URL) * url->num);

    fclose(pf);
    return url;
}

void algorithm_init(struct md5_table *md5)
{
    uint32_t i;

    if (hash_init(md5->num) != 0) {
        printf("hash init failed\n");
        exit(1);
    }

    for (i = 0; i < md5->num; i++)
        hash_put((uint8_t *)(md5->key + i), i);

    hash_dump();

    if (cuckoo_filter_init(md5) != 0) {
        printf("cuckoo init failed\n");
        exit(1);
    }

    for (i = 0; i < md5->num; i++)
        cuckoo_filter_put((uint8_t *)(md5->key + i), &i);

    cuckoo_filter_dump();

    if (bloom_filter_init(2500000) != 0) {
        printf("bloom init failed\n");
        exit(1);
    }

    for (i = 0; i < md5->num; i++)
        bloom_filter_put((uint8_t *)(md5->key + i));

    bloom_filter_dump();

}

void algorithm_test(struct md5_table *md5)
{
    uint32_t i, j;
    uint32_t hash_time, cuckoo_time, bloom_time;
    uint32_t hash_miss = 0, cuckoo_miss = 0, bloom_miss = 0;
    struct timeval start, end;

#define LOOP_TIME 200
    for (i = 0; i < md5->num; i++) {
        if (i%4 == 0) {
            md5->key[i][0] -= 1;
            md5->key[i][2] -= 2;
            md5->key[i][2] -= 3;
        }
    }

    gettimeofday(&start, NULL);
    for (i = 0; i < LOOP_TIME; i++) {
        for (j = 0; j < md5->num; j++) {
            if (hash_get((uint8_t *)(md5->key + j)) != j)
                hash_miss++;
        }
    }
    gettimeofday(&end, NULL);
    hash_time = 1000000*(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec);

    gettimeofday(&start, NULL);
    for (i = 0; i < LOOP_TIME; i++) {
        for (j = 0; j < md5->num; j++) {
            uint32_t index;
            if (cuckoo_filter_get((uint8_t *)(md5->key + j), &index) < 0)
                cuckoo_miss++;
        }
    }
    gettimeofday(&end, NULL);
    cuckoo_time = 1000000*(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec);

    gettimeofday(&start, NULL);
    for (i = 0; i < LOOP_TIME; i++) {
        for (j = 0; j < md5->num; j++) {
            if (bloom_filter_get((uint8_t *)(md5->key + j)) == 0)
                bloom_miss++;
        }
    }
    gettimeofday(&end, NULL);
    bloom_time = 1000000*(end.tv_sec-start.tv_sec)+(end.tv_usec-start.tv_usec);

    printf("Time-Miss hash %f/%d cockoo %f/%d bloom %f/%d\n",
            hash_time/1000.0, hash_miss,
            cuckoo_time/1000.0, cuckoo_miss,
            bloom_time/1000.0, bloom_miss);
}

int main(int argc, char **argv)
{
    struct url_table *url = NULL;
    struct md5_table *md5 = NULL;
    int i;

    if (argc != 2) {
        printf("Usage: ./url url_db\n");
        return 1;
    }

    if ((url = load_urldb(argv[1])) == NULL)
        return 2;

    md5 = (struct md5_table*)
        malloc(sizeof(struct md5_table) + sizeof(MD5) * url->num);
    if (md5 == NULL) {
        printf("malloc md5 table failed\n");
        return 3;
    }

    md5->num = url->num;
    for (i = 0; i < url->num; i ++) {
        unsigned char digest[16];
        getHashKey((char *)(url->urls + i), digest);
        memcpy(md5->key + i, digest, sizeof(MD5));
    }

    algorithm_init(md5);
    algorithm_test(md5);

    free(url);
    free(md5);
    return 0;
}
