# Snapshot manager

Simple snapshot utility that uses TAR to compress current directory into a
`.tar` file while ignoring some of the directories.

> [!NOTE]
> This could very well be a bash scirpt but why not have fun with it and do it
> in C and learn something in the process.

## Compilation & Dependencies

```
sudo dnf install libtar-devel

# compile with
make
```

## Options and flags

```
Usage: ./sm [options]

Available options:
  -t,--tag=1.4         set tag of the snapshot
  -h,--help            this help
  -v,--verbose         show detailed log
```

## License

[sm](https://github.com/mitjafelicijan/sm) was written by [Mitja
Felicijan](https://mitjafelicijan.com) and is released under the BSD
two-clause license, see the LICENSE file for more information.

