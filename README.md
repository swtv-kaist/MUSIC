# COfigurable MUtation Tool for C programs COMUT

## Compile

Before compilation of tool, clang v3.4 should be installed.

The current version of the tool has only been tested to compile and run successfully on clang v3.4.

After install and build clang, you have to edit llvm source and build directory specification in the first 4 lines of Makefile

```
LLVM_BASE_PATH=/CS453/
LLVM_SRC_DIRECTORY_NAME=llvm
LLVM_BUILD_DIRECTORY_NAME=llvm_build
LLVM_BUILD_MODE=Debug+Asserts
```

LLVM_BASE_PATH is the directory where your llvm source and build directory is.

LLVM_SRC_DIRECTORY_NAME & LLVM_BUILD_DIRECTORY_NAME are the name of folder containing llvm source and build respectively.

In short, according to the current Makefile:

	/CS453/llvm/ is the directory of llvm source.

	/CS453/llvm_build/ is the directory of the llvm build.

	/CS453/llvm_build/Debug+Asserts is the directory containing bin folder with all the executables.

Compile the tool using make to produce tool executable.

## Tool Options

```
./tool inputfilename [option ...]
```

### -o option

Usage: 
```
-o <directory>
```
Used to specify output directory. The directory must exist.

Default is current directory.

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
-rs <line> <col> -re <line> <col>
```
Used to specify the range where mutation operators can be applied. 

Default is start of input file for –rs and end of file for -re.

They do not have to go together and can be used separately

### -m option

Usage:
```
-m <mutant operator 1> [-A <domain>] [-B <range>] [<mutant operator 2> [-A <domain>] [-B <range>] ...]
```
Used to specify the mutant operator(s) to apply. 

Elements in domain, range must be separated by comma,  and written inside double quotes. 

–A, -B order cannot be swapped.

## Output

In the output directory, there will be mutant files for each mutant and and mutant database file named inputfilename_mut_db

## Examples

```
./tool test.c
./tool test.c -o mutant_test/ -rs 3 1 -re 15 5
./tool test.c -o mutant_test/ -l 3 -m ssdl OAAN -A "+, -" -B "*,/" crcr -B "3.5,3"
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