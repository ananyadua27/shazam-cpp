# shazam-cpp

C++17 audio fingerprinting engine implementing the Wang 2003
"Industrial-Strength Audio Search" algorithm that Shazam uses.

Identifies a song from a 5-second noisy audio clip in < 500 ms against a
database of hundreds of songs using a purely combinatorial approach.

```
$ ./shazam register music/porter.wav --title "Everything Goes On"
Reading:       music/porter.wav
Duration:      214.30 s  (44100 Hz, 2 ch → mono)
Fingerprinting …
Fingerprints:  87,412
Registered:    "Everything Goes On"  (id=1)
Elapsed:       1.23 s

$ ./shazam query samples/snippet_with_noise.wav
Fingerprinting …
Fingerprints:  9,814
Searching …

✓  Matched: "Everything Goes On"
   Votes:      347
   Confidence: 0.0354
   Elapsed:    0.38 s
```

## How It Works
### 1. FFT — O(N log N) Spectrogram

The core transform is a Cooley-Tukey radix-2 Decimation-in-Time FFT,
derived from the standard EE signals decomposition:

```
X[k] = A[k] + e^{-i 2π k / N} · B[k]      k = 0 … N-1
```

where `A`, `B` are N/2-point DFTs of the even- and odd-indexed sub-sequences.
This divide-and-conquer recursion reduces O(N²) naïve DFT work to O(N log N).

A Short-Time Fourier Transform (STFT) applies the FFT to overlapping
Hann-windowed frames (4096-sample window, 512-sample hop) to produce a
time-frequency magnitude spectrogram.

### 2. Constellation Map, Peak Extraction

Local maxima in the spectrogram above a magnitude threshold become peaks, the "constellation map." Peaks are noise-robust: additive room noise raises the floor uniformly but doesn't create new local maxima.

### 3. Combinatorial Hashing

For each anchor peak `(f1, t1)`, we pair it with fan-out target peaks
`(f2, t2)` in a look-ahead window and encode each pair as a 32-bit hash:

```
hash = [f1: 9 bits][f2: 9 bits][Δt: 14 bits]
```

stored in a SQLite database as `(hash → song_id, t1)`. The indexed hash
column gives O(1) average lookup.

### 4. Offset Histogram Matching

For a query clip, we generate its fingerprints and look up each hash.  
For every database hit `(song_id, t_db)` against a query fingerprint `(hash, t_q)`:

```
Δ = t_db − t_q   ← time alignment offset
histogram[song_id][Δ]++
```

A genuine match produces a spike in the histogram at a single Δ value.
All correctly-matched peak pairs have the same relative offset. Random
collisions spread uniformly and form a flat noise floor. The song with the
tallest histogram spike wins.

This is the discrete analogue of normalized cross-correlation: finding the
time shift that maximises agreement between two signals.

## Performance

| Operation | Time | Notes |
|---|---|---|
| Register 3-min song | ~1.2 s | FFT + DB write, single thread |
| Fingerprint density | ~400 fps/s of audio | @ default params |
| Query 5-s clip | ~380 ms | includes FFT + 10k DB lookups |
| DB size per song | ~700 KB | ~87k fingerprints × 12 bytes |
| 1M fingerprints | ~12 MB | packed 32-bit hashes in SQLite |

Tested on: Apple M2, macOS 14 / Ubuntu 22.04 with GCC 12.

## Build
### Dependencies

```bash
# macOS
brew install libsndfile sqlite
```

### Compile

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Run tests

```bash
cd build && ctest --output-on-failure
```

## Usage

```bash
# Register songs
./build/shazam register path/to/song.wav --title "Song Name"
./build/shazam register path/to/song.flac           

# Query (identify a clip)
./build/shazam query path/to/snippet.wav

# Manage database
./build/shazam list
./build/shazam stats
./build/shazam remove 3       

# Custom database path
./build/shazam register song.wav --db my_library.db
./build/shazam query   clip.wav  --db my_library.db
```

## Design Decisions

The Cooley-Tukey derivation is a core concept (covered in my
signals courses, EE120); implementing it from scratch demonstrates understanding of
the divide-and-conquer structure rather than treating FFT as a black box.
For production, swapping to FFTW is a one-line change.

SQLite's B-tree index on the hash column gives O(log n) disk lookup, good
enough for millions of fingerprints without loading the whole DB into RAM.
A production system (Shazam scale) would use a custom hash table with
memory-mapped files, but SQLite makes the architecture transparent and testable.

The Wang 2003 method is noise-tolerant, computationally cheap, and produces compact fixed-size hashes.
A 3-minute song generates ~87k fingerprints (~700 KB), vs. multiple MB for
embedding-based methods.

## References

1. Wang, A. (2003). *An Industrial-Strength Audio Search Algorithm.* ISMIR.
   https://www.ee.columbia.edu/~dpwe/papers/Wang03-shazam.pdf

2. Cooley, J. W. & Tukey, J. W. (1965). *An Algorithm for the Machine
   Calculation of Complex Fourier Series.* Mathematics of Computation.

3. Domingues, W. (2013). *Dejavu* (open-source Python reference implementation).
   https://github.com/worldveil/dejavu

## License

MIT
