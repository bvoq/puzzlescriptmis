#include "stringUtilities.h"

#include "rules.h"

// trim from start (in place)
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

string replaceStrForever(string str, string from, string to) {
    size_t start_pos;
    while((start_pos = str.find(from) ) != string::npos)
        str.replace(start_pos, from.size(), to);
    return str;
}

string replaceStrOnce(string str, string from, string to) {
    size_t start_pos;
    for(int i=0;i+from.size()<=str.size();++i) {
        if(str.substr(i,from.size()) == from) {
            str = str.substr(0,i) + to + str.substr(i+from.size());
            i += to.size()-1;
        }
    }
    
    return str;
}

string formatString(string str) {
    //Remove excess space and turn lines starting with = into empty lines.
    ltrim(str);
    rtrim(str);
    if(str.size() > 0 && str[0] == '=') str = "";
    
    //Lower-case everything
    for(char & c : str)
        if(c >= 'A' && c <= 'Z')
            c += 'a'-'A';
    
    return str;
}

//tokenizes on " " character
vector<string> tokenize(string str) {
    vector<string> tokens;
    string curstr = "";
    for(int i=0;i<=str.size();++i) {
        if(i == str.size() || isspace(str[i])) {
            if(curstr.size() > 0) {
                tokens.push_back(curstr);
                curstr.clear();
            }
        } else curstr.push_back(str[i]);
    }
    return tokens;
}



#include "utf8/checked.h"
#include "utf8/unchecked.h"
size_t actualStringDistance(string s) {
    return utf8::distance(s.begin(), s.end());
}

vector<string> splitIntoCharsOfSize1(string str) {
    vector<string> out;
    //actually handling utf8 correctly
    for(auto a = str.begin(), b = str.begin(); a!=str.end();) {
        utf8::advance (b, 1, str.end());
        string between = "";
        for(auto c = a; c != b; c++) between.push_back(*c);
        utf8::advance (a, 1, str.end());
        out.push_back(between);
    }
    return out;
}

string setString(string str, int pos, string symbol) {
    string out = "";
    int poscounter = 0;
    for(auto a = str.begin(), b = str.begin(); a!=str.end();) {
        utf8::advance (b, 1, str.end());
        string between = "";
        for(auto c = a; c != b; c++) between.push_back(*c);
        utf8::advance (a, 1, str.end());
        
        if(poscounter == pos) out += symbol;
        else out += between;
        poscounter++;
    }
    return out;
}

//modified from https://stackoverflow.com/questions/447206/c-isfloat-function
bool isFloat( string str ) {
    if(str.empty()) return false;
    std::istringstream iss(str);
    float f;
    iss >> noskipws >> f; // noskipws considers leading whitespace invalid
    // Check the entire string was consumed and if either failbit or badbit is set
    return iss.eof() && !iss.fail();
}

//taken from https://stackoverflow.com/questions/4654636/how-to-determine-if-a-string-is-a-number-with-c
bool isInt( string s ) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

/*
template<typename T,typename enable_if< is_enum<T>::value >::type>
typename enable_if<is_enum<T>::value, ostream&>::type operator<<(ostream& os, const T & t) {
    os << static_cast<unsigned int>(t) << endl;
    return os;
}

template<typename T,typename enable_if< is_enum<T>::value >::type>
typename enable_if<is_enum<T>::value, istream&>::type operator>>(istream& is, T & t) {
    unsigned int i; is >> i;
    t = static_cast<T>(i);
    return is;
}

template<typename A, typename B> ostream& operator<<(ostream& os, const pair<A,B> & p) {
    os << p.first << endl << p.second << endl;
    return os;
}

template<typename A, typename B> istream& operator<<(istream& is, pair<A,B> & p) {
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
        os << s << endl;
    return os;
}

template<typename T> istream& operator<<(istream& is, set<T> & se) {
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
*/
