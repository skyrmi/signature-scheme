# Signature Scheme Implementation

This repository contains an implementation of a post-quantum code based signature scheme in C.

## Prerequisites

Before building this project, ensure you have the following:

- C compiler
- Make build tool

## Setup

### 1. Install Required Libraries

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

### 2. Build the Project

To build the project, follow these steps:

```
git clone https://github.com/skyrmi/signature-scheme.git
cd signature-scheme
```

Edit the makefile to ensure that the `include_path` and `library_path` are set appropriately. Then run 
```
make
```
This will create the required executable named `sig`. 

## Usage

This cryptographic signature scheme supports three modular operations:

- **keygen** — Generate public and private keys

- **sign** — Create a signature for a message

- **verify** — Verify a message-signature pair

All outputs are handled via the output/ directory to keep the project root clean. The file `output/output.txt` contains the result of the last operation.

## Key Generation

```bash
./sig keygen [--use-seed] [--regenerate]
```

- Prompts for parameters unless params.txt already exists
- If --use-seed is given, deterministic key generation is used
- If --regenerate is given, forces regeneration even if cached data exists

Output:

- Saves all matrices and seeds (if used) to `matrix_cache/`
- Saves `params.txt` to project root

## Signing a Message

```bash
./sig sign -m <message-file> [-o <signature-file>]
```

- Uses the message from <message-file> (or generates a random one)
- Pads or truncates the message to match required length (dimension of generator matrix)
- Signature is saved to <signature-file> (default: output/signature.txt)

Output: 

- `final_message.txt`: normalized message
- `signature.txt`: signature matrix
- `hash.txt`: hash used in signing
- `public_key.txt`: public key

## Verifying a Signature

```bash
./sig verify -m <message-file> -s <signature-file>
```

- Uses previously saved `signature.txt`, `public_key.txt`, and `params.txt`
- Verifies the signature from <signature-file> against the message

Output:
Prints result to console and `output/output.txt`
