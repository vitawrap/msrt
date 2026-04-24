#include "node.hpp"
#include <algorithm>

namespace ms {
namespace browser {

    bool Node::appendChild(Node* node) {
        if (this == node->m_parent) return true;
        // check for possible cycle
        for (auto* walk = m_parent; walk; walk = walk->m_parent) {
            if (walk == node) {
                return false;
            }
        }

        if (node->m_parent)
            node->m_parent->removeChild(node);
        
        m_children.push_back(node);
        node->m_parent = this;
        return true;
    }

    bool Node::removeChild(Node* node) {
        if (node->m_parent == this) {

            m_children.erase(std::remove_if(m_children.begin(), m_children.end(), [&node](auto element){
                return node == element;
            }));
            node->m_parent = nullptr;
            return true;
        }
        return false;
    }

    EventAny* Node::findEvent(char const* name) {
        if (m_eventMap.empty())
            mapEvents();
        auto itr = m_eventMap.find(name);
        if (itr != m_eventMap.end())
            return itr->second;
        return nullptr;
    }

}
}
