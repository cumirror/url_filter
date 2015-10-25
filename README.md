# url_filter
采用CuckooFilter、BloomFilter和HashFilter实现对url的过滤，主要用于考察不同hash算法的复杂度和性能，并分析不同场景下算法的优劣。

1.目前已集成HashFilter、CuckooFilter和BloomFilter。

2.CuckooFilter算法来自begeekmyfriend/CuckooFilter，BloomFilter算法来自causes/bloom

参考：

https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf

http://coolshell.cn/articles/17225.html

