# FOMO 

Conventional caches are datapath caches due to each accessed item requiring that it must first reside in the cache before being consumed by the application. Non-datapath caches, being free of this requirement, demand entirely new techniques. Current non-datapath caches typically represented by flash- or NVM- based SSDs additionally have limited number of write cycles, motivating cache management strategies that minimize cache updates.
FOMO is a non-datapath cache admission policy that operates in two states: *insert* and *filter*, allowing cache insertions in the former state, and only selectively enabling insertions in the latter. Using two simple and inexpensive metrics, the *cache hit rate* and the *reuse rate of missed items*, FOMO makes judicious decisions about cache admission dynamically.


