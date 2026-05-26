#pragma once

#include <stack>
#include <vector>

namespace ms {

    /**
     * @brief Iterable stack
     * Default stack data structure, but with access to the container 
     */
    template <typename T, typename Container = std::vector<T> >
    class stack : public std::stack<T, Container> {
    public:
        stack() : std::stack<T, Container>() {}
        ~stack() {}
    
        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;
    
        iterator begin() { return this->c.begin(); }
        iterator end() { return this->c.end(); }
    
        iterator cbegin() { return this->c.cbegin(); }
        iterator cend() { return this->c.cend(); }
    
        Container& container() { return this->c; }
        Container const& container() const { return this->c; }
    };

}
