#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "md5/md5.h"
#include "HashFilter/hash.h"

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

static void getKey(char *string, uint8_t *digest)
{
    MD5_CTX context;
    unsigned int len = strlen(string);

    MD5Init(&context);
    MD5Update(&context, (unsigned char *)string, len);
    MD5Final(digest, &context);
}

static struct url_table *load_url(char *file)
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

int main(int argc, char **argv)
{
    struct url_table *url = NULL;
    struct md5_table *md5 = NULL;
    int i;

    if (argc != 2) {
        printf("Usage: ./url url_db\n");
        return 1;
    }

    if ((url = load_url(argv[1])) == NULL)
        return 2;

    md5 = (struct md5_table*)
        malloc(sizeof(struct md5_table) + sizeof(MD5) * url->num);
    if (md5 == NULL)
        return 3;
    md5->num = url->num;

    if (hash_init(url->num) != 0)
        return 4;

    for (i = 0; i < url->num; i ++) {
        unsigned char digest[16];
        getKey((char *)(url->urls + i), digest);
        memcpy(md5->key + i, digest, sizeof(MD5));
        hash_put(digest, i);
    }

    for (i = 0; i < md5->num; i++) {
        int id = 0;
        id = hash_get((uint8_t *)(md5->key + i));
        if (id == i)
            printf("Got %s correct\n", (char *)(url->urls + i));
        else
            printf("Got %s error id %d i %d\n",
                    (char *)(url->urls + i), id, i);
    }


    //hash_dump();

    free(url);
    free(md5);
    return 0;
}
