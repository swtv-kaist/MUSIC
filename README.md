# MUtation analySIs tool with high Configurability and extensibility MUSIC

## Install Clang/LLVM 4.0

MUSIC is built on top of Clang/LLVM so the first step is to get a working LLVM 
installation. See 
[Getting Started with the LLVM System](http://llvm.org/docs/GettingStarted.html)
 for more information.

If you are using a recent Ubuntu (≥ 14.04 and ≤ 17.10, e.g. 14.04 LTS), we 
recommend you to use the LLVM packages provided by LLVM itself. Use 
[LLVM Package Repository](http://apt.llvm.org/) to add the appropriate line to 
your /etc/apt/sources.list. As an example, for Ubuntu 14.04, the following 
lines should be added:

```
deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-4.0 main
deb-src http://apt.llvm.org/trusty/ llvm-toolchain-trusty-4.0 main
```

Then add the repository key and install the 4.0 packages:

```
$ wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
$ sudo apt-get update 
$ sudo apt-get install clang-4.0 libclang-common-4.0-dev libclang-4.0-dev libclang1-4.0 libllvm4.0 llvm-4.0 llvm-4.0-dev llvm-4.0-runtime
```

## Compile

Before compilation of MUSIC, clang v4.0 should be installed.

The current version of MUSIC has only been tested to compile and run successfully on clang v4.0, Linux x86-64.

After install and build clang, if necessary, you can edit llvm source and build directory specification in the following 3 lines of Makefile

```
LLVM_SRC_PATH := /usr/lib/llvm-4.0
LLVM_BUILD_PATH := /usr/lib/llvm-4.0
LLVM_BIN_PATH := $(LLVM_BUILD_PATH)/bin
```

LLVM_SRC_PATH & LLVM_BUILD_PATH are directories containing llvm source and build respectively.

LLVM_BIN_PATH is directory containing executables (ex. clang-4.0, llvm-config, ...)

In short, according to the current Makefile:

	/usr/lib/llvm-4.0 is the directory of llvm source.

	/usr/lib/llvm-4.0 is the directory of the llvm build.

	/usr/lib/llvm-4.0/bin is the directory containing bin folder with all the executables.

Compile using make to produce MUSIC executable.

## MUSIC Options

```
./music inputfilename1 [inputfilename2 ...] [option ...]
```

### -o option

Usage: 
```
-o <directory>
```
Used to specify output directory (absolute path). The directory must exist.

Default is current directory.

### -p option

Usage: 
```
-p <path-to-compile_commands.json>
```
Used to specify the compilation database file.

Default is none so target file will be compiled with macro definitions, include directories, and so on

Add -- at the end of the command if you want to apply MUSIC to target file without compilation database file.

### -l option

Usage:
```
-l <int>
```
Used to specify the maximum number of mutants generated at a mutation location by a mutant operator.

Default is generate all mutants possible.

### -rs -re option

Usage:
```
-rs <filename>:<line>[:<col>] -re <filename>:<line>[:<col>]			
```
Used to specify the range where mutation operators can be applied. 

Default is start of input file for –rs and end of file for -re.

They do not have to go together and can be used separately (i.e. only specify -rs or -re is ok).

### -x option

Usage:
```
-x <filename>:<line1>[,<line2>,...]
```
Used to specify the lines which will not be mutated in a target file.

Default is null so all lines in target file are subject to mutation.

-x is prioritized over -rs and -re. A line specified by -x option will be excluded even if it is in mutation range.

### -m option

Usage:
```
-m mutation_operator_name[:domain[:range]]
```
Used to specify the mutation operator(s) to apply. 

Can be specified multiple times.

Elements in domain, range must be separated by comma.

## Output

In the output directory (absolute path), there will be mutant files for each mutant and mutant database file named inputfilename_mut_db.

## Examples

```
./music /home/music/targets/test.c --
./music /home/music/targets/test.c -o /home/music/output/ -rs /home/music/targets/test.c:3:1 --
./music /home/music/targets/test.c -o /home/music/output/ -l 3 -m ssdl -m OAAN:+,-:* -p /home/music/compile_commands.json
```

## License

See the [LICENSE](LICENSE) file for details