#include <imagedecodestreamreader.hpp>
#include <iostream>
#include <XHelper/xhelper.hpp>

ImageDecodeStreamReader::ImageDecodeStreamReader(StreamReader * const o)
:DecorateStreamReader(o){}

int ImageDecodeStreamReader::open(std::string const &url) {
    return m_reader_->open(url);
}

int ImageDecodeStreamReader::close() {
    return m_reader_->close();
}

int ImageDecodeStreamReader::read(uint8_t * const buf, int const len) {
    auto const result{ m_reader_->read(buf, len) };
    std::cerr << FUNC_SIGNATURE << '\n';
    return result ;
}
