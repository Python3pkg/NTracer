#ifndef var_geometry_hpp
#define var_geometry_hpp

#include <Python.h>
#include <algorithm>

#include "geometry.hpp"
#include "pyobject.hpp"


namespace var {    
    /* an array that can be initialized with a call-back */
    template<typename T> struct init_array {
        size_t _size;
        void *data;
        
        T *begin() { return reinterpret_cast<T*>(data); }
        const T *begin() const { return reinterpret_cast<const T*>(data); }
        
        T *end() { return begin() + _size; }
        const T *end() const { return begin() + _size; }
        
        T &front() { return begin()[0]; }
        const T &front() const { return begin()[0]; }
        
        T &back() { return begin()[_size-1]; }
        const T &back() const { return begin()[_size-1]; }
        
        T &operator[](int i) { return begin()[i]; }
        const T &operator[](int i) const { return begin()[i]; }
        
        operator T*() { return begin(); }
        operator const T*() const { return begin(); }
        
        size_t size() const { return _size; }
        
        template<typename F> init_array(size_t _size,F f) : _size(_size) {
            assert(_size);
            
            data = operator new(sizeof(T) * _size);
            
            size_t i=0;
            try {
                for(; i<_size; ++i) new(&(*this)[i]) T(f(i));
            } catch(...) {
                while(i) (*this)[--i].~T();
                throw;
            }
        }
        
        init_array(const init_array&) = delete;
        init_array &operator=(const init_array&) = delete;
        
        ~init_array() {
            for(size_t i=0; i<_size; ++i) (*this)[i].~T();
            operator delete(data);
        }
    };
    
    template<typename RealItems,typename T> struct item_array {
        T *items;
        
        explicit item_array(int d) : size(d) {
            allocate();
        }
        
        item_array(const item_array &b) : size(b.size) {
            allocate();
            memcpy(items,b.items,RealItems::get(size) * sizeof(T));
        }
        
        item_array(item_array &&b) : items(b.items), size(b.size) {
            b.items = nullptr;
        }
        
        ~item_array() {
            if(simd::v_sizes<T>::value[0] == 1) free(items);
            else simd::aligned_free(items);
        }
            
        item_array &operator=(const item_array &b) {
            free(items);
            size = b.size;
            allocate();
            memcpy(items,b.items,RealItems::get(size) * sizeof(T));
            
            return *this;
        }
        
        item_array &operator=(item_array &&b) noexcept {
            free(items);
            size = b.size;
            items = b.items;
            b.items = nullptr;
            return *this;
        }
        
        int dimension() const {
            return size;
        }
        
    private:
        void allocate() {
            assert(size > 0);

            if(simd::v_sizes<T>::value[0] == 1) {
                if(!(items = reinterpret_cast<T*>(malloc(RealItems::get(size) * sizeof(T))))) throw std::bad_alloc();
            } else {
                items = reinterpret_cast<T*>(simd::aligned_alloc(
                    simd::largest_fit<T>(size) * sizeof(T),
                    RealItems::get(size) * sizeof(T)));
            }
        }
        
        int size;
    };

    template<typename T> struct item_store {
        typedef T item_t;
        
        static const int required_d = 0;
        
        template<typename U> using init_array = var::init_array<U>;
        template<typename U> using smaller_init_array = var::init_array<U>;

        template<typename RealItems,typename U=T> using type = item_array<RealItems,U>;
    };
}

#endif

