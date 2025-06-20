#include <future>
#include <iostream>
#include <XThreadPool/xthreadpool2.hpp>
#include <XSignal/xsignal.hpp>
#include <semaphore>

static std::mutex mtx{};

static constexpr auto wait_time{2};

struct Functor {
    auto operator()(const int id) const{
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << __PRETTY_FUNCTION__ << " id = " << id << "\n";
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
                std::cout << __PRETTY_FUNCTION__ << " id = " << 33 << "\n";
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
                std::cout << __PRETTY_FUNCTION__ << " name = " << name << "\n";
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
            std::cout << __PRETTY_FUNCTION__ << " f = " << f << "\n";
        }
        xtd::sleep_for_s(wait_time);
    }
    return f + 100.0;
}

class A final : public xtd::XAbstractTask2 {
    std::any run() override {
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << __PRETTY_FUNCTION__ << " id = " << m_id_ << "\n";
            }
            xtd::sleep_for_s(wait_time);
        }
        return std::string(__PRETTY_FUNCTION__) + "end";
    }
    int m_id_{};
public:
    explicit A(const int id):m_id_{id}{};
};

[[maybe_unused]] static inline void test1() {
    bool exit_{};

    const auto sigterm{xtd::Signal_Register(SIGTERM,{},[&]{
        exit_ = true;
    })};

    const auto sigint{xtd::Signal_Register(SIGINT,{},[&]{
        exit_ = true;
    })};

    const auto sigkill {xtd::Signal_Register(SIGKILL,{},[&]{
        exit_ = true;
    })};

    const auto pool2{xtd::XThreadPool2::create(xtd::XThreadPool2::Mode::CACHE)};
#if 0
    //pool2->setMode(xtd::XThreadPool2::Mode::FIXED);
    //pool2->start();
    for (int i{};i < 30;++i){
       std::make_shared<A>(i)->joinThreadPool(pool2);
    }

    //std::this_thread::sleep_for(std::chrono::seconds(10));

    Functor3 f3{
            .m_name = "test"
    };

    const auto p1{pool2->tempTaskJoin(&Functor3::func, std::addressof(f3), "34")} ,
            p2{pool2->tempTaskJoin(Double,35.0)};

    const auto r{pool2->tempTaskJoin([&](const int& id){
        pool2->stop();
        pool2->start();
        std::cerr << "p1->result<std::string>(): " << p1->result<std::string>() << "\n" << std::flush;
        for (int i {}; i < 3;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << __PRETTY_FUNCTION__ << "lambda id = " << id << "\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
        return id + 10;
    },31)};

    pool2->tempTaskJoin(Functor(),32);
    pool2->tempTaskJoin(Functor2());

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

     //std::cout << p1->result<std::string>() << "\n";
    //std::cout << p1->result<std::string>() << "\n"; //会再次被阻塞,是一个bug
     std::cout << p2->result<double>() << '\n'; //与上同理
#else
    //pool2->setMode(xtd::XThreadPool2::Mode::FIXED);
    decltype(pool2->taskJoin({})) task1{},task2{};
    task1 = pool2->tempTaskJoin([&](const auto &data_){
        std::cerr << "task1->result<int>(task2->NonblockModel): " <<
            task1->result<int>(std::chrono::seconds(5)) << "\n" << std::flush;
        pool2->stop();
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

[[maybe_unused]] static inline void test2(){

    // std::unordered_map<int,std::string> m_map{{1,"fuck1"},{2,"fuck2"},{3,"fuck3"},};
    // m_map.insert({3,"fuck3"});
    //
    // for (const auto &[key,value]:m_map){
    //     std::cerr << "key: " << key << " value: " << value << "\n";
    // }
    bool exit_{};
    std::atomic_bool b{};
    std::thread{[&]{
        while (!exit_){
            xtd::sleep_for_s(10);
            b.wait({},std::memory_order_acquire);
            std::cerr << __PRETTY_FUNCTION__ << "\n";
            b.store({},std::memory_order_release);
        }
    }}.detach();

    while (true){
        int v{};
        std::cin >> v;
        if (v < 0){
            break;
        }
        std::binary_semaphore bin{0};
        b.store(true,std::memory_order_release);
        b.notify_all();
    }
    exit_ = true;
}

[[maybe_unused]] static void test3(){

}

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
    test1();
    //test2();
    //test3();

    return 0;
}
