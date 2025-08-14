#include "main.hpp"
#include <future>
#include <iostream>
#include <XThreadPool/xthreadpool.hpp>
#if !defined(_WIN64) && !defined(_WIN32)
#include <XSignal/xsignal.hpp>
#endif
#include <list>
#include <deque>
#include <XHelper/xutility.hpp>
#include <XHelper/xtypetraits.hpp>
#include <XObject/xobject.hpp>
#include <XHelper/xoverload.hpp>
#include <utility>
#include <chrono>
#include <XHelper/xtypetraits.hpp>

static std::mutex mtx{};

static constexpr auto wait_time{2};

struct Functor {
    auto operator()(const int id) const{
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << " id = " << id << "\n";
            }
            xtd::sleep_for_s(wait_time);
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
            xtd::sleep_for_s(wait_time);
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
            xtd::sleep_for_s(wait_time);
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
        xtd::sleep_for_s(wait_time);
    }
    return f + 100.0;
}

class A final : public xtd::XRunnable<xtd::NonConst> {
    std::any run() override {
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << FUNC_SIGNATURE << " id = " << m_id_ << "\n";
                m_id_++;
            }
            xtd::sleep_for_s(wait_time);
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
#if !defined(_WIN64) && !defined(_WIN64)
    const auto sigterm{xtd::Signal_Register(SIGTERM,{},[&]{
        exit_ = true;
    })};

    const auto sigint{xtd::Signal_Register(SIGINT,{},[&]{
        exit_ = true;
    })};

    const auto sigkill {xtd::Signal_Register(SIGKILL,{},[&]{
        exit_ = true;
    })};
#endif
    const auto pool2{xtd::XThreadPool::create(xtd::XThreadPool::Mode::CACHE)},
                pool3{xtd::XThreadPool::create(xtd::XThreadPool::Mode::CACHE)};
#if 1
    //pool2->setMode(xtd::XThreadPool2::Mode::FIXED);
    pool2->setThreadTimeout(70);
    //pool2->start();

    for (int i{};i < 30;++i){
       std::make_shared<A>(i)->joinThreadPool(pool2);
    }

    //pool2->stop();

    Functor3 f3{.m_name = "test"};

    const auto p1{pool2->runnableJoin(&Functor3::func, std::addressof(f3), "34")} ,
            p2{pool2->runnableJoin(Double,35.0)};

