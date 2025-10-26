#include <node.hpp>
#include <iostream>

Node::Node(std::string name)
    :m_name_(std::move(name))
{}

void Node::setName(std::string name)
{ m_name_.swap(name); }

void Node::addChild(NodePtr const & n)
{ m_children_.push_back(n); }

void Node::removeChild(NodePtr const & n)
{ m_children_.remove(n); }

void Node::printInfo(int const level) const noexcept {
    for (int i {}; i < level ;i++)
    { std::cout << '\t'; }

    std::cout << m_name_ << std::endl;

    for (auto const & child : m_children_)
    { child->printInfo(level + 1); }
}
