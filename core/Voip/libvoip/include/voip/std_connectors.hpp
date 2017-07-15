#pragma once
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

// cross-DLL std types connectors
// Idea: in DLL:  std::string    dll_std_string_imp("abc");    
//                String         proxy(dll_std_string_imp);
//                                |
//                                v
//       in App:  std::string    app_std_string_imp(proxy);
//       now app can safely delete |proxy|
// and vice versa


namespace voip {

class ExtAllocator {
protected:
    VOIP_EXPORT char  *Alloc(size_t size);
    VOIP_EXPORT void   Free(char *ptr);
};
    
class   String : public ExtAllocator 
{

public:
    String() { reset(); }

    String(const char *str) { reset(); assign(str); }

    String(const std::string &std_str) { reset(); assign(std_str); }

    String(const String &str) { reset(); *this = str; }

    inline ~String()   { Free(_data); }

    // assigments
    void assign(const char *str) { assign(std::string(str)); }
    void assign(const char *str, size_t len) { 
        Free(_data);
        _data = NULL;
        _length = len;
        if( _length > 0 ) {
            _data = Alloc(_length+1);
            memcpy(_data, str, _length+1);
            _data[_length] = 0;
        }
    }
    void assign(const std::string &std_str) { assign(std_str.c_str(), std_str.length());  }
    

    String& operator =(const char* rhs)             { assign(rhs); return *this; }
    String& operator =(const std::string &rhs)      { assign(rhs); return *this; }
    String& operator =(const String& rhs)           { assign(rhs._data, rhs._length); return *this; }

    // mimic some std::string functionality
    inline size_t      length()  const  { return _length; }
    inline const char *c_str()   const  { return _length>0 ? _data : ""; }

    //implicit std::string conversion
    operator std::string() const { return std_string(); }

    std::string  std_string() const {
        if( _length > 0 )
            return std::string(_data, _length);
        else
            return std::string();
    }

private:
    inline void reset() { _data = NULL; _length=0; }



    char    *_data;
    size_t  _length;
};


template <typename T>
class  Vector : public ExtAllocator 
{

public:
    Vector() { reset(); };

    Vector(const T *buf, size_t buf_size) { reset(); assign(buf, buf_size); }

    Vector(const std::vector<T> &std_vec) { reset(); assign(std_vec); }

	Vector(const Vector<T> &vec) { reset(); *this = vec; }

    inline ~Vector()   { 
        assign(NULL, 0);
    }

    // assigments
    void assign(const T *buf, size_t buf_size) {
        if( _size > 0 ) {
            T *el = (T*)_data;
            while(_size--) {
                el->~T();
                el++;
            }
        }
        Free(_data);
        _data = NULL;
        _size = buf_size;
        if( _size > 0) {
            _data = Alloc(_size * sizeof(T));
            T *dest = (T*)_data;
            for(size_t i=0; i < _size; i++) 
                 new(dest + i) T(*(buf + i));       // placement new! whoaa!
        }
    }

    void assign(const std::vector<T> &std_vec) {  assign(std_vec.size()>0 ? &std_vec[0] : NULL, std_vec.size()); }

    Vector& operator =(const std::vector<T> &rhs)      { assign(rhs); return *this; }
    Vector& operator =(const Vector& rhs)              { assign((T*)rhs._data, rhs._size); return *this; }

    // mimic some std::vector functionality
    inline size_t      size()                   const  { return _size; }
    inline const T&    operator[](size_t idx)   const  { return ((const T*)_data)[idx]; }   // note: no checks
    inline       T&    operator[](size_t idx)          { return ((      T*)_data)[idx]; }   // note: no checks

    //implicit std::vector conversion
    operator std::vector<T>() const { return std_vector(); }

    std::vector<T>  std_vector() const {
        if( _size > 0 )
            return std::vector<T>((const T*)_data, ((const T*)_data) +_size);
        else
            return std::vector<T>();
    }

private:
    inline void reset() { _data = NULL; _size=0; }

     char       *_data;
     size_t      _size;
};



} // namespace voip