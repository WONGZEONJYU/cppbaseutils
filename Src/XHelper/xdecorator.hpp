#ifndef X_DECORATOR_HPP
#define X_DECORATOR_HPP 1

#define FOR_EACH_DECORATOR(op) op() op(noexcept)

#define FOR_EACH_CVREF_D(op) \
    op(&) op(&&) \
    op(const &) op(const &&) \
    op(volatile &) op(volatile &&) \
    op(const volatile &) op(const volatile &&)

#define FOR_EACH_CVREF_DECORATOR(op) \
    op(volatile)  op(const)  op(const volatile) \
    FOR_EACH_CVREF_D(op)

#define FOR_EACH_CVREF_DECORATOR_NOEXCEPT(op) \
    FOR_EACH_DECORATOR(op) \
    FOR_EACH_CVREF_DECORATOR(op) \
    op(& noexcept) op(&& noexcept) \
    op(volatile noexcept) op(volatile & noexcept) op(volatile && noexcept) \
    op(const noexcept) op(const & noexcept) op(const && noexcept) \
    op(const volatile noexcept) op(const volatile & noexcept) op(const volatile && noexcept)

#define FOR_EACH_CONST_DECORATOR(op) \
    op(const) op(const &) op(const &&) \
    op(const noexcept) op(const & noexcept) op(const && noexcept) \
    op(const volatile) op(const volatile &) op(const volatile &&) \
    op(const volatile noexcept) op(const volatile & noexcept) op(const volatile && noexcept)

#define FOR_EACH_NONCONST_DECORATOR(op) \
    op() op(&) op(&&) \
    op(noexcept) op(& noexcept) op(&& noexcept) \
    op(volatile) op(volatile &) op(volatile &&) \
    op(volatile noexcept) op(volatile & noexcept) op(volatile && noexcept)

#endif
