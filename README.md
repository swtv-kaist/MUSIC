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

Before compilation of tool, clang v4.0 should be installed.

The current version of the tool has only been tested to compile and run successfully on clang v4.0, Linux x86-64.

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

Compile the tool using make to produce tool executable.

## Tool Options

```
./tool inputfilename1 [inputfilename2 ...] [option ...]
```

### -o option

Usage: 
```
-o <directory>
```
Used to specify output directory. The directory must exist.

Default is current directory.

### -p option

Usage: 
```
-p <path-to-compile_commands.json>
```
Used to specify the compilation database file.

Default is none so target file will be compiled with macro definitions, include directories, and so on

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
-rs <line>[ <col>] -re <line>[ <col>]
```
Used to specify the range where mutation operators can be applied. 

Default is start of input file for –rs and end of file for -re.

They do not have to go together and can be used separately

### -m option

Usage:
```
-m mutation_operator_name[:domain[:range]]
```
Used to specify the mutation operator(s) to apply. 

Can be specified multiple times.

Elements in domain, range must be separated by comma.

## Output

In the output directory, there will be mutant files for each mutant and mutant database file named inputfilename_mut_db.

## Examples

```
./music test.c --
./music test.c -o mutant_test/ -rs 3 1 -re 15 5 --
./music test.c -o mutant_test/ -l 3 -m ssdl -m OAAN:+,-:*,/ -p compile_commands.json
```


<!-- ## Deployment

Add additional notes about how to deploy this on a live system

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags). 

## Authors

* **Billie Thompson** - *Initial work* - [PurpleBooth](https://github.com/PurpleBooth)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
* etc

 -->