# Alea - Modern RNGs without compromise

Alea is a collection of Pseudo Random Generator in Modern C++ designed to be used in scientific codebases and in places where determinism and high quality of random streams matters.

# Why ? 

Most Random generators including the de-factor standard one of the STL (Mersenne Twister, the PCGs family and the Xorshifts family under performs or/and are difficult to use reliably in massively processing ( ManyCores, GPUs, NPUs, etc... ) 

This leads to the emergence of a family of RNGs named Counter Based Random Generator (CBRNs) that have much better property in these environment

To know more please refer to the Random123 initial publication.

Alea provides an optimised, modern and tested implementation of these RNGs.

# References

[^1]: Random123, "Parallel random numbers: as easy as 1, 2, 3.", [https://doi.org/10.1145/2063384.2063405](https://doi.org/10.1145/2063384.2063405)
