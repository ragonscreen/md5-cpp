#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// constants used while processing each chunk
constexpr std::array<int, 64> s{
    7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
    14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
    4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21,
};

constexpr std::array<uint32_t, 64> K{
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE, 0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE, 0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA, 0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED, 0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C, 0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05, 0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039, 0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1, 0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
};

constexpr std::array<uint32_t, 64> G = []() -> std::array<uint32_t, 64> {
        std::array<uint32_t, 64> g{};

        for (uint32_t i = 0; i < 64; i++) {
                if (i < 16) g[i] = i;
                else if (i < 32) g[i] = (5 * i + 1) % 16;
                else if (i < 48) g[i] = (3 * i + 5) % 16;
                else g[i] = (7 * i) % 16;
        }

        return g;
}();

constexpr int CHUNK_SIZE_BYTES = 64;      // DO NOT CHANGE: MD5 operates on 64-byte (512-bit) chunks
constexpr int BUF_SIZE_BYTES   = 32'768;  // file read buffer size, must be at least 64

using BUF = std::array<uint8_t, BUF_SIZE_BYTES>;

uint64_t FILE_SIZE_BYTES{};

std::array<uint32_t, 4> H = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};  // state

void process_chunk(const BUF& buf, const int offset) {
        std::array<uint32_t, 16> M{};

#pragma GCC unroll 16
        for (uint32_t j = 0; j < 16; j++) {
                uint32_t i = 4 * j + offset;
                M[j] = (static_cast<uint32_t>(buf[i + 3]) << 24) |
                       (static_cast<uint32_t>(buf[i + 2]) << 16) |
                       (static_cast<uint32_t>(buf[i + 1]) << 8) | (static_cast<uint32_t>(buf[i]));
        }

        uint32_t A = H[0];
        uint32_t B = H[1];
        uint32_t C = H[2];
        uint32_t D = H[3];

#pragma GCC unroll 64
        for (int i = 0; i < 64; i++) {
                uint32_t F{};

                if (i < 16) F = (B & C) | (~B & D);
                else if (i < 32) F = (B & D) | (C & ~D);
                else if (i < 48) F = B ^ C ^ D;
                else F = C ^ (B | ~D);

                F += A + K[i] + M[G[i]];
                A  = D;
                D  = C;
                C  = B;
                B += std::rotl(F, s[i]);
        }

        H[0] += A;
        H[1] += B;
        H[2] += C;
        H[3] += D;
}

auto process_input(const std::string& file_name) -> int {
        std::ifstream file(file_name, std::ios::binary);

        if (!file) {
                std::cerr << "Error: Unable to open file \"" << file_name << "\".\n";

                return 1;
        }

        BUF buf{};

        int pad_start_idx{};
        int offset{};

        while (true) {
                file.read(reinterpret_cast<char*>(buf.data()), BUF_SIZE_BYTES);
                auto bytes_read  = static_cast<int>(file.gcount());
                FILE_SIZE_BYTES += bytes_read;

                offset = 0;

                // process all except final chunk
                for (; offset < bytes_read - CHUNK_SIZE_BYTES; offset += CHUNK_SIZE_BYTES)
                        process_chunk(buf, offset);

                // this is not the final buf, process final chunk as usual
                if (bytes_read == BUF_SIZE_BYTES) {
                        process_chunk(buf, offset);
                        continue;
                }

                // this is the final buf

                pad_start_idx = bytes_read - offset;

                // we have a full chunk to process before padding
                if (pad_start_idx == CHUNK_SIZE_BYTES) {
                        process_chunk(buf, offset);
                        pad_start_idx = 0;
                }

                break;
        }

        // move bytes remaining to front of buffer for simpler processing
        for (int i = 0; i < pad_start_idx; i++) buf[i] = buf[i + offset];

        buf[pad_start_idx++] = 0x80;
        int pad_offset       = 0;

        while (pad_start_idx != CHUNK_SIZE_BYTES - 8) {
                if (pad_start_idx == CHUNK_SIZE_BYTES) {
                        process_chunk(buf, pad_offset);
                        pad_offset    += CHUNK_SIZE_BYTES;
                        pad_start_idx  = 0;
                }

                buf[pad_start_idx + pad_offset] = 0x00;
                pad_start_idx++;
        }

        uint64_t file_size_bits = FILE_SIZE_BYTES * 8;

#pragma GCC unroll 8
        for (int i = 0, s = 0; i < 8; i++, s += 8)
                buf[pad_start_idx + pad_offset + i] = static_cast<uint8_t>(file_size_bits >> s);

        process_chunk(buf, pad_offset);

        file.close();

        return 0;
}

auto get_output() -> std::string {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (const auto& w : H)
                for (int s = 0; s < 32; s += 8)
                        oss << std::setw(2) << static_cast<int>(static_cast<uint8_t>(w >> s));

        return oss.str();
}

auto main(int argc, char** argv) -> int {
        if (argc <= 1) {
                std::cerr << "Input file must be specified.\n";

                return 1;
        }

        std::string file_name = argv[1];
        int         retval    = process_input(file_name);

        if (retval == 0) std::cout << get_output() << "\n";

        return retval;
}
