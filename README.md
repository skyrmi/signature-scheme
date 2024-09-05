# Signature Scheme Implementation

This repository contains an implementation of a code based signature scheme in C. The implementation relies on the Brouwer Zimmermann algorithm described in this [paper](https://dl.acm.org/doi/10.1145/3302389), which requires x86_64 vector instructions for calculating the minimum distance.

## Prerequisites

Before building this project, ensure you have the following:

- A system with x86_64 architecture
- GCC compiler
- Make build tool

## Installation

### 1. Install Brouwer Zimmermann Executable
```
git clone https://github.com/Lcrypto/Brouwer-Zimmermann_Hamming_distance.git
cd Brouwer-Zimmermann_Hamming_distance
./configure --enable-vectorization=yes
make && make install
```
Configuring with `--enable-vectorization=yes` is recommended for faster performance. Ensure that the executable is present on path as `test_distance`. 
The executable requires `config.in` (sample provided in this repository) to be present in the same directory.

### 2. Install Required Libraries

This project depends on [FLINT](https://github.com/flintlib/flint) and [libsodium](https://doc.libsodium.org/installation). Check the linked pages for installation instructions, this document shows how to build them from source.

#### FLINT

FLINT requires [GMP](https://gmplib.org/), [MPFR](https://www.mpfr.org/) and [GNU Build System](https://www.gnu.org/software/automake/manual/html_node/GNU-Build-System.html) to be already installed, these can be installed on a Ubuntu system with:

```
apt install libgmp-dev libmpfr-dev make autoconf libtool-bin
```

To download and build FLINT:

```
git clone https://github.com/flintlib/flint.git && cd flint
./bootstrap.sh
./configure                        # ./configure --help for more options
make && make check
```

#### libSodium

Download the latest stable version of the [tarball](https://download.libsodium.org/libsodium/releases/) and then follow these instuctions:

```
./configure
make && make check
make install
```

On Linux systems, to make the linker aware of the newly installed libraries, run the `ldconfig` command.

### 3. Build the Project

To build the project, follow these steps:

```
git clone https://github.com/skyrmi/signature-scheme.git
cd signature-scheme/src
```

Edit the makefile to ensure that the `include_path` and `library_path` are set appropriately. Then run 
```
make
```
This will create the required executable named `main`. 

## Usage

The program will ask for user input for code parameters from the standard input, and generate random values if any are left unspecified. The results are written to a file named `output.txt` in the same directory. For longer computations, the file `temp_matrix.txt` can be inspected in a text reader supporting auto-reload on change from disk to see progress of generator matrix generation.

## References 

- Fernando Hernando, Francisco D. Igual, and Gregorio Quintana-Ortí Fast Implementations of the Brouwer-Zimmermann Algorithm for the Computation of the Minimum Distance of a Random Linear Code. March 2018, https://arxiv.org/pdf/1603.06757.pdf
- Fernando Hernando, Francisco D. Igual, and Gregorio Quintana-Ortí. 2019. Algorithm 994: Fast Implementations of the Brouwer-Zimmermann Algorithm for the Computation of the Minimum Distance of a Random Linear Code. ACM Trans. Math. Softw. 45, 2, Article 23 (June 2019), 28 pages. https://doi.org/10.1145/3302389
