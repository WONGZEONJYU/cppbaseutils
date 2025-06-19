#include <iostream>
#include <XThreadPool/xthreadpool2.hpp>
#include <XSignal/xsignal.hpp>
#include <thread>


static std::mutex mtx{};

static constexpr auto wait_time{2};

struct Functor {
    auto operator()(const int id) const{
        for (int i {}; i < 3 ;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << __PRETTY_FUNCTION__ << " id = " << id << "\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
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
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
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
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
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
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
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
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
        return {};
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

    const auto pool2{xtd::XThreadPool2::create()};
    pool2->setMode(xtd::XThreadPool2::Mode::CACHE);

#if 1
    //pool2->start();
    for (int i{};i < 30;++i){
       std::make_shared<A>(i)->joinThreadPool(pool2);
       //pool2->taskJoin(std::make_shared<A>(i));
    }
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    //pool2->stop();

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
    decltype(pool2->taskJoin({})) task1{};

    task1 = pool2->tempTaskJoin([&](const auto &data_){
        //task1->joinThreadPool(pool2); //会反复套娃
        for (int i{};i < 3;++i){
            {
                std::unique_lock lock(mtx);
                std::cout << __PRETTY_FUNCTION__ << " data = " << data_ << "\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        }
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
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cerr << "task1 return: " << task1->result<int>() << "\n";
#endif
}

[[maybe_unused]] static inline void test2(){

    std::unordered_map<int,std::string> m_map{{1,"fuck1"},{2,"fuck2"},{3,"fuck3"},};
    m_map.insert({3,"fuck3"});

    for (const auto &[key,value]:m_map){
        std::cerr << "key: " << key << " value: " << value << "\n";
    }
}

int main(const int argc,const char **const argv){
    (void )argc,(void )argv;
    test1();
    //test2();

    return 0;
}
