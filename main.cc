#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

constexpr std::array<uint32_t, 64> s{
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

constexpr int BUF_SIZE     = 8192;
constexpr int CHNK_SIZE    = 64;
uint64_t      TOT_BYTE_CNT = 0;

uint32_t A0 = 0x67452301;
uint32_t B0 = 0xEFCDAB89;
uint32_t C0 = 0x98BADCFE;
uint32_t D0 = 0x10325476;

void process_chunk(std::array<uint8_t, BUF_SIZE>& buf, int offset) {
        std::cout << "Processing buf with offset " << offset << ".\n";

        std::array<uint32_t, 16> M{};

        for (int j = 0; j < 16; j++) {
                uint32_t i = 4 * j + offset;
                M[j] = (static_cast<uint32_t>(buf[i + 3]) << 24) |
                       (static_cast<uint32_t>(buf[i + 2]) << 16) |
                       (static_cast<uint32_t>(buf[i + 1]) << 8) | (static_cast<uint32_t>(buf[i]));
        }

        uint32_t A = A0;
        uint32_t B = B0;
        uint32_t C = C0;
        uint32_t D = D0;

        for (int i = 0; i < 64; i++) {
                uint32_t F = 0;
                uint32_t g = 0;

                if (i < 16) {
                        F = (B & C) | ((~B) & D);
                        g = i;
                } else if (i < 32) {
                        F = (D & B) | ((~D) & C);
                        g = (5 * i + 1) % 16;
                } else if (i < 48) {
                        F = B ^ C ^ D;
                        g = (3 * i + 5) % 16;
                } else {
                        F = C ^ (B | (~D));
                        g = (7 * i) % 16;
                }

                F  = F + A + K[i] + M[g];
                A  = D;
                D  = C;
                C  = B;
                B += (F << s[i]) | (F >> (32 - s[i]));
        }

        A0 += A;
        B0 += B;
        C0 += C;
        D0 += D;
}

void process_file(const std::string& fname) {
        std::ifstream file(fname, std::ios::binary);

        if (!file) {
                std::cerr << "Error: Unable to open file: \"" << fname << "\"\n";

                return;
        }

        std::array<uint8_t, BUF_SIZE> buf{};

        int final_offset       = 0;
        int bytes_in_final_buf = 0;

        while (true) {
                file.read(reinterpret_cast<char*>(buf.data()), BUF_SIZE);
                auto bytes_read  = static_cast<int>(file.gcount());
                TOT_BYTE_CNT    += bytes_read;

                std::cout << "Read " << bytes_read << " bytes from file.\n";

                int offset = 0;

                // process all except final chunk
                for (; offset + CHNK_SIZE < bytes_read; offset += CHNK_SIZE)
                        process_chunk(buf, offset);

                // this is not the final buf, process final chunk as usual
                if (bytes_read == BUF_SIZE) {
                        process_chunk(buf, offset);
                        continue;
                }

                // this is the final buf
                final_offset       = offset;
                bytes_in_final_buf = bytes_read - offset;
                break;
        }

        // std::cout << "final_offset_1: " << final_offset << "\n";
        // std::cout << "bytes_in_final_buf: " << bytes_in_final_buf << "\n";

        // std::array<uint8_t, BUF_SIZE> final_buf{};

        // for (int i = 0; i < bytes_in_final_buf; i++) final_buf[i] = buf[final_offset++];

        // final_buf[bytes_in_final_buf] = 0x80;
        // int marker                    = bytes_in_final_buf + 1;

        // while (marker != CHNK_SIZE - 8) {
        //         final_buf[marker++] = 0x00;

        //         if (marker == CHNK_SIZE) {
        //                 process_chunk(final_buf, 0);
        //                 marker = 0;
        //         }
        // }

        // std::cout << "Total Bytes: " << TOT_BYTE_CNT << "\n";
        file.close();
}

auto main(int argc, char** argv) -> int {
        if (argc <= 1) {
                std::cerr << "Input file must be specified.\n";

                return 1;
        }

        std::string fname = argv[1];

        process_file(fname);

        return 0;
}
