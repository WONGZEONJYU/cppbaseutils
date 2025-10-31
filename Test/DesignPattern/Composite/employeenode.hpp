#ifndef EMPLOYEENODE_HPP
#define EMPLOYEE_NODE_HPP 1

#include <node.hpp>

class EmployeeNode : public Node {

public:
    constexpr EmployeeNode() = default;
    explicit EmployeeNode(std::string name);
};

#endif
