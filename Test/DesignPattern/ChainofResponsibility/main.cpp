#include <iostream>
#include <frame.hpp>
#include <processnode.hpp>

class MeiyanNode : public ProcessNode
{
public:
    std::shared_ptr<Frame> pro(std::shared_ptr<Frame> const & ) override {
        auto frame { std::make_shared<Frame>() };
        printf("美颜\n");
        return frame;
    }
};

class DaojuNode : public ProcessNode
{
public:
    std::shared_ptr<Frame> pro(std::shared_ptr<Frame> const & ) override {
        auto frame { std::make_shared<Frame>() };
        printf("道具\n");
        return frame;
    }
};

class LvjingNode : public ProcessNode
{
public:
    std::shared_ptr<Frame> pro(std::shared_ptr<Frame> const & ) override {
        auto frame { std::make_shared<Frame>() };
        printf("滤镜\n");
        return frame;
    }
};

int main() {
    // 从相机拿到数据
    auto const frame { std::make_shared<Frame>() };

    auto const meiyan { std::make_shared<MeiyanNode>() };
    auto const daoju { std::make_shared<DaojuNode>() };
    auto const lvjing { std::make_shared<LvjingNode>() };

    meiyan->setNext(lvjing);
    lvjing->setNext(daoju);

    auto const ret{ meiyan->process(frame) };
    return 0;
}
