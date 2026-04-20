#pragma once

#include "script_host.hpp"
#include <vector>

namespace ms {
namespace browser {

    class Node {
    public:
        typedef std::vector<Node*> NodeList;

    private:
        NodeList m_children;
    
    public:

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
