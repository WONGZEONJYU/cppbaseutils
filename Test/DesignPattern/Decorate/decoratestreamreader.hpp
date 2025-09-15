#ifndef DECORATE_STREAM_READER_HPP
#define DECORATE_STREAM_READER_HPP 1

#include <streamreader.hpp>

class DecorateStreamReader : public StreamReader {

protected:
    StreamReader * m_reader_{};
public:
    ~DecorateStreamReader() override = default;
protected:
    explicit DecorateStreamReader(StreamReader * = {});
};

#endif
