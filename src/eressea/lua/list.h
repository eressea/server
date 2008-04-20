#ifndef LUA_LIST_H
#define LUA_LIST_H

#include <string>
#include <kernel/item.h>

namespace eressea {

  template<class T, class N = T>
  class listnode {
  public:
    static N next(N& node) { return node->next; }
    static T value(N& node) { return node; }
  };

  template<class T, class N = T, class nodetype = listnode<T, N> >
  class list {
  public:
    class iterator {
    public:
      iterator(const N& index) : m_index(index) {}
      T operator*() { return nodetype::value(m_index); }
      bool operator==(const iterator& iter) {
        return iter.m_index==m_index;
      }
      bool operator!=(const iterator& iter) {
        return iter.m_index!=m_index;
      }
      iterator& operator++() {
        if (m_index) m_index = nodetype::next(m_index);
        return *this;
      }
    private:
      N m_index;
    };
    typedef iterator const_iterator;
    list<T, N, nodetype>(const N& clist) : m_clist(clist) {}
    iterator begin() const { return iterator(m_clist); }
    iterator end() const { return iterator(NULL); }

  public:
    N m_clist;
  };

  class bind_items {
  public:
    static item * next(item * node) { return node->next; }
    static std::string value(item * node) { 
      return std::string(node->type->rtype->_name[0]);
    }
  };

};

#endif
