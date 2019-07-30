#ifndef stringUtilities_h
#define stringUtilities_h

#include "macros.h"

extern void ltrim(std::string &s);
extern void rtrim(std::string &s);
extern string replaceStrForever(string str, string from, string to);
extern string replaceStrOnce(string str, string from, string to);

extern string formatString(string str);
extern vector<string> tokenize(string str);

extern size_t actualStringDistance(string s);
extern vector<string> splitIntoCharsOfSize1(string str);
extern string setString(string inp, int pos, string posstr);

extern bool isFloat(string myString);
extern bool isInt(string myString);


template<typename T,typename enable_if< is_enum<T>::value >::type>
typename enable_if<is_enum<T>::value, ostream&>::type operator<<(ostream& os, const T & t);

template<typename T,typename enable_if< is_enum<T>::value >::type>
typename enable_if<is_enum<T>::value, istream&>::type operator>>(istream& is, T & t);

template<typename A, typename B> ostream& operator<<(ostream& os, const pair<A,B> & p);
template<typename A, typename B> istream& operator>>(istream& is, pair<A,B> & p);


template<typename T> ostream& operator<<(ostream& os, const vector<T> & v);

template<typename T> istream& operator>>(istream& is, vector<T> & v);

template<typename T> ostream& operator<<(ostream& os, const set<T> & s);
template<typename T> istream& operator>>(istream& is, set<T> & se);

template<typename A, typename B> ostream& operator<<(ostream& os, const map<A,B> & m);

template<typename A, typename B> istream& operator>>(istream& is, map<A,B> & m);


/*
template<typename T,typename enable_if< is_enum<T>::value >::type>
//typename enable_if<is_enum<T>::value, ostream&>::type
ostream& operator<<(ostream& os, const T & t) {
    os << static_cast<unsigned int>(t) << endl;
    return os;
}
*/
/*
template<typename T,typename enable_if< is_enum<T>::value >::type>
//typename enable_if<is_enum<T>::value, istream&>::type
istream& operator>>(istream& is, T & t) {
    unsigned int i; is >> i;
    t = static_cast<T>(i);
    return is;
}*/


template<typename A, typename B> ostream& operator<<(ostream& os, const pair<A,B> & p) {
    os << p.first << endl << p.second << endl;
    return os;
}

template<typename A, typename B> istream& operator>>(istream& is, pair<A,B> & p) {
    is >> p.first >> p.second;
    return is;
}

template<typename T> ostream& operator<<(ostream& os, const vector<T> & v) {
    os << v.size() << endl;
    for(const auto & c : v)
        os << c << endl;
    return os;
}

template<typename T> istream& operator>>(istream& is, vector<T> & v) {
    size_t s;
    is >> s;
    for(size_t i=0;i<s;++i) {
        T t;
        is >> t;
        v.push_back(t);
    }
    return is;
}

template<typename T> ostream& operator<<(ostream& os, const set<T> & s) {
    os << s.size() << endl;
    for(const auto & c : s)
        os << c << endl;
    return os;
}

template<typename T> istream& operator>>(istream& is, set<T> & se) {
    size_t s;
    is >> s;
    for(size_t i=0;i<s;++i) {
        T t;
        is >> t;
        se.insert(t);
    }
    return is;
}

template<typename A, typename B> ostream& operator<<(ostream& os, const map<A,B> & m) {
    os << m.size() << endl;
    for(const auto & p : m)
        os << p.first << endl << p.second << endl;
    return os;
}

template<typename A, typename B> istream& operator>>(istream& is, map<A,B> & m) {
    size_t s;
    is >> s;
    for(size_t i=0;i<s;++i) {
        A a; B b;
        is >> a;
        is >> b;
        m[a] = b;
    }
    return is;
}

#endif /* stringUtilities_h */
