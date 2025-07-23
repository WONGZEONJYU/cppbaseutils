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
#if defined(MACOS) || defined(LINUX)
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
        std::cerr << "p1->result<std::string>(): " << p1->result<std::string>(p1->NonblockModel) << "\n" << std::flush;
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

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
    //test1();
    //test2();
    //test3();
    //test4(123);
    test5();
    return 0;
}
