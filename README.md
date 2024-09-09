# count lines
==================

**Introduction**
---------------

This code counts the total number of lines of code in a given open-source project. It can be used to count lines of code in projects such as the Linux kernel or other software repositories.

### count_loc.c

Count lines of code in a given repository.

### Compile and Run
```bash
gcc count_loc.c -o count_loc
./count_loc
```

**Count Rust Lines only**
-------------------------

### countrust.c
This is only for knowing how much rust code is there in a perticular project.
Count Rust lines of code in a given repository. this is mainly for knowing for each release of linux kernel how much rust code is incorporated.

### Compile and Run
```bash
gcc countrust.c -o countrust
./countrust
```

**Usage**
-----

1. Clone the repository you want to count lines of code for.
2. Run the corresponding executable
3. Enter the path to the repository when prompted.
4. The total number of lines of code will be displayed.
