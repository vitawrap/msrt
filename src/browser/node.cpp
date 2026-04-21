#include "node.hpp"
#include <algorithm>

namespace ms {
namespace browser {

    void Node::appendChild(Node* node) {
        if (this == node->m_parent) return;
        if (node->m_parent)
            node->m_parent->removeChild(node);
        
        m_children.push_back(node);
        node->m_parent = this;
    }

    void Node::removeChild(Node* node) {
        if (node->m_parent == this) {

            m_children.erase(std::remove_if(m_children.begin(), m_children.end(), [&node](auto element){
                return node == element;
            }));
            node->m_parent = nullptr;
        }
    }

}
}
