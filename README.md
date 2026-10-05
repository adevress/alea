# Alea - Modern RNGs without compromise

[![CI](https://github.com/adevress/alea/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/adevress/alea/actions/workflows/ci.yml)

Alea is a collection of Pseudo Random Generator in Modern C++ designed to be used in scientific codebases and in places where determinism and high quality of random streams matters.

# Why ? 

Most Random generators including the de-factor standard one of the STL (Mersenne Twister, the PCGs family and the Xorshifts family under performs or/and are difficult to use reliably in massively processing ( ManyCores, GPUs, NPUs, etc... ) 

This leads to the emergence of a family of RNGs named Counter Based Random Generator (CBRNs) that have much better property in these environment

To know more please refer to the Random123 initial publication.

Alea provides an optimised, modern and tested implementation of these RNGs.

# GPU / CUDA

Alea is single source: the very same `threefry`, `philox` and
`counter_engine` headers are callable from CUDA kernels, without any code
duplication. The execution space annotations live in
`alea/gpu_portability.hpp` and expand to nothing under a plain C++ compiler.

Build the GPU unit tests and benchmarks with:

```sh
cmake -G Ninja -DALEA_ENABLE_CUDA=ON -DALEA_ENABLE_BENCH=ON ..
ninja
ctest -V            # runs ALEA_GPU_Test
./ALEA_GPU_Bench    # GPU throughput
```

The GPU tests and benchmarks launch 1024 blocks of the maximum number of
threads per block (about one million threads) and compare every device result
with the host reference. `ALEA_ENABLE_CUDA` compiles for the GPU of the build
machine (`CMAKE_CUDA_ARCHITECTURES=native`); pass
`-DCMAKE_CUDA_ARCHITECTURES=...` to target other devices.

# References

[^1]: Random123, "Parallel random numbers: as easy as 1, 2, 3.", [https://doi.org/10.1145/2063384.2063405](https://doi.org/10.1145/2063384.2063405)
