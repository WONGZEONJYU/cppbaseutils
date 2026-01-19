#include <testobject.hpp>

namespace QCoro {

    TestContext::TestContext(QEventLoop & el) : m_eventLoop_ { std::addressof(el) } {
        m_eventLoop_->setProperty("testFinished", false);
        m_eventLoop_->setProperty("shouldNotSuspend", false);
    }

    TestContext::TestContext(TestContext &&other) noexcept
    { std::swap(m_eventLoop_, other.m_eventLoop_); }

    TestContext &TestContext::operator=(TestContext &&other) noexcept {
        std::swap(m_eventLoop_, other.m_eventLoop_);
        return *this;
    }

    TestContext::~TestContext() {
        if (m_eventLoop_) {
            m_eventLoop_->setProperty("testFinished", true);
            m_eventLoop_->quit();
        }
    }

    void TestContext::setShouldNotSuspend() const
    { m_eventLoop_->setProperty("shouldNotSuspend", true); }

}
