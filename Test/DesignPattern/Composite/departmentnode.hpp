#ifndef DEPARTMENT_NODE_HPP
#define DEPARTMENT_NODE_HPP 1

#include <node.hpp>

class DepartmentNode : public Node {

public:
    constexpr DepartmentNode() = default;
    explicit DepartmentNode(std::string name);
};

#endif
