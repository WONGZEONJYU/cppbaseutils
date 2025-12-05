#ifndef X_RINGBUFFER_HPP
#define X_RINGBUFFER_HPP 1

#include <array>
#include <XAtomic/xatomic.hpp>
#include <algorithm>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename> class RingBufferIterator;

template<typename ,std::size_t N = 1024> requires(N > 0) class RingBuffer;

template<typename T,std::size_t N> requires(N > 0)
class RingBuffer {
    static_assert(N > 0,"RingBuffer Size must be greater than 0");
    static_assert(!std::is_const_v<T>, "RingBuffer does not support const types");

    template<typename> friend class RingBufferIterator;

    std::array<T,N> m_buffer_{};
    XAtomicInteger<std::size_t> m_head_{},m_tail_{},m_size_{};

public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = value_type*;
    using const_pointer = const value_type *;
    using iterator = RingBufferIterator<RingBuffer>;
    using const_iterator = RingBufferIterator<RingBuffer const>;

    constexpr RingBuffer() = default;

    constexpr explicit RingBuffer(value_type const (&values)[N])
        : m_tail_{N - 1}, m_size_{N}
    { std::ranges::copy(std::begin(values), std::end(values),m_buffer_.begin()); }

    constexpr explicit RingBuffer(const_reference v)
        :m_tail_{N - 1}, m_size_{N}
    { std::ranges::fill(m_buffer_.begin(), m_buffer_.end(),v); }

    constexpr RingBuffer(RingBuffer const &) = default;
    RingBuffer &operator=(RingBuffer const &) = default;

    constexpr RingBuffer(RingBuffer &&) = default;
    RingBuffer &operator=(RingBuffer &&) = default;

    constexpr auto size() const noexcept{ return m_size_.loadRelaxed(); }
    static constexpr auto capacity() noexcept{ return N; }
    constexpr auto empty() const noexcept{ return !m_size_.loadAcquire(); }
    constexpr auto full() const noexcept{ return N == m_size_.loadAcquire(); }

    constexpr void clear() noexcept {
        m_size_.storeRelease({});
        m_head_.storeRelease({});
        m_tail_.storeRelease({});
    }

    constexpr reference operator[](size_type const pos) noexcept
    { return m_buffer_[(m_head_.loadAcquire() + pos) % N]; }

    constexpr const_reference operator[](size_type const pos) const noexcept
    { return const_cast<RingBuffer&>(*this)[pos]; }

    constexpr reference at(size_type const pos) {
        if (pos < size()) { m_buffer_[(m_head_.loadAcquire() + pos) % N];}
        throw std::out_of_range("Index is out of range!");
    }

    constexpr const_reference at(size_type const pos) const
    { return const_cast<RingBuffer*>(this)->at(pos); }

    constexpr reference front() {
        if(size() > 0) { return m_buffer_[m_head_.loadAcquire()]; }
        throw std::logic_error("Buffer is empty");
    }

    constexpr const_reference front() const
    { return const_cast<RingBuffer*>(this)->front(); }

    constexpr reference back() {
        if(size() > 0) {return m_buffer_[m_tail_.loadAcquire()];}
        throw std::logic_error("Buffer is empty");
    }

    constexpr const_reference back() const
    { return const_cast<RingBuffer*>(this)->back(); }

    constexpr void push_back(value_type const & value)
    { append(value); }

    constexpr void push_back(value_type && value)
    { append(std::move(value)); }

    constexpr value_type pop_front() {
        if (empty()) { throw std::logic_error("Buffer empty"); }
        auto const index{ m_head_.loadAcquire() };
        auto value{ std::move(m_buffer_[index]) };
        m_head_.storeRelease((index + 1) % N);
        m_size_.deref();
        return value;
    }

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, m_size_.loadAcquire()); }

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, size()); }

    const_iterator cBegin() const { return const_iterator(this, 0); }
    const_iterator cEnd() const { return const_iterator(this, size()); }

private:
    template<typename Tp> requires std::is_same_v<std::decay_t<Tp>,value_type>
    constexpr void append(Tp && value) {
        if(empty()) {
            m_size_.ref();
        } else if(!full()) {
            m_tail_.storeRelease((m_tail_.loadAcquire() + 1) % N);
            m_size_.ref();
        } else {
            m_head_.storeRelease((m_head_.loadAcquire() + 1) % N);
            m_tail_.storeRelease((m_tail_.loadAcquire() + 1) % N);
        }
        m_buffer_[m_tail_.loadAcquire()] = std::forward<Tp>(value);
    }
};

template<typename RingBufferType>
class RingBufferIterator {

    template<typename ,std::size_t N> requires(N > 0) friend class RingBuffer;

