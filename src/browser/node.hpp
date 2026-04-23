#pragma once

#include "core/events.hpp"
#include "script_host.hpp"
#include <unordered_map>
#include <vector>

namespace ms {
namespace browser {

    class Node {
    public:
        typedef std::vector<Node*> NodeList;
        std::unordered_map<std::string, EventAny*> m_eventMap;

    private:
        Node* m_parent;
        NodeList m_children;

    protected:
        /** Add all events to the event map */
        virtual void mapEvents() {}
    
    public:
        Node() :
            m_parent(nullptr)
        {}
        virtual ~Node() {
            if (m_parent)
                m_parent->removeChild(this);
        } // children are GC collected

        bool appendChild(Node* node);
        bool removeChild(Node* node);

        bool hasChildNodes() const { return m_children.size(); }
        Node* getParent() const { return m_parent; }

        // EventTarget


        // STL interface
        const NodeList::const_iterator begin() const { return m_children.cbegin(); }
        const NodeList::const_iterator end() const { return m_children.cend(); }
        const NodeList::iterator begin() { return m_children.begin(); }
        const NodeList::iterator end() { return m_children.end(); }
    };

    /**
     * @brief Simulated HTML element
     * 
     */
    class HTMLElement : public Node {

    };

}
}
