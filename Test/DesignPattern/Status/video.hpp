#ifndef XUTILS2_VIDEO_HPP
#define XUTILS2_VIDEO_HPP

#include <status.hpp>

class Video final {
    StatusPtr m_status_{};

public:
    explicit Video();
    ~Video() = default;
    void pass();
    void fail();
};

#endif
