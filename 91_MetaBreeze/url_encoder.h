//
//  Header.h
//  signaling-server
//
//  Created by leeco on 2017/11/29.
//
#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>


/*
 std::string strJson=URL_TOOLS::UrlDecode(URL_TOOLS::base64_decode(parameter));
 */


namespace URL_TOOLS
{
    typedef unsigned char uchar;
    static const std::string b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";//=
    static std::string base64_encode(const std::string &in) {
        std::string out;
        
        int val=0, valb=-6;
        for (uchar c : in) {
            val = (val<<8) + c;
            valb += 8;
            while (valb>=0) {
                out.push_back(b[(val>>valb)&0x3F]);
                valb-=6;
            }
        }
        if (valb>-6) out.push_back(b[((val<<8)>>(valb+8))&0x3F]);
        while (out.size()%4) out.push_back('=');
        return out;
    }
    static std::string base64_decode(const std::string &in) {
        std::string out;
        std::vector<int> T(256,-1);
        for (int i=0; i<64; i++) T[b[i]] = i;
        
        int val=0, valb=-8;
        for (uchar c : in) {
            if (T[c] == -1) break;
            val = (val<<6) + T[c];
            valb += 6;
            if (valb>=0) {
                out.push_back(char((val>>valb)&0xFF));
                valb-=8;
            }
        }
        return out;
    }
    
    
    
    
    unsigned char ToHex(unsigned char x)
    {
        return  x > 9 ? x + 55 : x + 48;
    }
    
    unsigned char FromHex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else assert(0);
        return y;
    }
    
    std::string UrlEncode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
                strTemp += str[i];
            else if (str[i] == ' ')
                strTemp += "+";
            else
            {
                strTemp += '%';
                strTemp += ToHex((unsigned char)str[i] >> 4);
                strTemp += ToHex((unsigned char)str[i] % 16);
            }
        }
        return strTemp;
    }
    
    std::string UrlDecode(const std::string& str)
    {
        std::string strTemp = "";
        size_t length = str.length();
        for (size_t i = 0; i < length; i++)
        {
            if (str[i] == '+') strTemp += ' ';
            else if (str[i] == '%')
            {
                assert(i + 2 < length);
                unsigned char high = FromHex((unsigned char)str[++i]);
                unsigned char low = FromHex((unsigned char)str[++i]);
                strTemp += high*16 + low;
            }
            else strTemp += str[i];
        }
        return strTemp;
    }
    
    

}

//const char kUnreservedChar[] = {
//    //0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, // 2
//    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 3
//    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
//    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, // 5
//    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
//    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, // 7
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // D
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // F
//};
//
//const char kHexToNum[] = {
//    // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 1
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 2
//    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, // 3
//    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 4
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 5
//    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 6
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 7
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 8
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 9
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // A
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // B
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // C
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // D
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // E
//    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // F
//};
//
//class UrlEncoder {
//public:
//    static inline void Encode(const std::string &in, std::string *out, bool uppercase = false) {
//        std::stringstream ss;
//
//        for (std::string::const_iterator it = in.begin(); it != in.end(); ++it) {
//            if (kUnreservedChar[*it]) {
//                ss << *it;
//            } else {
//                ss << '%' << std::setfill('0') << std::hex;
//                if (uppercase) ss << std::uppercase;
//                ss << (int)*it;
//            }
//        }
//
//        out->assign(ss.str());
//    }
//
//    static inline std::string Encode(const std::string &in, bool uppercase = false) {
//        std::string out;
//        Encode(in, &out, uppercase);
//        return out;
//    }
//
//    static inline bool Decode(const std::string &in, std::string *out) {
//        std::stringstream ss;
//
//        std::string::const_iterator it = in.begin();
//
//        char tmp, encoded_char;
//        while (it != in.end()) {
//            if (*it == '%') {
//                if (++it == in.end()) return false;
//                tmp = kHexToNum[*it];
//                if (tmp < 0) return false;
//                encoded_char = (16 * tmp);
//
//                if (++it == in.end()) return false;
//                tmp = kHexToNum[*it];
//                if (tmp < 0) return false;
//                encoded_char += tmp;
//
//                ++it;
//
//                ss << encoded_char;
//            } else {
//                ss << *it;
//                ++it;
//            }
//        }
//
//        out->assign(ss.str());
//
//        return true;
//    }
//
//    static inline std::string Decode(const std::string &in) {
//        std::string out;
//        Decode(in, &out);
//        return out;
//    }
//
//private:
//};

//#endif // URL_ENCODER_H
