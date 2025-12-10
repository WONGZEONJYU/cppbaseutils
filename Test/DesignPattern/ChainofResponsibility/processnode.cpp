#include <processnode.hpp>

std::shared_ptr<Frame> ProcessNode::process(std::shared_ptr<Frame> const & f) {
    auto frame{ pro(f) };
    if (!m_next_) { return frame; }
    return m_next_->process(frame);
}

void ProcessNode::setNext(std::shared_ptr<ProcessNode> const & next) noexcept
{ m_next_ = next; }
