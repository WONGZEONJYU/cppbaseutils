#include <decryptstreamreader.hpp>
#include <iostream>
#include <XHelper/xhelper.hpp>

DecryptStreamReader::DecryptStreamReader(StreamReader * const o)
    :DecorateStreamReader(o) {
}

int DecryptStreamReader::open(std::string const &url) {
    return m_reader_->open(url);
}

int DecryptStreamReader::close() {
    return m_reader_->close();
}

int DecryptStreamReader::read(uint8_t * const buf, int const len) {
    auto const ret{ m_reader_->read(buf, len) };
    std::cerr << FUNC_SIGNATURE << '\n';
    return ret;
}
