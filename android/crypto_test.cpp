// Known-answer check for android/crypto_portable.cpp (FIPS 180-4 "abc" vectors, RFC 4231 test case 2).
#include "crypto.h"

#include <cstdio>
#include <cstring>

static bool Same(const uint8_t* got, const char* hex, size_t size) {
    char text[129] = {};
    for (size_t i = 0; i < size; ++i) std::snprintf(text + 2 * i, 3, "%02x", got[i]);
    bool ok = std::strcmp(text, hex) == 0;
    std::printf("%s %s\n", ok ? "OK  " : "FAIL", text);
    return ok;
}

int main() {
    const uint8_t abc[] = {'a', 'b', 'c'};
    uint8_t sha1[20], sha256[32], streamed[32], hmac[32];
    bool ok = true;
    dcp::crypto::Sha1(abc, 3, sha1);
    ok &= Same(sha1, "a9993e364706816aba3e25717850c26c9cd0d89d", 20);
    dcp::crypto::Sha256(abc, 3, sha256);
    ok &= Same(sha256, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", 32);
    // 1,000,000 x 'a' through the stream in uneven pieces.
    dcp::crypto::Sha256Stream stream;
    static uint8_t a[1000];
    std::memset(a, 'a', sizeof(a));
    for (size_t done = 0, step = 1; done < 1000000; done += step, step = step % 997 + 1) {
        size_t take = step < 1000000 - done ? step : 1000000 - done;
        stream.Update(a, take);
        if (take < step) { done += take; break; }
    }
    stream.Finish(streamed);
    ok &= Same(streamed, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0", 32);
    const char* key = "Jefe";
    const char* data = "what do ya want for nothing?";
    dcp::crypto::HmacSha256(reinterpret_cast<const uint8_t*>(key), 4, reinterpret_cast<const uint8_t*>(data),
                            std::strlen(data), hmac);
    ok &= Same(hmac, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", 32);
    return ok ? 0 : 1;
}
