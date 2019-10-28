# Fast Repo

This is an implementation of an NDN repository based on RocksDB key-value storage.
Repo stores packets as-is, i.e. binary wire format as the value and it's name used as a key.
Repo tries to perform exact prefix match if `CanBePrefix` is false, otherwise, repo performs longest prefix match (see the [source code](https://github.com/remap/fast-repo/blob/master/src/storage/storage-engine.cpp#L321)) using [RocksDB iterator](https://github.com/facebook/rocksdb/wiki/Iterator-Implementation). Since iteration is performed, it is not an optimal solution and need to be revised.

Upon start, repo automatically registers prefixes for the data it has (by finding shortest common prefixes for all keys in the repo -- this requires full DB scan on startup which may take some time depending on how large your storage is).

For installation instructions, see [INSTALL.md](INSTALL.md).

## How is it different from repo-ng?

fast-repo code was based on repo-ng code, however it does not implement repo-ng protocol fully. Some of the functionality need to be ported still and new functionality added (see the table below).


```
           fast-repo             |        repo-ng
---------------------------------+---------------------------------
	❗️                       |✅ Basic Repo Insertion Protocol
	❗️                       |✅ Watched Prefix Insertion Protocol
	🚫                       |✅ Tcp Bulk Insert
	❗️                       |✅ Repo Deletion Protocol
	✅ Pattern Fetching      |🚫
	✅ NDN-RTC Stream        |🚫
	✅ Counter               |🚫
	❓ CNL GObj              |🚫
	❓ CNL GObj Stream       |🚫
---------------------------------+---------------------------------

❗️ -- need to be implemented
🚫 -- won't implement
✅ -- implemented

```
