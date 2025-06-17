#ifndef X_ABSTRACT_TASK2_HPP
#define X_ABSTRACT_TASK2_HPP

#include <any>
#include <memory>
#include <functional>
#include <tuple>
#include <XHelper/xhelper.hpp>

XTD_NAMESPACE_BEGIN
XTD_INLINE_NAMESPACE_BEGIN(v1)

class XThreadPool2;
class XAbstractTask2;
using XAbstractTask2_Ptr = std::shared_ptr<XAbstractTask2>;

class XAbstractTask2 : public std::enable_shared_from_this<XAbstractTask2> {
    friend class XThreadPool2;
    class XAbstractTask2Private;
    std::shared_ptr<XAbstractTask2Private> m_d_{};
    X_DECLARE_PRIVATE_D(m_d_,XAbstractTask2Private)
    std::any result_() const;
public:
    X_DEFAULT_COPY_MOVE(XAbstractTask2)
    virtual ~XAbstractTask2() = default;

    template<typename T>
    [[maybe_unused]] T result() const noexcept(false){
        const auto v{result_()};
        return v.has_value() ? std::any_cast<T>(v) : T{};
    }

    [[maybe_unused]] virtual void set_nextHandler(const std::weak_ptr<XAbstractTask2> &);

    [[maybe_unused]] virtual void requestHandler(const std::any &);

    [[maybe_unused]] XAbstractTask2_Ptr joinThreadPool(const std::shared_ptr<XThreadPool2> &) ;

protected:
    XAbstractTask2();
    [[maybe_unused]] bool is_running_() const;
    [[maybe_unused]] virtual void responseHandler(const std::any &) {}
private:
    void exec_();
    void set_result_(const std::any &) const;
    void set_exit_function_(std::function<bool()> &&) const;
    virtual std::any run() = 0;
};

template<typename Fn,typename ...Args>
class XTempTask final: public XAbstractTask2{
    using Tuple = std::tuple<std::decay_t<Fn>,std::decay_t<Args>...>;
    Tuple m_tuple_{};
    using ReturnType = std::invoke_result_t<Fn,Args...>;
    enum class Private{};
    std::any run() override {
        using indices = std::make_index_sequence<std::tuple_size_v<Tuple>>;
        if constexpr (std::is_same_v<ReturnType,void>){
            call(indices{});
            return {};
        }else{
            return call(indices{});
        }
    }

    template<size_t ...I>
    ReturnType call(std::index_sequence<I...>){
        return std::invoke(std::get<I>(std::forward<Tuple>(m_tuple_))...);
    }

public:
    explicit XTempTask(Private,Fn &&fn,Args && ...args):m_tuple_(std::forward<Fn>(fn),std::forward<Args>(args)...){}

    static auto create(Fn &&fn,Args && ...args){
        try{
            return std::make_shared<XTempTask>(Private(),std::forward<Fn>(fn),std::forward<Args>(args)...);
        }
        catch (const std::exception &){
            return std::shared_ptr<XTempTask>{};
        }
    }
};

XTD_INLINE_NAMESPACE_END
XTD_NAMESPACE_END

#endif
