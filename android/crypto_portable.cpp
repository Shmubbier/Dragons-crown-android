// Android: the hashes from launcher/crypto.h in plain C++ (FIPS 180-4 SHA-1 and SHA-256, RFC 2104 HMAC).
// AES is not needed on this path yet and reports failure.
#include "crypto.h"

#include <cstring>

namespace dcp::crypto {
namespace {

inline uint32_t rotl(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t load_be(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3]; }
inline void store_be(uint8_t* p, uint32_t v) { p[0] = uint8_t(v >> 24); p[1] = uint8_t(v >> 16); p[2] = uint8_t(v >> 8); p[3] = uint8_t(v); }

// Merkle-Damgard buffering shared by both hashes: 64-byte blocks, big-endian 64-bit bit length.
template <typename Compress, size_t Words>
struct Md {
    uint32_t h[Words];
    uint8_t block[64];
    size_t used = 0;
    uint64_t total = 0;
    void Update(const uint8_t* data, size_t size) {
        total += size;
        while (size) {
            size_t take = 64 - used < size ? 64 - used : size;
            std::memcpy(block + used, data, take);
            used += take; data += take; size -= take;
            if (used == 64) { Compress::Run(h, block); used = 0; }
        }
    }
    void Finish(uint8_t* out) {
        uint64_t bits = total * 8;
        uint8_t pad = 0x80;
        Update(&pad, 1);
        uint8_t zero = 0;
        while (used != 56) Update(&zero, 1);
        uint8_t length[8];
        for (int i = 0; i < 8; ++i) length[i] = uint8_t(bits >> (56 - 8 * i));
        Update(length, 8);
        for (size_t i = 0; i < Words; ++i) store_be(out + 4 * i, h[i]);
    }
};

struct Sha1Compress {
    static void Run(uint32_t* h, const uint8_t* b) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) w[i] = load_be(b + 4 * i);
        for (int i = 16; i < 80; ++i) w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h[0], c1 = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (c1 & c) | (~c1 & d); k = 0x5a827999; }
            else if (i < 40) { f = c1 ^ c ^ d; k = 0x6ed9eba1; }
            else if (i < 60) { f = (c1 & c) | (c1 & d) | (c & d); k = 0x8f1bbcdc; }
            else { f = c1 ^ c ^ d; k = 0xca62c1d6; }
            uint32_t t = rotl(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rotl(c1, 30); c1 = a; a = t;
        }
        h[0] += a; h[1] += c1; h[2] += c; h[3] += d; h[4] += e;
    }
};

const uint32_t kSha256K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

struct Sha256Compress {
    static void Run(uint32_t* h, const uint8_t* b) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) w[i] = load_be(b + 4 * i);
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t v[8];
        std::memcpy(v, h, sizeof(v));
        for (int i = 0; i < 64; ++i) {
            uint32_t s1 = rotr(v[4], 6) ^ rotr(v[4], 11) ^ rotr(v[4], 25);
            uint32_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
            uint32_t t1 = v[7] + s1 + ch + kSha256K[i] + w[i];
            uint32_t s0 = rotr(v[0], 2) ^ rotr(v[0], 13) ^ rotr(v[0], 22);
            uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            uint32_t t2 = s0 + maj;
            v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
            v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
        }
        for (int i = 0; i < 8; ++i) h[i] += v[i];
    }
};

using Sha1State = Md<Sha1Compress, 5>;
using Sha256State = Md<Sha256Compress, 8>;

void InitSha1(Sha1State& s) {
    const uint32_t iv[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    std::memcpy(s.h, iv, sizeof(iv));
}
void InitSha256(Sha256State& s) {
    const uint32_t iv[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::memcpy(s.h, iv, sizeof(iv));
}

}  // namespace

bool Sha1(const uint8_t* data, size_t size, uint8_t (&digest)[20]) {
    Sha1State s;
    InitSha1(s);
    s.Update(data, size);
    s.Finish(digest);
    return true;
}

bool Sha256(const uint8_t* data, size_t size, uint8_t (&digest)[32]) {
    Sha256State s;
    InitSha256(s);
    s.Update(data, size);
    s.Finish(digest);
    return true;
}

bool HmacSha256(const uint8_t* key, size_t key_size, const uint8_t* data, size_t size, uint8_t (&digest)[32]) {
    uint8_t k[64] = {};
    if (key_size > 64) {
        uint8_t hashed[32];
        Sha256(key, key_size, hashed);
        std::memcpy(k, hashed, 32);
    } else if (key_size) {
        std::memcpy(k, key, key_size);
    }
    uint8_t pad[64];
    Sha256State inner;
    InitSha256(inner);
    for (int i = 0; i < 64; ++i) pad[i] = k[i] ^ 0x36;
    inner.Update(pad, 64);
    inner.Update(data, size);
    uint8_t inner_digest[32];
    inner.Finish(inner_digest);
    Sha256State outer;
    InitSha256(outer);
    for (int i = 0; i < 64; ++i) pad[i] = k[i] ^ 0x5c;
    outer.Update(pad, 64);
    outer.Update(inner_digest, 32);
    outer.Finish(digest);
    return true;
}

struct Sha256Stream::Impl { Sha256State state; };
Sha256Stream::Sha256Stream() : impl_(new Impl) { InitSha256(impl_->state); }
Sha256Stream::~Sha256Stream() = default;
bool Sha256Stream::Update(const void* data, size_t size) {
    impl_->state.Update(static_cast<const uint8_t*>(data), size);
    return true;
}
bool Sha256Stream::Finish(uint8_t (&digest)[32]) {
    impl_->state.Finish(digest);
    return true;
}

}  // namespace dcp::crypto
