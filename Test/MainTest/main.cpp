#include <future>
#include <iostream>
#include <XThreadPool/xthreadpool.hpp>
#if !defined(_WIN64) && !defined(_WIN32)
#include <XSignal/xsignal.hpp>
#endif
#include <list>
#include <deque>
#include <XHelper/xutility.hpp>
#include <XObject/xobject.hpp>
#include <XHelper/xoverload.hpp>
#include <XMemory/xmemory.hpp>
#include <utility>
#include <chrono>
#include <XTupleHelper/xtuplehelper.hpp>
#include <XMath/xmath.hpp>
#include <XDesignPattern/xcor.hpp>

static std::mutex mtx{};

static constexpr auto wait_time{2};

struct Functor {
    auto operator()(const int id) const{
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << " id = " << id << "\n";
            }
            XUtils::sleep_for_s(wait_time);
        }
        return id;
    }
};

struct Functor2 {
    auto operator()() const{
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << " id = " << 33 << "\n";
            }
            XUtils::sleep_for_s(wait_time);
        }
        return 33;
    }
};

struct Functor3 {
    std::string m_name{};
    [[nodiscard]] std::string func(const std::string &name) const{
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << " name = " << name << "\n";
            }
            XUtils::sleep_for_s(wait_time);
        }
        return name + m_name;
    }
};

static double Double(const double f){
    for (int i {}; i < 3 ;++i){
        {
            std::unique_lock lock(mtx);
            std::cout << FUNC_SIGNATURE << " f = " << f << "\n";
        }
        XUtils::sleep_for_s(wait_time);
    }
    return f + 100.0;
}

class A final : public XUtils::XRunnable<XUtils::NonConst> {
    std::any run() override {
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << " id = " << m_id_ << "\n";
                m_id_++;
            }
            XUtils::sleep_for_s(wait_time);
        }
        return std::string(FUNC_SIGNATURE) + "end";
    }
    int m_id_{};
public:
    explicit A(const int id):m_id_{id}{}
    ~A() override = default;
};

