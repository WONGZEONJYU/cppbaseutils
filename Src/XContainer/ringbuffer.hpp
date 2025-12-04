#ifndef X_RINGBUFFER_HPP
#define X_RINGBUFFER_HPP 1

#include <array>
#include <XAtomic/xatomic.hpp>
#include <algorithm>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

template<typename ,std::size_t N = 1024> requires(N > 0)
class RingBuffer;

template<typename T,std::size_t N> requires(N > 0)
class RingBuffer {
    static_assert(N > 0,"RingBuffer Size must be greater than 0");
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

    template<bool isConst>
    class RingBufferIterator {

        template<typename Tp> using rw = std::reference_wrapper<Tp>;
        using rbType = std::conditional_t<isConst,rw<const RingBuffer>,rw<RingBuffer>>;

        rbType m_ref_{};
        size_type m_index_{};

    public:
        using iterator_category = std::random_access_iterator_tag;
        using self_type = RingBufferIterator;
        using value_type = value_type;
        using reference = reference;
        using const_reference = const_reference;
        using pointer = pointer;
        using const_pointer = const_pointer;
        using size_type = size_type;
        using difference_type = difference_type;

        explicit RingBufferIterator(RingBuffer & buffer, size_type const index)
                : m_ref_{buffer}, m_index_{index} {}

        explicit RingBufferIterator(RingBuffer const & buffer, size_type const index)
                : m_ref_{buffer}, m_index_{index} {}

        self_type & operator++() {
            if (m_index_ >= m_ref_.get().size())
                { throw std::out_of_range("Iterator cannot be incremented past the end of the range");}
            ++m_index_;
            return *this;
        }

        self_type operator++(int)
        { self_type temp = *this; ++*this; /*operator++();*/ return temp; }

        self_type & operator--() {
            if(m_index_ <= 0)
                { throw std::out_of_range("Iterator cannot be decremented before the beginning of the range");}
            --m_index_;
            return *this;
        }

        self_type operator--(int)
        { auto temp {*this};  --*this; /*operator--(); */ return temp; }

        self_type operator+(difference_type const offset) const
        { auto temp {*this}; return temp.operator+=(offset); }

        self_type operator-(difference_type const offset) const
        { auto temp { *this }; return temp.operator-=(offset); }

        difference_type operator-(self_type const & other) const
        { return m_index_ - other.m_index_; }

        self_type & operator+=(difference_type const offset) {
            auto const next { (m_index_ + offset) % rbType::capacity() };
            if (next >= m_ref_.size())
                { throw std::out_of_range("Iterator cannot be incremented past the bounds of the range"); }
            m_index_ = next;
            return *this;
        }

        self_type & operator-=(difference_type const offset)
        { return *this += -offset;  /* return operator+=(-offset); */ }

        reference operator[](difference_type const offset)
        { return *(*this + offset); /* return operator+(offset).operator*(); */ }

        const_reference operator[](difference_type const offset) const
        {  return *(*this + offset); /* return operator+(offset).operator*(); */ }

        reference operator*() {
            if(m_ref_.get().empty() || !inBounds())
                { throw std::logic_error("Cannot differentiate the iterator"); }
            return m_ref_.get().m_buffer_[(m_ref_.get().m_head_.loadAcquire() + m_index_) % RingBuffer::capacity()];
        }

        const_reference operator*() const
        { return const_cast<RingBufferIterator&>(this).operator*(); }

        const_pointer operator->() const
        { return std::addressof(operator*()); }

        pointer operator->()
        { return std::addressof(operator*()); }

        constexpr bool compatible(self_type const & other) const
        { return m_ref_.get().m_buffer_.data() == other.m_ref_.get().m_buffer_.data(); }

        [[nodiscard]] constexpr bool inBounds() const
        { return !m_ref_.get().empty() && (m_ref_.get().m_head_.loadAcquire() + m_index_) % RingBuffer::capacity() <= m_ref_.get().m_tail_.loadAcquire(); }

        bool operator==(const self_type& other) const
        { return compatible(other) && m_index_ == other.m_index_; }

        bool operator!=(const self_type& other) const
        { return !(*this == other); }

        bool operator<(const self_type& other) const
        { return m_index_ < other.m_index_; }

        bool operator>(const self_type& other) const
        { return other < *this; }

        bool operator<=(const self_type& other) const
        { return !(other < *this); }

        bool operator>=(const self_type& other) const
        { return !(*this < other); }
    };

    using iterator = RingBufferIterator<false>;
    using const_iterator = RingBufferIterator<true>;

    iterator begin() { return iterator(*this, 0); }
    iterator end() { return iterator(*this, m_size_.loadAcquire()); }

    const_iterator begin() const { return const_iterator(*this, 0); }
    const_iterator end() const { return const_iterator(*this, m_size_.loadAcquire()); }

    const_iterator cBegin() const { return const_iterator(*this, 0); }
    const_iterator cEnd() const { return const_iterator(*this, m_size_.loadAcquire()); }

    // template<bool>
    // friend class RingBufferIterator;

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

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
