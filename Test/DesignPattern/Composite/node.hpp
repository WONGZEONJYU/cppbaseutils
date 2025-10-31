#ifndef NODE_HPP
#define NODE_HPP 1

#include <string>
#include <memory>
#include <list>

class Node;
using NodePtr = std::shared_ptr<Node>;

class Node {
    std::string m_name_{};
    std::list<NodePtr> m_children_{};

public:
    constexpr Node() = default;

    explicit Node(std::string );

    void setName( std::string );

    void addChild(NodePtr const &);

    void removeChild(NodePtr const &);

    void printInfo(int level) const noexcept;

    virtual ~Node() = default;
};

#endif
