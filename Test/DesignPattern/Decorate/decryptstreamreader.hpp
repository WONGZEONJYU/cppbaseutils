#ifndef DECRYPT_STREAM_READER_HPP
#define DECRYPT_STREAM_READER_HPP 1

#include <decoratestreamreader.hpp>

class DecryptStreamReader final : public DecorateStreamReader {
public:
    explicit DecryptStreamReader(StreamReader * );
    ~DecryptStreamReader() override  = default;
    int open(std::string const & url) override ;
    int close() override;
    int read(void *, int ) override;
};

#endif
