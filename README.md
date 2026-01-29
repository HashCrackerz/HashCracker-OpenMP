<p align="left">
  <img width="150" alt="logo" src="https://github.com/user-attachments/assets/73297594-3581-4afd-ad0b-39d4bc0e66bf" />
</p>

# HashCracker (OpenMP)
## _Parallel SHA-256 Brute Force & Dictionary (salted) Password Cracker_

[🇬🇧 English](README.md) | [🇮🇹 Italiano](README-IT.md)

![alt text](https://img.shields.io/badge/Language-C++/OpenMP-green)

![alt text](https://img.shields.io/badge/Algorithm-SHA256-purple)

Extension of the project for the Accelerated Computing Systems course at the Master's degree in Computer Engineering, University of Bologna.
**Parallel** application for password cracking through Brute Force attack on SHA-256 hashes (including salted) and dictionary attack, leveraging the parallelism provided by HPC systems and the OpenMP library.

## 📝 Description
This repository contains the OpenMP implementation of a password cracker to reverse SHA-256 hashes. It supports Brute Force attacks and dictionary attacks, including salt support. The goal is to show how to redesign the parallelism strategy by moving from millions of lightweight threads (CUDA) to a limited number of more powerful CPU threads (OpenMP), preserving correctness and achieving good speedups on multi-core systems.

## ⚙️ Features
- **Brute Force**: search-space partitioning by first character (dynamic scheduling strategies).
- **Dictionary Attack**: efficient in-memory loading and parallel word processing.
- **Salted Hashes Support**: salt handling as prefix/suffix in verifications.
- **Early Exit: shared volatile flag for early termination when password is found**.
- **Test / SLURM scripts**: templates to run scalability tests on clusters (G100).

## 📂 Project Structure
- `kernel_omp.cpp` — main OpenMP kernel (brute force).
- `kernel_omp_estensione.cpp` — version with salt & dictionary.
- `ESTENSIONE/` — specific code for hybrid attacks (dictionary + salt).
- `UTILS/` — utility functions (I/O, argument parsing, charset loader).
- `ASSETS/` — charset, wordlists (e.g., CharSet.txt, rockyou_trimmed.txt).

## 🛠️ Requirements
- **Hardware**: Multi-core CPU 
- **Software**:
  - g++ with OpenMP support (GCC / Clang).
  - OpenSSL (for <openssl/sha.h>).
 
## 🚀 Compilation

Make sure OpenSSL libraries are linked correctly.
```shell
g++ -fopenmp -O3 kernel_omp.cpp sequenziale.cpp UTILS/utils.cpp -o bruteforce_omp -lssl -lcrypto
```
_(change file names and dependencies based on the version to compile)_

## 💻 Usage
If your system is not managed by a job scheduler, you can directly run the compiled file: 
```shell
./bruteforce_estensione_omp <hash_target> <min_len> <max_len> <file_charset> [<use_dictionary(yes/no)> <dictionary_file>] 
```
_(example with extended version with salt and dictionary)_

Otherwise, it is necessary to schedule execution through a scheduler (such as SLURM): 

```shell
sbatch ./launcher.sh
```

## 👥 Authors
- [Andrea Vitale](https://github.com/WHYUBM)
- [Matteo Fontolan](https://github.com/itsjustwhitee)
