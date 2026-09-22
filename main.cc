#include <array>
#include <fstream>
#include <iostream>
#include <string>

void process_file(const std::string& fname) {
        std::ifstream file(fname, std::ios::binary);

        if (!file) {
                std::cerr << "Error: Unable to open file: \"" << fname << "\"\n";

                return;
        }

        constexpr int              buf_size = 8192;
        std::array<char, buf_size> buf{};

        int total_bytes = 0;

        while (true) {
                file.read(buf.data(), buf_size);
                auto bytes_read  = static_cast<int>(file.gcount());
                total_bytes     += bytes_read;

                std::cout << "Read " << bytes_read << " bytes from file.\n";

                if (bytes_read == 0) break;
        }

        std::cout << "Total Bytes: " << total_bytes << "\n";
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