    using ringBuffer_type = RingBufferType;

    using RefRingBuffer = ringBuffer_type *;

    RefRingBuffer m_ref_{};
    RingBufferType::size_type m_index_{};

public:
    using iterator_category = std::random_access_iterator_tag;
    using self_type = RingBufferIterator;
    using value_type = RingBufferType::value_type;
    using size_type = RingBufferType::size_type;
    using difference_type = std::ptrdiff_t;
    using reference = std::conditional_t< std::is_const_v<ringBuffer_type>
            ,typename RingBufferType::const_reference
            ,typename RingBufferType::reference
        >;
    using pointer = std::conditional_t<std::is_const_v<ringBuffer_type>
                ,typename RingBufferType::const_pointer
                , typename RingBufferType::pointer
        >;

    constexpr RingBufferIterator() = default;

    template<typename OtherIter>
    constexpr RingBufferIterator(RingBufferIterator<OtherIter> const & other)
        :m_ref_ { other.m_ref_ },m_index_ { other.m_index_ } {}

    constexpr self_type & operator++() {
        if (m_index_ >= m_ref_->size()) { throw std::out_of_range("Iterator cannot be incremented past the end of the range");}
        ++m_index_;
        return *this;
    }

    constexpr self_type operator++(int)
    { auto temp{*this}; ++*this; /*operator++();*/ return temp; }

    constexpr self_type & operator--() {
        if (m_index_ <= 0) { throw std::out_of_range("Iterator cannot be decremented before the beginning of the range");}
        --m_index_;
        return *this;
    }

    constexpr self_type operator--(int)
    { auto temp{*this}; --*this; /*operator--(); */ return temp; }

    constexpr self_type operator+(difference_type const offset) const
    { auto temp{*this}; return temp += offset ; /* return temp.operator+=(offset); */ }

    constexpr self_type operator-(difference_type const offset) const
    { auto temp{ *this }; return temp -= offset; /*return temp.operator-=(offset);*/ }

    constexpr difference_type operator-(self_type const & other) const
    { return m_index_ - other.m_index_; }

    constexpr self_type & operator+=(difference_type const offset) {
        auto const next { (m_index_ + offset) % ringBuffer_type::capacity() };
        if (next >= m_ref_->size()) { throw std::out_of_range("Iterator cannot be incremented past the bounds of the range"); }
        m_index_ = next;
        return *this;
    }

    constexpr self_type & operator-=(difference_type const offset)
    { return *this += -offset; /* return operator+=(-offset); */ }

    constexpr reference operator[](difference_type const offset) const
    { return *(*this + offset); /* return operator+(offset).operator*(); */ }

    constexpr reference operator*() const {
        if (m_ref_->empty() || !inBounds()) { throw std::logic_error("Cannot differentiate the iterator"); }
        return m_ref_->m_buffer_[(m_ref_->m_head_.loadAcquire() + m_index_) % RingBufferType::capacity()];
    }

    constexpr pointer operator->() const
    { return std::addressof(operator*()); }

    constexpr bool compatible(self_type const & other) const noexcept
    { return dPtr() == other.dPtr(); }

    [[nodiscard]] constexpr bool inBounds() const noexcept {
        return !m_ref_->empty()
            && (m_ref_->m_head_.loadAcquire() + m_index_) % RingBufferType::capacity()
            <= m_ref_->m_tail_.loadAcquire();
    }

    bool operator<(self_type const & other) const noexcept
    { return m_index_ < other.m_index_; }

    bool operator>(self_type const & other) const noexcept
    { return other < *this; }

    bool operator<=(self_type const & other) const noexcept
    { return !(other < *this); }

    bool operator>=(self_type const & other) const noexcept
    { return !(*this < other); }

    template<typename L,typename R>
    friend constexpr bool operator==(RingBufferIterator<L> const & ,RingBufferIterator<R> const & ) noexcept;

    template<typename L,typename R>
    friend constexpr bool operator!=(RingBufferIterator<L> const & ,RingBufferIterator<R> const & ) noexcept;

private:
    auto dPtr() const noexcept
    { return m_ref_->m_buffer_.data(); }

    RingBufferIterator(RefRingBuffer const rb, size_type const index)
    : m_ref_{rb}, m_index_{index} {}
};

template<typename L,typename R>
constexpr bool operator==(RingBufferIterator<L> const & lhs,RingBufferIterator<R> const& rhs) noexcept
{ return lhs.dPtr() == rhs.dPtr() && lhs.m_index_ == rhs.m_index_; }

template<typename L,typename R>
constexpr bool operator!=(RingBufferIterator<L> const & lhs,RingBufferIterator<R> const& rhs) noexcept
{ return !(lhs == rhs); }

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
