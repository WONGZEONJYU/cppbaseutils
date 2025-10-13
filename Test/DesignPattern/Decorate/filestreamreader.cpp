#include <filestreamreader.hpp>

int FileStreamReader::open(const std::string &url) {
    ifs_.open(url.c_str(),std::ios::in);
    if (!ifs_) {
        return -1;
    }
    return 0;
}

int FileStreamReader::close() {
    ifs_.close();
    return 0;
}

int FileStreamReader::read(void * const buf, int const wantLen) {
    ifs_.read(static_cast<char*>(buf),wantLen);
    auto const readLen = ifs_.gcount();
    return static_cast<int>(readLen);
}
