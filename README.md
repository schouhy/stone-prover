# Overview

[STARK](https://starkware.co/stark/) is a proof system. It uses cutting-edge cryptography to
provide poly-logarithmic verification resources and proof size, with minimal and
post-quantum-secure assumptions.

This repository contains a prover and a verifier for STARKs, and in particular for the CPU AIR
underlying the CairoZero programming language.

# Installation instructions

## Building using the dockerfile

The root directory contains a dedicated Dockerfile which automatically compiles everything.
You should have docker installed (see https://docs.docker.com/get-docker/).

Clone the repository:

```bash
git clone https://github.com/schouhy/stone-prover.git
```

Build the docker image:

```bash
cd stone-prover
docker build --tag stone-prover-test-cases .
```

This will compile stone prover and add a few test cases for the Fibonacci AIR.
Once the docker image is built, you can run the test cases.

Test case 1:
```bash
docker run --rm stone-prover-test-cases ./case_1
```

Test case 2:
```bash
docker run --rm stone-prover-test-cases ./case_2
```
