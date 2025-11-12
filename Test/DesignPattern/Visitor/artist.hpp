#ifndef XUTILS2_ARTIST_HPP
#define XUTILS2_ARTIST_HPP

#include <staff.hpp>
#include <memory>

class Artist;
using ArtistPtr = std::shared_ptr<Artist>;

class Artist final : public Staff
    ,public std::enable_shared_from_this<Artist>
{
    int m_picNum_{};

public:
    explicit Artist(std::string name,int picNum);
    void accept(Visitor *) override;
    constexpr int getPicNum() const noexcept
    { return m_picNum_; }
};

#endif
