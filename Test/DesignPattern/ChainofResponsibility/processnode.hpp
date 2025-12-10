#ifndef XUTILS2_PROCESSNODE_HPP
#define XUTILS2_PROCESSNODE_HPP

#include <memory>

struct Frame;

class ProcessNode : public std::enable_shared_from_this<ProcessNode> {

    std::shared_ptr<ProcessNode> m_next_{};
protected:
    constexpr ProcessNode() = default;

public:
    virtual ~ProcessNode() = default;
    std::shared_ptr<Frame> process(std::shared_ptr<Frame> const & );
    virtual std::shared_ptr<Frame> pro(std::shared_ptr<Frame> const & ) = 0;
    void setNext(std::shared_ptr<ProcessNode> const & ) noexcept;
};

#endif
