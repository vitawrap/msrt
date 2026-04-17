#pragma once

#include <stdexcept>
#include <string>
#include <math.h>
#include "cjson/cJSON.h"

namespace ms {
namespace io {

/**
 * Runtime error wrapper for message
 */
class CJSONError : public std::runtime_error {
public:
    CJSONError(char const* err)
        : std::runtime_error(err)
    {}
};

/**
 * JSON tree visitor, wraps over cJSON (convenient naming).
 * This tree is invalidated if cJSON_Delete is called on the hierarchy.
 */
class CJSONNode {
    cJSON* m_node;

public:
    CJSONNode()
        : m_node(nullptr) {}

    CJSONNode(cJSON* node)
        : m_node(node) {}

    /**
     * Assuming object, index it, throws CJSONError upon failure.
     */
    CJSONNode operator[] (char const* key) const noexcept(false) {
        cJSON* node = cJSON_GetObjectItemCaseSensitive(m_node, key);
        if (!node)
            throw CJSONError{(std::string{"failed for key: "} + key).c_str()};

        return CJSONNode{node};
    }

    /**
     * Assuming array, index it, throws CJSONError upon failure.
     */
    CJSONNode operator[] (int item) const noexcept(false) {
        cJSON* node = cJSON_GetArrayItem(m_node, item);
        if (!node)
            throw CJSONError{(std::string{"failed for index: "} + std::to_string(item)).c_str()};

        return CJSONNode{node};
    }

    /**
     * Get string value of this node.
     */
    operator char*() const noexcept(false) {
        char* str = cJSON_GetStringValue(m_node);
        if (!str)
            throw CJSONError{"failed to get string value"};

        return str; 
    }

    /**
     * Get number value of this node.
     */
    operator double() const noexcept(false) {
        double number = cJSON_GetNumberValue(m_node);
        if (number == NAN)
            throw CJSONError{"failed to get number value"};
        
        return number;
    }

    /**
     * Get boolean value of this node.
     */
    operator bool() const {
        return cJSON_IsTrue(m_node);
    }

    /**
     * Get array size of this node.
     * (My silly little cJSON edit returns -1 for non-arrays, not the case by default)
     */
    int size() const noexcept(false) {
        int size = cJSON_GetArraySize(m_node);
        if (size == -1)
            throw CJSONError{"not an array"};
        return size;
    }

    /**
     * If this item is defined as a key of the parent object,
     * this is the name of that key.
     */
    char const* name() const {
        return m_node->string;
    }

    /**
     * Test if the node is valid.
     */
    bool valid() const {
        return m_node;
    }

    /**
     * Same as "valid" but with actual internal node address.
     */
    cJSON* ptr() const {
        return m_node;
    }

    /**
     * Make this compatible with cJSON macros.
     */
    cJSON* operator-> () {
        return m_node;
    }

    /**
     * Another operator for cJSON macro compatibility
     */
    CJSONNode& operator= (cJSON* node) {
        m_node = node;
        return * this;
    }
};

/**
 * CJSON RAII wrapper. Again, convenient class name.
 */
class CJSON {
    cJSON* m_root;

public:
    CJSON()
        : m_root(nullptr)
    {}

    /**
     * Parse JSON text, this object holds the root, and frees it.
     */
    CJSONNode parse(char const* text) {
        m_root = cJSON_Parse(text);
        if (!m_root)
            throw CJSONError{"invalid JSON file"};

        return CJSONNode{m_root};
    }

    ~CJSON() {
        if (m_root)
            cJSON_Delete(m_root);
    }
};

// Re-define this for the sake of consistency and opacity.
#define CJSONNode_forEach(element, array) \
for(element=(array.ptr()!=nullptr)?(array)->child:nullptr;\
    element.ptr()!=nullptr; element=element->next)

}
}
