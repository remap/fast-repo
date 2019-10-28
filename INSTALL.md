# Submodules

Reposistory has a number of submodules it depends on, so clone using `--recursive` option.

# Dependencies

* [ndn-cpp](https://github.com/named-data/ndn-cpp/blob/master/INSTALL.md)
* [cnl-cpp](https://github.com/named-data/cnl-cpp/blob/master/INSTALL.md)
* [rocksdb](https://github.com/facebook/rocksdb/blob/master/INSTALL.md)
* protobuf
* boost

Compile these dependencies from source or use `brew` (`apt-get`) if aplicable.

```
brew install boost rocksdb protobuf
```

```
sudo apt-get install -y libboost-all-dev protobuf-compiler
```

# Build

1. Clone with third-party code:

```
git clone --recursive https://github.com/remap/fast-repo.git
```

2. Specify paths to dependencies in `CPPFLAGS` and `LDFLAGS` variables and configure & compile the project:

```
CPPFLAGS="..." LDFLAGS="..." ./configure 
make
```

# Run

Run `fast-repo` to see the help message on supported arguments.

