#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <functional>
#include <Windows.h>
#include <Commdlg.h>
#include <iomanip>
#include <map>
#include <set>
#include <fstream>
#include <cassert>
#include <random>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include "cloak.h" // https://github.com/pfultz2/Cloak

namespace NF {
    using std::vector;
    using std::string, std::string_view, std::stringstream, std::to_string;
    using std::wstring, std::wstring_view, std::wstringstream, std::to_wstring;
    using std::function;
    using std::cout, std::endl;
    using std::pair;
    using std::shared_ptr, std::make_shared, std::dynamic_pointer_cast;
    using std::map;
    using std::set;
    using std::fstream, std::ifstream, std::ofstream, std::getline;
    using std::tuple;

    typedef bool Bit, u1;
    typedef uint8_t Byte, u8;
    typedef uint16_t Word, u16;
    typedef uint32_t DWord, u32;
    typedef uint64_t QWord, u64;

    struct u8x1 {
        u8x1() : byte(0) {}
        u8x1(Byte b) : byte(b) {}
        Bit bit(Byte i) { return ((i < 8) && ((byte & (1 << i)) >> i)) ? 1 : 0; }
        void SetBit(Byte i, Bit data) { (i < 8 && data) ? (byte |= (1 << i)) : (byte &= ~(1 << i)); }
        string ToString() {
            stringstream ss;
            Byte i = 8;
            while (i--) { ss << bit(i); if (i == 4) { ss << " "; } }
            return ss.str();
        }
        Byte byte;
    };

    union u8x2 {
        Byte byte[2];
        Word word;
    };

    union u8x4 {
        Byte byte[4];
        Word word[2];
        DWord dWord;
    };

    union u8x8 {
        Byte byte[8];
        Word word[4];
        DWord dWord[2];
        QWord qWord;
    };

#pragma pack(push, 1)
    /*
    CP56Time2a
    |_|_7|_6|_5|_4|_3|_2|_1|_0|
    |0|__________ms___________|
    |1|__________ms___________|
    |2|iv|**|_____minute______|
    |3|su|*****|_____hour_____|
    |4|__wday__|_____mday_____|
    |5|***********|____mon____|
    |6|**|________year________|
    */
    // undone~
    struct CP56Time2a {
        // byte 0~1
        u16 ms; // 0~59999
        // 2
        u8  minute : 6; // 0~59
        u8  _1 : 1;
        u8  iv : 1; // 0~1, 0: valid, 1: invalid
        // 3
        u8  hour : 5; // 0~23
        u8  _2 : 2;
        u8  su : 1; // 0~1, 0: standard time, 1: summer time
        // 4
        u8  mday : 5; // 1~31
        u8  wday : 3; // 1~7
        // 5
        u8  mon : 4; // 1~12
        u8  _3 : 4;
        // 6
        u8  year : 7; // 0~99, +2000
        u8  _4 : 1;
        CP56Time2a() { Clear(); }
        CP56Time2a(const vector<Byte>& bytes, size_t i = 0) {
            Clear();
            if (bytes.size() >= (7 + i)) { memcpy(this, &bytes[i], sizeof(CP56Time2a)); }
        }
        // TEST
        CP56Time2a(u8 _year, u8 _mon, u8 _mday, u8 _hour, u8 _min, u16 _ms) {
            Clear();
            iv = 0;
            year = _year;
            mon = _mon;
            mday = _mday;
            hour = _hour;
            minute = _min;
            ms = _ms;
        }
        void Clear() { memset(this, 0, sizeof(CP56Time2a)); iv = 1; /* invalid */ }
        // Calculate the day of the year
        constexpr u16 yday() const {
            constexpr u16 mdays[12] = {
                0,
                0 + 31,
                0 + 31 + 28,
                0 + 31 + 28 + 31,
                0 + 31 + 28 + 31 + 30,
                0 + 31 + 28 + 31 + 30 + 31,
                0 + 31 + 28 + 31 + 30 + 31 + 30,
                0 + 31 + 28 + 31 + 30 + 31 + 30 + 31,
                0 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
                0 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
                0 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
                0 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
            };
            // From 2000 to 2099 are every four years there is a leap year, so the calculation is simple :-)
            return mdays[mon - 1] + mday + (mon > 2 && !(year % 4));
        }
        // Calculate how many milliseconds since 2000 after
        constexpr u64 ToMs() const {
            return
                (u64)ms +
                (u64)minute * 60 * 1000 +
                (u64)hour * 60 * 60 * 1000 +
                (u64)yday() * 24 * 60 * 60 * 1000 +
                ((u64)year * 365 + (year + 3) / 4) * 24 * 60 * 60 * 1000;
        }
        constexpr u64 operator-(const CP56Time2a& rhs) const { return ToMs() - rhs.ToMs(); }
        constexpr u64 ToMs(const CP56Time2a& begin) const { return ToMs() - begin.ToMs(); }
        string ToString() const {
            if (iv == 1) { return "[INVALID]"; }
            stringstream ss;
            if (mon != 0 && mday != 0) {
                ss << ((int)year + 2000) << "."
                   << std::setw(2) << std::setfill('0') << (int)mon << "."
                   << std::setw(2) << std::setfill('0') << (int)mday << " ";
            }
            ss << std::setw(2) << std::setfill('0') << (int)hour << ":"
               << std::setw(2) << std::setfill('0') << (int)minute << ":"
               << std::setw(6) << std::setfill('0') << std::fixed << std::setprecision(3) << (ms / 1000.0f);
            return ss.str();
        }
    };
#pragma pack(pop)

