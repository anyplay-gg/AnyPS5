#ifndef CORE_LIBS_PRX_LIBSCENETCTL_SRC_NETBRIDGEAUTH_HPP
#define CORE_LIBS_PRX_LIBSCENETCTL_SRC_NETBRIDGEAUTH_HPP


#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace NetBridgeAuth {

constexpr std::size_t NonceSize = 32;
constexpr std::size_t ProofSize = 32;
constexpr std::uint8_t RoleBridge = 1;
constexpr std::uint8_t RoleService = 2;

using Nonce = std::array<std::uint8_t, NonceSize>;
using Proof = std::array<std::uint8_t, ProofSize>;

namespace detail {

class Sha256 {
public:
    Sha256() {
        Reset();
    }

    void Reset() {
        state = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au, 0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
        used = 0;
        total = 0;
    }

    void Update(const std::uint8_t* data, std::size_t size) {
        total += size;
        while (size > 0) {
            const std::size_t take = std::min<std::size_t>(size, 64 - used);
            std::memcpy(block + used, data, take);
            used += take;
            data += take;
            size -= take;
            if (used == 64) {
                Compress(block);
                used = 0;
            }
        }
    }

    std::array<std::uint8_t, 32> Finish() {
        const std::uint64_t bits = total * 8;
        const std::uint8_t one = 0x80;
        Update(&one, 1);
        const std::uint8_t zero = 0;
        while (used != 56) Update(&zero, 1);
        std::uint8_t length[8];
        for (int index = 0; index < 8; ++index) length[index] = static_cast<std::uint8_t>(bits >> (56 - 8 * index));
        Update(length, 8);
        std::array<std::uint8_t, 32> digest{};
        for (int word = 0; word < 8; ++word) {
            for (int index = 0; index < 4; ++index) digest[word * 4 + index] = static_cast<std::uint8_t>(state[word] >> (24 - 8 * index));
        }
        return digest;
    }

private:
    static std::uint32_t Rotate(std::uint32_t value, int bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    void Compress(const std::uint8_t* chunk) {
        static constexpr std::uint32_t K[64] = {
            0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
            0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
            0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
            0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
            0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
            0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
            0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
            0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
        std::uint32_t w[64];
        for (int index = 0; index < 16; ++index) {
            w[index] = (static_cast<std::uint32_t>(chunk[index * 4]) << 24) | (static_cast<std::uint32_t>(chunk[index * 4 + 1]) << 16) |
                (static_cast<std::uint32_t>(chunk[index * 4 + 2]) << 8) | static_cast<std::uint32_t>(chunk[index * 4 + 3]);
        }
        for (int index = 16; index < 64; ++index) {
            const std::uint32_t s0 = Rotate(w[index - 15], 7) ^ Rotate(w[index - 15], 18) ^ (w[index - 15] >> 3);
            const std::uint32_t s1 = Rotate(w[index - 2], 17) ^ Rotate(w[index - 2], 19) ^ (w[index - 2] >> 10);
            w[index] = w[index - 16] + s0 + w[index - 7] + s1;
        }
        std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4], f = state[5], g = state[6], h = state[7];
        for (int index = 0; index < 64; ++index) {
            const std::uint32_t s1 = Rotate(e, 6) ^ Rotate(e, 11) ^ Rotate(e, 25);
            const std::uint32_t choose = (e & f) ^ (~e & g);
            const std::uint32_t t1 = h + s1 + choose + K[index] + w[index];
            const std::uint32_t s0 = Rotate(a, 2) ^ Rotate(a, 13) ^ Rotate(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = s0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    std::array<std::uint32_t, 8> state{};
    std::uint8_t block[64]{};
    std::size_t used = 0;
    std::uint64_t total = 0;
};

}  // namespace detail

inline Proof Sha256Digest(const std::uint8_t* data, std::size_t size) {
    detail::Sha256 hash;
    hash.Update(data, size);
    return hash.Finish();
}

inline Proof Hmac(const std::uint8_t* key, std::size_t keySize, const std::uint8_t* message, std::size_t messageSize) {
    std::uint8_t padded[64]{};
    if (keySize > sizeof(padded)) {
        const auto digest = Sha256Digest(key, keySize);
        std::memcpy(padded, digest.data(), digest.size());
    } else if (keySize > 0) {
        std::memcpy(padded, key, keySize);
    }
    std::uint8_t inner[64];
    std::uint8_t outer[64];
    for (std::size_t index = 0; index < 64; ++index) {
        inner[index] = static_cast<std::uint8_t>(padded[index] ^ 0x36);
        outer[index] = static_cast<std::uint8_t>(padded[index] ^ 0x5c);
    }
    detail::Sha256 first;
    first.Update(inner, sizeof(inner));
    first.Update(message, messageSize);
    const auto innerDigest = first.Finish();
    detail::Sha256 second;
    second.Update(outer, sizeof(outer));
    second.Update(innerDigest.data(), innerDigest.size());
    return second.Finish();
}

inline Proof MakeProof(const std::vector<std::uint8_t>& token, std::uint8_t role, const Nonce& bridgeNonce, const Nonce& serviceNonce) {
    static constexpr char Context[] = "AnyPlay bridge v1";
    std::vector<std::uint8_t> message(Context, Context + sizeof(Context) - 1);
    message.push_back(role);
    message.insert(message.end(), bridgeNonce.begin(), bridgeNonce.end());
    message.insert(message.end(), serviceNonce.begin(), serviceNonce.end());
    return Hmac(token.data(), token.size(), message.data(), message.size());
}

inline bool SameBytes(const std::uint8_t* left, const std::uint8_t* right, std::size_t size) {
    std::uint8_t difference = 0;
    for (std::size_t index = 0; index < size; ++index) difference |= static_cast<std::uint8_t>(left[index] ^ right[index]);
    return difference == 0;
}

inline bool SameProof(const Proof& left, const Proof& right) {
    return SameBytes(left.data(), right.data(), left.size());
}

}  // namespace NetBridgeAuth

#endif