    xtd::XAbstractRunnable_Ptr lambda{};
    lambda = pool2->runnableJoin([&](const int& id){
        pool2->stop();
        pool2->start();
        pool2->runnableJoin(lambda);
        std::cerr << "p1->result<std::string>(): " << p1->result<std::string>(xtd::XAbstractRunnable::Model::NONBLOCK) << "\n" << std::flush;
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
    //pool2->setMode(xtd::XThreadPool2::Mode::FIXED);
    xtd::XAbstractTask2_Ptr task1{},task2{};
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
            xtd::sleep_for_s(wait_time);
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
        xtd::sleep_for_s(1);
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
            xtd::sleep_for_s(10);
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

    std::deque<xtd::XAbstractRunnable_Ptr> tasks1,task2s{};
    for (int i{};i < 1000000;++i){
        tasks1.push_back(std::make_shared<A>(10));
    }

    auto last_time{system_clock::now()};
    for (const auto &task : tasks1){
        task2s.push_back(task);
    }
    auto runtime{duration_cast<milliseconds>(system_clock::now() - last_time).count()};
    std::cerr <<  "deque w:" << runtime << std::endl;

    std::unordered_map<void*,xtd::XAbstractRunnable_Ptr> tasks2{};
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
    using namespace xtd;
    using List_t = XPrivate::List<int8_t,uint8_t,int16_t,uint16_t,int32_t,uint32_t>;
    using Value = XPrivate::List_Left<List_t,6>::Value;
    std::cout << xtd::typeName<Value>() << std::endl;
    std::string filename{"IMAGE01.PNG"};
    filename = xtd::toLower(std::move(filename));
    std::cerr << filename << std::endl;
    std::cerr << filename.substr(filename.find('.')) << std::endl;
}

[[maybe_unused]]
static void test5(){

    std::cerr << sizeof(std::size_t) << std::endl;

    std::tuple t{0,1l,2ll,3ul,4ull,0.1f,0.2,"const char *",std::string{"std::string"}};

    std::cerr << std::boolalpha << xtd::is_tuple_v<decltype(t)> << std::endl;
    std::cerr << std::boolalpha << xtd::is_tuple_v<std::tuple<>> << std::endl;

    xtd::for_each_tuple(xtd::Left_Tuple<2>(t),[](std::size_t & index,const auto &i) {
        std::cerr << "index:" <<  index++ << std::endl;
        std::cerr << i << std::endl;
    });

    std::cerr << std::endl;

    xtd::for_each_tuple(xtd::SkipFront_Tuple<2>(t),[](std::size_t & ,const auto &i) {
        std::cerr << i << std::endl;
    });

    std::cerr << std::endl;

    xtd::for_each_tuple(xtd::Last_Tuple<2>(t),[](std::size_t & ,const auto &i) {
        std::cerr << i << std::endl;
    });
}

class ATest:public xtd::XObject {
public:
    void send(int d) && noexcept {

    }

    void slot(int) const volatile & noexcept {

    }
};

class BTest:public xtd::XObject {

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

class CTest final : public xtd::XHelperClass<CTest> {
    X_HELPER_CLASS

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
    // CTest() noexcept {
    //     std::cerr << FUNC_SIGNATURE << std::endl;
    // }

    ~CTest(){
        delete new int[10];
        std::cerr << FUNC_SIGNATURE << std::endl;
    }
};

class AAA final : public xtd::XSingleton<AAA> {
    X_SINGLETON_CLASS
    int aa{100};
public:
    void p(){
        using namespace std::chrono_literals;
        std::cerr << FUNC_SIGNATURE << aa <<  "\n";
    }
protected:
    AAA()
    {
        std::cerr << FUNC_SIGNATURE << "\n";
    }
    ~AAA(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
    bool construct_(){return true;}
};

[[maybe_unused]] static void test6(){
    std::cerr << std::boolalpha << std::is_constructible_v<AAA> << std::endl;

#if 1
    int a1{1},a2{2};
    std::string aa{"2"};
    auto p1 = CTest::CreateUniquePtr(xtd::Parameter{std::ref(a1)},xtd::Parameter{100});
    auto p2 = CTest::CreateSharedPtr(xtd::Parameter{std::ref(a2)},xtd::Parameter{std::move(aa),2});
    delete CTest::Create({},{});

    std::unique_ptr<CTest> a {CTest::Create({},xtd::Parameter{})};

    delete CTest::Create({},xtd::Parameter{200});

    int a3{300};
    delete CTest::Create(xtd::Parameter{std::ref(a3)},{});

#endif

     auto p{std::move(AAA::UniqueConstruction())};
    p->p();

    //delete p.get();
    // AAA::instance()->p();

    //auto a{AAA::instance()};

//    std::cerr << a.use_count() << std::endl;

//    std::cerr << a.use_count() << std::endl;
//    AAA::destroy();
//    AAA::destroy();
//    std::cerr << a.use_count() << std::endl;

    //xtd::makeUnique<CTest>();

    // std::cerr << std::boolalpha << xtd::is_private_mem_func<CTest,int>::value << std::endl;
    // std::cerr << xtd::is_private_mem_func<CTest>::value << std::endl;
    //std::cerr << std::boolalpha << is_default_constructor_accessible<CTest>::value << std::endl;
    //std::cerr << std::boolalpha << is_default_constructor_accessible<CTest>::value << std::endl;

#if 0
    ATest obja;
    BTest objb;
    std::cerr << std::boolalpha << xtd::XObject::connect(&obja,xtd::xOverload<int>(&ATest::send),&obja,xtd::xOverload<int>(&ATest::slot)) << "\n";
    std::cerr << std::boolalpha << xtd::XObject::connect(&obja,xtd::xOverload<int>(&ATest::send),&objb,&BTest::slot) << "\n";
    std::cerr << std::boolalpha << xtd::XObject::connect(&obja,xtd::xOverload<int>(&ATest::send),&objb,&BTest::slot) << "\n";
    std::cerr << std::boolalpha << xtd::XObject::connect(&obja,xtd::xOverload<int>(&ATest::send),&objb,[](const int &){}) << "\n";
    xtd::XObject::disconnect(&obja, nullptr,&objb, nullptr);


    auto ff{xtd::xOverload<int>(&ATest::send)};
    const auto signal_{reinterpret_cast<void**>(&ff)};
    void *args[]{signal_};
    //std::cerr << std::boolalpha << showaddr(&ATest::send,args) << std::endl;
    //MemberFunctionInfo a (&ATest::send);
    std::cerr << std::hash<void*>{}(*signal_) << std::endl;
    xtd::sleep_for_s(3);
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
    explicit Data(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }

    Data(const Data &a ) = delete;

    Data(Data &&) noexcept{
        std::cerr << FUNC_SIGNATURE << "\n";
    }

    Data &operator=(const Data & ) = delete;

    Data &operator=(Data && ) noexcept{
        std::cerr << FUNC_SIGNATURE << "\n";
        return *this;
    }

    ~Data(){
        std::cerr << FUNC_SIGNATURE << "\n";
    }
};



[[maybe_unused]] static void test7() {

    auto f0{[](Data  const &d){

    }};

    auto f1{[&]<typename T,std::size_t ...I>(T && t,std::index_sequence<I...>){
        std::cerr << FUNC_SIGNATURE << " " << xtd::typeName(t) << std::endl;
        f0( std::get<I>( std::forward<T> (t) )... );
    }};

    auto f2{[&]<typename ...A>(std::tuple<A...> && t){
        std::cerr << FUNC_SIGNATURE << " " << xtd::typeName(t) << std::endl;
        f1(std::forward<decltype(t)>(t),std::make_index_sequence<sizeof...(A)>{});
    }};

    Data d;
    std::tuple t{std::move(d)};
    f2(std::move(t));
}

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
    //test1();
    //test2();
    //test3();
    //test4(123);
    //test5();
    //test6();
    test7();
    return 0;
}
