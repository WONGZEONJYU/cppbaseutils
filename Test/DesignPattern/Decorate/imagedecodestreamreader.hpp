#ifndef IMAGE_DECODE_STREAM_READER_HPP
#define IMAGE_DECODE_STREAM_READER_HPP 1

#include <decoratestreamreader.hpp>

class ImageDecodeStreamReader final : public DecorateStreamReader {
public:
    explicit ImageDecodeStreamReader(StreamReader * );
    ~ImageDecodeStreamReader() override = default;
    int open(std::string const &url) override;
    int close() override;
    int read(void *, int) override;
};

#endif
