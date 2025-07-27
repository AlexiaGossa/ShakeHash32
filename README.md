# ShakeHash32 (old project from 2012-2014)
ShakeHash32 is non-cryptographic hash generator developped for high speed processing of strings hash, additional UDP error checking and other general hash based tables.

ShakeHash32 was written to provide improved quality, high speed and modified BSD licence than other non-cryptographic hashs.
The ShakeHash32 is located in file "hybrid.cpp". The current license only applies to the ShakeHash32. 

Google SMHasher test program is used to compare ShakeHash32, with low 32-bit result from Google CityHash, Murmur3A, MD5, SHA1, FNV and Bob Jenkin’s hash.
All hashs are tested several times using 2 laptops (ASUS N73SV - Intel Core i7 2630QM and Alienware m17x R3 – Intel i7 2860QM).

<img width="436" height="219" alt="image" src="https://github.com/user-attachments/assets/6c0c50dd-8655-47e7-a6ea-a6da4ba665b2" />

The collision graph shows the very good efficiency of ShakeHash.


Avalanche test provides the same result between ShakeHash32, CityHash32, Murmur3A, MD5 and SHA1. Other hash competitors have a very poor avalanche effect.

ShakeHash32 outperforms CityHash32 with ‘Zeroes’ tests.
Murmur3A provides the best result with ‘Window’ test.

Other tests show the same results between CityHash32, ShakeHash32 and Murmur3A.

<img width="1076" height="561" alt="image" src="https://github.com/user-attachments/assets/8efcd340-9b94-4d86-886b-4700bdacfca9" />

Murmur3A outperforms ShakeHash32 (+41%) and CityHash32 (+63%).
ShakeHash32 is faster than CityHash32 (+16%)

Murmur3A is an excellent choice :
- Excellent performance
- Public domain licence
- Low collision result and good avalanche effect

CityHash32
- Very good performance
- MIT Licence
- Low collision result and good avalanche effect

ShakeHash32
- Very good performance
- Modified BSD Licence
- Very low collision result and good avalanche effect
