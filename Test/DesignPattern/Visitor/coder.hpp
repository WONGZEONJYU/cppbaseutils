#ifndef XUTILS2_CODER_HPP
#define XUTILS2_CODER_HPP

#include <staff.hpp>
#include <memory>

class Coder;
using CoderPtr = std::shared_ptr<Coder>;

class Coder final : public Staff
    ,public std::enable_shared_from_this<Coder>
{
    int m_codelines_{};

public:
    explicit Coder(std::string name,int codelines);
    void accept(Visitor *) override;
    constexpr int codelines() const noexcept
    { return m_codelines_; }
};

#endif
