#ifndef STREAM_READER_HPP
#define STREAM_READER_HPP 1

#include <istream>
#include <string>

class StreamReader {
protected:
    constexpr StreamReader () = default;

public:
    virtual ~StreamReader() = default;
    virtual int open(std::string const & url) = 0;
    virtual int close() = 0 ;
    virtual int read(uint8_t *, int ) = 0;
};

#endif