    CP56Time2a ToTime(string_view str, string_view format) {
        stringstream ss;
        CP56Time2a result;
        ss << str;
        int val;
        char c;
        for (size_t i = 0; i < format.size(); ++i) {
            switch (format[i]) {
            case '%':
                ss >> val;
                switch (format[++i]) {
                case 'Y': result.year = (u8)(val - 2000); break;
                case 'm':
                    if (format[i + 1] == 's') { result.ms += (u16)(val); }
                    else { result.mon = (u8)(val); }
                    break;
                case 'd': result.mday = (u8)(val); break;
                case 'H': result.hour = (u8)(val); break;
                case 'M': result.minute = (u8)(val); break;
                case 'S': result.ms += (u8)(val / 1000); break;
                default: return result;
                }
                break;
            default:
                ss >> c;
                if (format[i] != c) { return result; }
                break;
            }
        }
        result.iv = 0;
        return result;
    }

    int GetHexVal(const vector<Byte>& code) {
        stringstream ss;
        ss << std::hex;
        for (auto&& c : code) { ss << (int)c; }
        int val;
        ss >> val;
        return val;
    }

    int GetHexVal(const vector<Byte>& code, size_t begin, size_t len) {
        stringstream ss;
        ss << std::hex;
        while (len--) { ss << (int)code[begin + len]; }
        int val;
        ss >> val;
        return val;
    }

    static void SSClear(stringstream& ss) {
        ss.str("");
        ss.clear();
        ss << std::dec;
    }

    // u8"xxx"_str, to utf8
    inline const char* operator""_str(const char8_t* u8str, size_t) {
        return reinterpret_cast<const char*>(u8str);
    }

    // utf8 to str
    inline string to_string(const char8_t* u8str) {
        return string(reinterpret_cast<const char*>(u8str));
    }

    // str to utf8
    string to_u8string(string_view str) {
        int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.data(), -1, nullptr, 0);
        assert(nwLen > 0);
        wchar_t* pwBuf = new wchar_t[(size_t)nwLen + 1];
        ZeroMemory(pwBuf, (size_t)nwLen * 2 + 2);
        ::MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.length(), pwBuf, nwLen);
        int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, nullptr, 0, nullptr, nullptr);
        assert(nLen > 0);
        char* pBuf = new char[(size_t)nLen + 1];
        ZeroMemory(pBuf, (size_t)nLen + 1);
        ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, nullptr, nullptr);
        string retStr(pBuf);
        delete[]pwBuf;
        delete[]pBuf;
        pwBuf = nullptr;
        pBuf = nullptr;
        return retStr;
    }

    // The pop-up dialog allows users manual looking for to open the file, return to the file name
    string FindFileManually(string_view filter) {
        OPENFILENAME ofn = { 0 };
        TCHAR fileName[MAX_PATH] = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = filter.data();
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = sizeof(fileName);
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = TEXT("Please select a file");
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        return GetOpenFileName(&ofn) ? fileName : "";
    }

    // str to bytes
    vector<Byte> ToBytes(string_view str) {
        stringstream ss;
        ss << str;
        vector<Byte> bytes;
        while (!ss.eof()) {
            int val;
            ss >> std::hex >> val;
            if (!ss.fail()) { bytes.push_back((unsigned char)val); }
        }
        return bytes;
    }

    // str to int
    int ToInt(string_view str) {
        stringstream ss;
        ss << str.data();
        int val = 0;
        ss >> std::hex >> val;
        return val;
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& out, const std::vector<T>& data) {
        for (auto&& d : data) {
            if constexpr (HasMenber_ToString<T>::IsTure) { out << d.ToString() << std::endl; } else { out << d << " "; }
        }
        return out;
    }

    typedef unsigned char Byte;
    std::ostream& operator<<(std::ostream& out, const std::vector<Byte>& bytes) {
        for (auto&& b : bytes) {
            out << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
        }
        return out;
    }

    template<typename T>
    struct HasMenber_ToString {
    private:
        template<typename U>
        static auto Check(int) -> decltype(std::declval<U>().ToString(), std::true_type()) {}
        template<typename U>
        static std::false_type Check(...) {}
    public:
        enum { IsTure = std::is_same<decltype(Check<T>(0)), std::true_type>::value };
    };

    // Black Magic Macro Definition~
#ifdef CLOAK_GUARD_H

#define IS_EMPTY(x) CHECK(CAT(PRIMITIVE_CAT(IS_EMPTY_, x), 0()))
#define IS_EMPTY_0() PROBE()

#define DEBUG(x) cout << #x << " = " << x << endl

#define DEBUG_N_INDIRECT() DEBUG_N
#define DEBUG_N(x, ...) \
IF(IS_EMPTY(x)) ( \
    , \
    DEBUG(x); \
    OBSTRUCT(DEBUG_N_INDIRECT)()(__VA_ARGS__) \
)

#endif
}
