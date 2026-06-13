#pragma once
#include <windows.h>

template <int N>
struct XorString {
    char data[N];
    constexpr XorString(const char(&str)[N]) : data{} {
        for (int i = 0; i < N; ++i)
            data[i] = str[i] ^ (0xC5 + i);
    }
    void decrypt() {
        for (int i = 0; i < N; ++i)
            data[i] = data[i] ^ (0xC5 + i);
    }
    const char* get() { decrypt(); return data; }
};

#define XS(str) [&](){ static XorString<sizeof(str)> _xs(str); return _xs.get(); }()

// Wide version
template <int N>
struct XorWString {
    wchar_t data[N];
    constexpr XorWString(const wchar_t(&str)[N]) : data{} {
        for (int i = 0; i < N; ++i)
            data[i] = str[i] ^ (0x9A + i);
    }
    void decrypt() {
        for (int i = 0; i < N; ++i)
            data[i] = data[i] ^ (0x9A + i);
    }
    const wchar_t* get() { decrypt(); return data; }
};

#define XWS(str) [&](){ static XorWString<sizeof(str)/2> _xws(str); return _xws.get(); }()