[[maybe_unused]] static void test1() {
    bool exit_{};
#if !defined(_WIN32) && !defined(_WIN64)
    const auto sigterm{XUtils::Signal_Register(SIGTERM,{},[&]{
        exit_ = true;
    })};

    const auto sigint{XUtils::Signal_Register(SIGINT,{},[&]{
        exit_ = true;
    })};

    const auto sigkill {XUtils::Signal_Register(SIGKILL,{},[&]{
        exit_ = true;
    })};
#endif
    const auto pool2{XUtils::XThreadPool::create(XUtils::XThreadPool::Mode::CACHE)},
                pool3{XUtils::XThreadPool::create(XUtils::XThreadPool::Mode::CACHE)};
#if 1
    //pool2->setMode(XUtils::XThreadPool2::Mode::FIXED);
    pool2->setThreadTimeout(70);
    //pool2->start();

    for (int i{};i < 30;++i){
       std::make_shared<A>(i)->joinThreadPool(pool2);
    }

    //pool2->stop();

    Functor3 f3{.m_name = "test"};

    const auto p1{pool2->runnableJoin(&Functor3::func, std::addressof(f3), "34")} ,
            p2{pool2->runnableJoin(Double,35.0)};

    XUtils::XAbstractRunnable_Ptr lambda{};
    lambda = pool2->runnableJoin([&](const int& id){
        pool2->stop();
        pool2->start();
        pool2->runnableJoin(lambda);
        std::cerr << "p1->result<std::string>(): " << p1->result<std::string>(XUtils::XAbstractRunnable::Model::NONBLOCK) << "\n" << std::flush;
        for (int i {}; i < 3;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << "lambda id = " << id << "\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
        return id + 10;
    },31);

    pool3->runnableJoin(lambda);
    pool2->runnableJoin(Functor(),32);
    pool2->runnableJoin(Functor2());

    const auto last_time{std::chrono::system_clock::now()};
    while (std::chrono::system_clock::now() - last_time < std::chrono::seconds(90)){
        if (exit_){ break;}
        {
            std::unique_lock lock(mtx);
            std::cout << "current threads: " << pool2->currentThreadsSize() << "\n" <<
                "busy threads: " << pool2->busyThreadsSize() << "\n" <<
                "idle threads: " << pool2->idleThreadsSize() << "\n" <<
                "tasks :" << pool2->currentTasksSize() << "\n" <<
                std::flush;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "lambda->result<int>(): " << lambda->result<int>() << "\n" << std::flush;
     std::cout << "p1->result<std::string>(): " << p1->result<std::string>() << "\n"<< std::flush;
     std::cout << "p2->result<double>(): " << p2->result<double>() << "\n" << std::flush; //与上同理

    pool3->runnableJoin(lambda);

    std::cout << "pool3 lambda->result<int>(): " << lambda->result<int>() << "\n" << std::flush;
    std::cout << "pool3 lambda->result<int>(): " << lambda->result<int>() << "\n" << std::flush;
#else
    //pool2->setMode(XUtils::XThreadPool2::Mode::FIXED);
    XUtils::XAbstractTask2_Ptr task1{},task2{};
    task1 = pool2->taskJoin([&](const auto &data_){
        std::cerr << "task1->result<int>(): " <<
            task1->result<int>() << "\n" << std::flush;
        pool2->stop();//安全
        task1->joinThreadPool(pool2);//安全
        task2 = pool2->taskJoin(std::make_shared<A>(456));
        for (int i{};i < 3;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << __PRETTY_FUNCTION__ << " data = " << data_ << "\n";
            }
            XUtils::sleep_for_s(wait_time);
        }
        std::cerr <<
            "task2->result<std::string>(): " <<
            task2->result<std::string>() << "\n" << std::flush;
        task2->joinThreadPool(pool2);
        return data_;
    },123);

    const auto last_time{std::chrono::system_clock::now()};
    while (std::chrono::system_clock::now() - last_time < std::chrono::seconds(90)){
        if (exit_){ break;}
        {
            std::unique_lock lock(mtx);
            std::cout << "current threads: " << pool2->currentThreadsSize() << "\n" <<
                      "busy threads: " << pool2->busyThreadsSize() << "\n" <<
                      "idle threads: " << pool2->idleThreadsSize() << "\n" <<
                      "tasks :" << pool2->currentTasksSize() << "\n" <<
                      std::flush;
        }
        XUtils::sleep_for_s(1);
    }
    std::cerr << "task1 return: " << task1->result<int>(std::chrono::seconds(5)) << "\n";
    std::cerr << "task1 return: " << task1->result<int>(std::chrono::seconds(5)) << "\n";
    std::cerr << "task2 return: " << task2->result<std::string>(task2->NonblockModel) << "\n";
    std::cerr << "task2 return: " << task2->result<std::string>(task2->NonblockModel) << "\n";

    std::cerr << "sss:" << std::make_shared<A>(0)->result<std::string>() << "\n";
#endif
}

#include <thread>

[[maybe_unused]] static void test2(){

    bool exit_{};
    std::atomic_bool b{};
    std::thread{[&]{
        while (!exit_){
            XUtils::sleep_for_s(10);
            b.wait({},std::memory_order_acquire);
            std::cerr << FUNC_SIGNATURE << "\n";
            b.store({},std::memory_order_release);
        }
    }}.detach();

    while (true){
        int v{};
        std::cin >> v;
        if (v < 0){
            break;
        }
        b.store(true,std::memory_order_release);
        b.notify_all();
    }
    exit_ = true;
}

[[maybe_unused]] static void test3(){

    using namespace std::chrono;

    std::deque<XUtils::XAbstractRunnable_Ptr> tasks1,task2s{};
    for (int i{};i < 1000000;++i){
        tasks1.push_back(std::make_shared<A>(10));
    }

    auto last_time{system_clock::now()};
    for (const auto &task : tasks1){
        task2s.push_back(task);
    }
    auto runtime{duration_cast<milliseconds>(system_clock::now() - last_time).count()};
    std::cerr <<  "deque w:" << runtime << std::endl;

    std::unordered_map<void*,XUtils::XAbstractRunnable_Ptr> tasks2{};
    last_time = system_clock::now();

    for (const auto &task : tasks1){
        tasks2[task.get()] = task;
    }

    runtime = duration_cast<milliseconds>(system_clock::now() - last_time).count();
    std::cerr <<  "unordered_map:" << runtime << std::endl;
}

HAS_MEM_FUNC(call,0)
HAS_MEM_VALUE(m_a)
HAS_MEM_TYPE(type)

struct AS{
    void call() {

    }
    void operator()() const {}
    int m_a{};
    using type = AS;
};

void call(const int a){
    std::cerr << a << std::endl;
}

[[maybe_unused]] static void test4(int)
{
    using namespace XUtils;
    using List_t = XPrivate::List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t>;
    using Value = XPrivate::List_Left<List_t,6>::Value;
    std::cout << XUtils::typeName<Value>() << std::endl;
    std::string filename{"IMAGE01.PNG"};
    filename = XUtils::toLower(std::move(filename));
    std::cerr << filename << std::endl;
    std::cerr << filename.substr(filename.find('.')) << std::endl;
}

[[maybe_unused]]
static void test5(){

    std::cerr << sizeof(std::size_t) << std::endl;

    std::tuple t{0,1l,2ll,3ul,4ull,0.1f,0.2,"const char *",std::string{"std::string"}};

    std::cerr << std::boolalpha << XUtils::is_tuple_v<decltype(t)> << std::endl;
    std::cerr << std::boolalpha << XUtils::is_tuple_v<std::tuple<>> << std::endl;

    XUtils::for_each_tuple(XUtils::Left_Tuple<2>(t),[](std::size_t & index,const auto &i) {
        std::cerr << "index:" <<  index++ << std::endl;
        std::cerr << i << std::endl;
    });

    std::cerr << std::endl;

    XUtils::for_each_tuple(XUtils::SkipFront_Tuple<2>(t),[](std::size_t & ,const auto &i) {
        std::cerr << i << std::endl;
    });

    std::cerr << std::endl;

    XUtils::for_each_tuple(XUtils::Last_Tuple<2>(t),[](std::size_t & ,const auto &i) {
        std::cerr << i << std::endl;
    });
}

class ATest:public XUtils::XObject {
public:
    void send(int ) && noexcept {

    }

    void slot(int) const volatile & noexcept {

    }
};

class BTest:public XUtils::XObject {

public:
    void send(int d)  noexcept {
        emitSignal(this,&BTest::send, nullptr,d);
    }

    void slot(int) const volatile & noexcept {

    }

};

template<typename T, typename... Args>
class is_default_constructor_accessible {

    enum {
        result = std::disjunction_v<std::is_constructible<T, Args...>
        ,std::is_nothrow_constructible<T, Args...>
        ,std::is_trivially_constructible<T,Args...>
        >
    };

    template<typename > struct is_copy_move_constructor {
        enum { value = false };
    };

#if __cplusplus >= 202002L
    template<typename ...AS> requires(sizeof...(AS) == 1)
    struct is_copy_move_constructor<std::tuple<AS...>> {
        using Tuple_ = std::tuple<AS...>;
        using First_ = std::tuple_element_t<0, Tuple_>;
        enum {
            value = std::disjunction_v<
                std::is_same<First_, T &>,
                std::is_same<First_, const T &>,
                std::is_same<First_, T &&>,
                std::is_same<First_, const T &&>
            >
        };
    };
#else
    template<> struct is_copy_move_constructor<std::tuple<>> {
        enum { value = false };
    };
    template<typename ...AS>
    struct is_copy_move_constructor<std::tuple<AS...>> {
    private:
        using Tuple_ = std::tuple<AS...>;
        using First_ = std::tuple_element_t<0, Tuple_>;
    public:
        enum {
            value = std::disjunction_v<
                std::is_same<First_, T &>,
            std::is_same<First_, const T &>,
            std::is_same<First_, T &&>,
            std::is_same<First_, const T &&>>
        };
    };
#endif

public:
    enum {
        value = result && !is_copy_move_constructor<std::tuple<Args...>>::value
    };
};

class CTest final : public XUtils::XTwoPhaseConstruction<CTest> {
    X_TWO_PHASE_CONSTRUCTION_CLASS

    bool construct_(int const a){
        std::cerr << FUNC_SIGNATURE << " a = " << a << std::endl;
        return true;
    }

    bool construct_(std::string && a1,int const a2){
        std::cerr << FUNC_SIGNATURE << " a1 = " << a1 << " ,a2 = " << a2 << std::endl;
        return true;
    }

    bool construct_() {
        std::cerr << FUNC_SIGNATURE << std::endl;
        return true;
    }

    explicit CTest(int &a) noexcept{
        std::cerr << FUNC_SIGNATURE << "a = " << a << std::endl;
    }
protected:
    CTest() noexcept {
        std::cerr << FUNC_SIGNATURE << std::endl;
    }

public:
//     CTest() noexcept {
//         std::cerr << FUNC_SIGNATURE << std::endl;
//     }

    ~CTest(){
        delete new int[10];
        std::cerr << FUNC_SIGNATURE << std::endl;
    }
};

class AAA final : public XUtils::XTwoPhaseConstruction<AAA> {
    X_TWO_PHASE_CONSTRUCTION_CLASS
    int aa{100};
    void p(){
        using namespace std::chrono_literals;
        std::cerr << FUNC_SIGNATURE << aa <<  "\n";
    }

protected:
    AAA(int )
    {
        std::cerr << FUNC_SIGNATURE << "\n";
    }

    ~AAA(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
    bool construct_(){return true;}
};


struct BBB{

    BBB(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
    ~BBB(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
};

[[maybe_unused]] static void test6(){

#if 1
    {
        int a1{1}, a2{2};
        std::string aa{"2"};
        auto p1 = CTest::CreateUniquePtr(XUtils::Parameter{std::ref(a1)}, XUtils::Parameter{100});
        auto p2 = CTest::CreateSharedPtr(XUtils::Parameter{std::ref(a2)}, XUtils::Parameter{std::move(aa), 2});

        delete CTest::Create({}, {});

        std::unique_ptr<CTest> a{CTest::Create({}, XUtils::Parameter{}) };

        delete CTest::Create({}, XUtils::Parameter{200});

        int a3{300};
        delete CTest::Create(XUtils::Parameter{std::ref(a3)}, {});

        return;
    }
    std::cerr << "\n\n";
#endif

    //AAA::UniqueConstruction(XUtils::Parameter{1});

    // AAA::UniqueConstruction(XUtils::Parameter{1},{})->p();
    // AAA::UniqueConstruction(XUtils::Parameter{1},{})->p();
    // AAA::UniqueConstruction(XUtils::Parameter{1},{})->p();

    //delete p.get();
    // AAA::instance()->p();

    //auto a{AAA::instance()};

//    std::cerr << a.use_count() << std::endl;

//    std::cerr << a.use_count() << std::endl;
//    AAA::destroy();
//    AAA::destroy();
//    std::cerr << a.use_count() << std::endl;

    //XUtils::makeUnique<CTest>();

    // std::cerr << std::boolalpha << XUtils::is_private_mem_func<CTest,int>::value << std::endl;
    // std::cerr << XUtils::is_private_mem_func<CTest>::value << std::endl;
    //std::cerr << std::boolalpha << is_default_constructor_accessible<CTest>::value << std::endl;
    //std::cerr << std::boolalpha << is_default_constructor_accessible<CTest>::value << std::endl;

#if 0
    ATest obja;
    BTest objb;
    std::cerr << std::boolalpha << XUtils::XObject::connect(&obja,XUtils::xOverload<int>(&ATest::send),&obja,XUtils::xOverload<int>(&ATest::slot)) << "\n";
    std::cerr << std::boolalpha << XUtils::XObject::connect(&obja,XUtils::xOverload<int>(&ATest::send),&objb,&BTest::slot) << "\n";
    std::cerr << std::boolalpha << XUtils::XObject::connect(&obja,XUtils::xOverload<int>(&ATest::send),&objb,&BTest::slot) << "\n";
    std::cerr << std::boolalpha << XUtils::XObject::connect(&obja,XUtils::xOverload<int>(&ATest::send),&objb,[](const int &){}) << "\n";
    XUtils::XObject::disconnect(&obja, nullptr,&objb, nullptr);


    auto ff{XUtils::xOverload<int>(&ATest::send)};
    const auto signal_{reinterpret_cast<void**>(&ff)};
    void *args[]{signal_};
    //std::cerr << std::boolalpha << showaddr(&ATest::send,args) << std::endl;
    //MemberFunctionInfo a (&ATest::send);
    std::cerr << std::hash<void*>{}(*signal_) << std::endl;
    XUtils::sleep_for_s(3);
#endif

#if 0
    std::unordered_map<int,std::string> map;
    map[0] = "123";
    map[1] = "456";
    map[2] = "789";
    map.reserve(1);
    std::cerr << map.size() << std::endl;
    for (auto it{map.cbegin()};it != map.end();++it){
        std::cerr << it->second << std::endl;
    }
#endif
}

struct Data {
    int a,b;
//    explicit Data(){
//        std::cerr << FUNC_SIGNATURE << "\n";
//    }
//
//    Data(const Data &a ) = delete;
//
//    Data(Data &&) noexcept{
//        std::cerr << FUNC_SIGNATURE << "\n";
//    }
//
//    Data &operator=(const Data & ) = delete;
//
//    Data &operator=(Data && ) noexcept{
//        std::cerr << FUNC_SIGNATURE << "\n";
//        return *this;
//    }
//
//    ~Data(){
//        std::cerr << FUNC_SIGNATURE << "\n";
//    }

};

[[maybe_unused]] static void test7() {

    auto f0{[](Data  const &){

    }};

    auto f1{[&]<typename T,std::size_t ...I>(T && t,std::index_sequence<I...>){
        std::cerr << FUNC_SIGNATURE << " " << XUtils::typeName(t) << std::endl;
        f0( std::get<I>( std::forward<T> (t) )... );
    }};

    auto f2{[&]<typename ...A>(std::tuple<A...> && t){
        std::cerr << FUNC_SIGNATURE << " " << XUtils::typeName(t) << std::endl;
        f1(std::forward<decltype(t)>(t),std::make_index_sequence<sizeof...(A)>{});
    }};

    Data d;
    std::tuple t{std::move(d)};
    f2(std::move(t));
}

namespace detail {

// instance类的作用：定义了operator函数，提供到任意类型的隐式转换操作符，用于模拟构造Aggregate类型时所需的任意类型参数
    struct instance {
        template<typename Type>
        operator Type() const { return {};}
    };

    template <typename Aggregate, typename IndexSequence = std::index_sequence<>,
            typename = void>
    struct arity_impl : IndexSequence {};

// 特化版本
    template <typename Aggregate, std::size_t... Indices>
    struct arity_impl<Aggregate, std::index_sequence<Indices...>,
            std::void_t<decltype( Aggregate {
                    ( Indices,  (std::declval<instance>())  )...,
                    (std::declval<instance>()) } ) > >
            : arity_impl<Aggregate,
                    std::index_sequence<Indices..., sizeof...(Indices)>> {};

}  // namespace detail

template <typename T>
constexpr std::size_t arity() noexcept {
// 使用decay_t去除类型修饰（如const/volatile/引用）
    return detail::arity_impl<std::decay_t<T>>().size();
}

class A1 {
public:
    virtual void f1(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
    virtual void f2(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
};

class A2 {
    virtual void f1() {
        std::cerr << FUNC_SIGNATURE << "\n";
    }
};

struct A3 : public A1 , public A2 {
    virtual void f4()  {
        std::cerr << FUNC_SIGNATURE << "\n";
    }
    int a;
};

[[maybe_unused]] static void test8() {

    auto p0 = XUtils::makeUnique<int[]>(10);
    auto p1 = XUtils::makeShared<int[][2]>(10,{545,14512});
    auto p2 = XUtils::makeShared<std::vector<char>[512]>({1,2,3,4,5});
    auto p3 = XUtils::makeShared<int[10]>();
    auto p4 = XUtils::makeShared<int[]>(5);

    std::cerr << std::boolalpha
        << XUtils::Range(std::pair{1.0,3.0},XUtils::Range::Open,XUtils::Range::Open)(3.0)
        << std::endl;

    std::cerr << XUtils::typeName<std::decay_t<int[][1]>>() << std::endl;
    std::cerr << XUtils::calculate_total_elements<std::remove_extent_t<int[][2][10][3]> >() << std::endl;

    std::cerr << XUtils::typeName<std::vector<int>::const_pointer const >() << std::endl;
}

struct Test {
    friend void test9();
private:
    Test()
    { std::cerr << FUNC_SIGNATURE << "\n"; }
    ~Test()
    { std::cerr << FUNC_SIGNATURE << "\n"; }
};

[[maybe_unused]] void test9()
{
    std::allocator<Test> alloc{};
    auto p = alloc.allocate(1);
    new(p) Test{};
    p->~Test();
    alloc.deallocate(p,1);
}

void test10()
{
    constexpr std::string_view pattern { "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789" };

    std::string data {"1) %FIX, 2) %HACK, 3) %TODO"};

    std::cout << "替换前：" << data << '\n';

    constexpr std::string_view replacement = "%DONE%";

    for (std::string::size_type first{}, last{};
        (first = data.find('%', first)) != std::string::npos;
        first += replacement.size())
    {
        last = data.find_first_not_of(pattern, first + 1);
        if (last == std::string::npos)
            last = data.length();

        // 现在 first 位于 '%'，而 last 位于找到的子串的尾后位置
        data.replace(first, last - first, replacement);
    }

    std::cout << "替换后：" << data << '\n';
}

class A11 : public XUtils::XCOR<XUtils::Const,std::string>{
public:
    A11() = default;
};

class B11 : public XUtils::XCOR<XUtils::Const,std::string> {
    public:
    B11() = default;

    void responseHandler(Arguments const &) const override {
        std::cerr << FUNC_SIGNATURE << "\n";
    }

};

void test11()
{
    A11 a;
    B11 b;
    a.setNextResponse(&b);
    a.request(std::string{"fuck"});
}

int main(){
    //test1();
    //test2();
    //test3();
    //test4(123);
    //test5();
    //test6();
    //test7();
    //test8();
    //test9();
    //test10();
    test11();
    return 0;
}
