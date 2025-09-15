#ifndef FILE_STREAM_READER_HPP
#define FILE_STREAM_READER_HPP 1

#include <streamreader.hpp>
#include <fstream>

class FileStreamReader final: public StreamReader {
    std::fstream ifs_{};
public:
    int open(std::string const & url) override;
    int close() override;
    int read(uint8_t *, int wantLen) override;
};

#endif
