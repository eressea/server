#ifndef BINDINGS_H
#define BINDINGS_H

struct lua_State;

extern void bind_region(struct lua_State * L);
extern void bind_unit(struct lua_State * L);
extern void bind_ship(struct lua_State * L);
extern void bind_building(struct lua_State * L);

namespace eressea {
  template<class T>
  class list {
  public:
    class iterator {
    public:
      iterator(T * index) : m_index(index) {}
      T * operator*() { return m_index; }
      bool operator==(const iterator& iter) {
        return iter.m_index==m_index;
      }
      iterator& operator++() {
        if (m_index) m_index = m_index->next;
        return *this;
      }
    private:
      T * m_index;
    };
    typedef iterator const_iterator;
    list<T>(T * clist) : m_clist(clist) {}
    iterator begin() const { return iterator(m_clist); }
    iterator end() const { return iterator(NULL); }

  public:
    T * m_clist;
  };
};

#endif
