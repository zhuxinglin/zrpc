/*
*
*
*
*
*
*
*
*
*
*/

#ifndef _SERIALIZABLE__H__
#define _SERIALIZABLE__H__

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#define _SERIALIZE(o, m, f)    \
    do                         \
    {                          \
        o.Operation(#f, m);    \
        o |= f;                \
    } while (0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused"

namespace serialize
{

struct ISerialize
{
    ISerialize() : m_pszFieldName(0), m_qwType(0) {}
    inline void Operation(const char* pszFieldName, uint64_t qwType)
    {m_pszFieldName = pszFieldName; m_qwType = qwType;}
protected:
    const char *m_pszFieldName;
    uint64_t m_qwType;
};

static void StrConv(std::stringstream& ssStr, const std::string& s)
{
    ssStr << "\"";
    const char* p = s.c_str();
    int i = s.length();
    while (*p != 0 && i > 0)
    {
        switch (*p)
        {
        case '\\':
            ssStr << "\\\\";
        break;

        case '\"':
            ssStr << "\\\"";
        break;

        case '\n':
            ssStr << "\\\\n";
        break;

        case '\t':
            ssStr << "\\\\t";
        break;

        case '\r':
            ssStr << "\\\\r";
        break;

        case '\b':
            ssStr << "\\\\b";
        break;

        case '\f':
            ssStr << "\\\\f";
        break;

        default:
            ssStr << *p;
        break;
        }
        ++p;
        --i;
    }
    ssStr << "\"";
}

// ==============================================================================
// C++对象转Json
//===============================================================================
class CJson : public ISerialize
{
public:
    CJson(){}
#define JSON_TO_BOOL(type)                                  \
    CJson& operator|=(type& b)                              \
    {                                                       \
        if (m_bIsChar)                                      \
            m_ssJson << ",";                                \
        if (m_pszFieldName)                                 \
            m_ssJson << "\"" << m_pszFieldName << "\":";    \
        m_ssJson << (b ? "true" : "false");                 \
        m_bIsChar = true;                                   \
        return *this;                                       \
    }

    JSON_TO_BOOL(bool)
    JSON_TO_BOOL(const bool)
#undef JSON_TO_BOOL

#define JSON_TO_TYPE8(type)             \
    CJson& operator|=(type& c)          \
    {                                   \
        short a = c;                    \
        *this &= a;                     \
        return *this;                   \
    }

    JSON_TO_TYPE8(char)
    JSON_TO_TYPE8(const char)
    JSON_TO_TYPE8(uint8_t)
    JSON_TO_TYPE8(const uint8_t)
#undef JSON_TO_TYPE8

#define JSON_TO_TYPE(type)              \
    CJson& operator|=(type& c)          \
    {                                   \
        *this &= c;                     \
        return *this;                   \
    }

    JSON_TO_TYPE(short)
    JSON_TO_TYPE(const short)
    JSON_TO_TYPE(uint16_t)
    JSON_TO_TYPE(const uint16_t)
    JSON_TO_TYPE(int)
    JSON_TO_TYPE(const int)
    JSON_TO_TYPE(uint32_t)
    JSON_TO_TYPE(const uint32_t)
    JSON_TO_TYPE(long)
    JSON_TO_TYPE(const long)
    JSON_TO_TYPE(uint64_t)
    JSON_TO_TYPE(const uint64_t)
    JSON_TO_TYPE(float)
    JSON_TO_TYPE(double)
    JSON_TO_TYPE(long double)
    JSON_TO_TYPE(const float)
    JSON_TO_TYPE(const double)
    JSON_TO_TYPE(const long double)
#undef JSON_TO_TYPE

    CJson &operator|=(std::string &s)
    {
        *this |= (const std::string)s;
        return *this;
    }

    CJson &operator|=(const std::string &s)
    {
        if (m_bIsChar)
            m_ssJson << ",";
        if (m_pszFieldName)
            m_ssJson << "\"" << m_pszFieldName << "\":";
        
        StrConv(m_ssJson, s);
        m_bIsChar = true;
        return *this;
    }

    template<typename OBJ>
    CJson& operator|=(OBJ& o)
    {
        if (!m_pszFieldName)
        {
            if (m_bIsChar)
                m_ssJson << ",{";
            else
                m_ssJson << "{";
        }
        else
        {
            if (m_bIsChar)
                m_ssJson << ",";
            m_ssJson << "\"" << m_pszFieldName << "\":{";
        }
        // bool bIsChar = m_bIsChar;
        m_bIsChar = false;
        o.Serialize(*this);
        m_ssJson << "}";
        return *this;
    }

// bool bIsChar = m_bIsChar;
#define OJ_CONTAINER_FUN(type)                            \
    CJson &operator|=(type &v)                            \
    {                                                     \
        if (m_bIsChar)                                    \
            m_ssJson << ",";                              \
        if (!m_pszFieldName)                              \
            m_ssJson << "[";                              \
        else                                              \
            m_ssJson << "\"" << m_pszFieldName << "\":["; \
        typename type::iterator it = v.begin();           \
        m_pszFieldName = NULL;                            \
        m_bIsChar = false;                                \
        for (uint32_t i = 0; it != v.end(); ++it, ++i)    \
        {                                                 \
            m_pszFieldName = nullptr;                     \
            *this |= *it;                                 \
        }                                                 \
        m_ssJson << "]";                                  \
        return *this;                                     \
    }

    template<typename T>
    OJ_CONTAINER_FUN(std::vector<T>);

    template<typename T>
    OJ_CONTAINER_FUN(std::list<T>);

    template<typename T>
    OJ_CONTAINER_FUN(std::set<T>);
#undef OJ_CONTAINER_FUN

    template<typename K, typename V>
    CJson& operator|=(std::map<K, V>& m)
    {
        if (m_bIsChar)
            m_ssJson << ",";
        
        if (m_pszFieldName)
            m_ssJson << "\"" << m_pszFieldName << "\":[";
        else
            m_ssJson << "[";
        
        m_pszFieldName = NULL;
        //bool bIsChar = m_bIsChar;
        bool bIsFirst = false;
        typename std::map<K, V>::iterator it = m.begin();
        for (; it != m.end(); ++ it)
        {
            m_bIsChar = false;
            if (bIsFirst)
                m_ssJson << ",[";
            else
            {
                m_ssJson << "[";
                bIsFirst = true;
            }

            m_pszFieldName = nullptr;
            *this |= it->first;
            m_pszFieldName = nullptr;
            *this |= it->second;
            m_ssJson << "]";
        }
        m_ssJson << "]";
        return *this;
    }

    template<typename OBJ>
    std::string ToJson(OBJ& o)
    {
        init();
        *this |= o;
        return m_ssJson.str();
    }

    template<typename OBJ>
    CJson& operator << (OBJ& o)
    {
        init();
        *this |= o;
        return *this;
    }

    const std::string GetJson(){return m_ssJson.str();}

private:
    void init()
    {
        m_ssJson.str("");
        m_pszFieldName = nullptr;
        m_bIsChar = false;
    }

    template<typename T>
    inline CJson& operator &= (T& c)
    {
        if (m_bIsChar)
            m_ssJson << ",";
        if (m_pszFieldName)
            m_ssJson << "\"" << m_pszFieldName << "\":";
        m_ssJson << c;
        m_bIsChar = true;
        return *this;
    }

private:
    std::stringstream m_ssJson;
    bool m_bIsChar;
};

// ==========================================================================
// C++ STL 转 C++ 对象
// ==========================================================================
template <class OBJECT>
class CStlToObj
{
public:
    explicit CStlToObj(OBJECT& o): m_pObj(&o){}

private:
    template<typename STL>
    class CMapToObj : public ISerialize
    {
    public:
        explicit CMapToObj(STL& o):m_pObj(&o){};

    #define TO_OBJ_FIELD(type)              \
        CMapToObj& operator|= (type &c)     \
        {                                   \
            to_type(c, m_pObj);             \
            return *this;                   \
        }

        TO_OBJ_FIELD(int16_t)
        TO_OBJ_FIELD(uint16_t)
        TO_OBJ_FIELD(int32_t)
        TO_OBJ_FIELD(uint32_t)
        TO_OBJ_FIELD(std::string)
        TO_OBJ_FIELD(int64_t)
        TO_OBJ_FIELD(uint64_t)
        TO_OBJ_FIELD(float)
        TO_OBJ_FIELD(double)
    #undef TO_OBJ_FIELD

        template<typename TY, typename M>
        inline void to_type(TY& c, std::map<std::string, M>*& m)
        {
            typename STL::iterator it = m->find(m_pszFieldName);
            if (it == m->end())
                return ;
            ConversionValue<TY> oValue(c);
            oValue << it->second;
        }

        template<typename TY, typename M>
        inline void to_type(TY& c, std::map<uint32_t, M>*& m)
        {
            typename STL::iterator it = m->find(m_qwType);
            if (it == m->end())
                return ;
            ConversionValue<TY> oValue(c);
            oValue << it->second;
        }

        template<class OBJ>
        CMapToObj& operator |= (OBJ &c)
        {
            to_obj(c, m_pObj);
            return *this;
        }

        template<typename OBJ, typename OBJ1>
        void to_obj(OBJ& o, std::map<uint32_t, OBJ1>*& m)
        {
            typename STL::iterator it = m->find(m_qwType);
            if (it == m->end())
                return;

            o = it->second;
        }

        template<typename OBJ, typename OBJ1>
        void to_obj(OBJ& o, std::map<std::string, OBJ1>*& m)
        {
            typename STL::iterator it = m->find(m_pszFieldName);
            if (it == m->end())
                return;

            o = it->second;
        }

        template <class OBJ>
        CMapToObj& operator >> (OBJ& o)
        {
            o.Serialize(*this);
            return *this;
        }

    private:
        STL* m_pObj;
    };

private:
    template<class T>
    class ConversionValue
    {
    public:
        ConversionValue(T& obj):m_pPtr(&obj){}

        template<typename F>
        ConversionValue& operator << (F & c)
        {
            to_type(m_pPtr, c);
            return *this;
        }

        template<typename TYPE, typename TI>
        inline void to_type(TYPE*& t, TI& v)
        {*t = static_cast<TYPE>(v);}

#define TO_OBJ_STRING(type)                             \
        inline void to_type(const std::string*& v, type& t)   \
        {*v = std::to_string(t);}

        TO_OBJ_STRING(int16_t)
        TO_OBJ_STRING(uint16_t)
        TO_OBJ_STRING(int32_t)
        TO_OBJ_STRING(uint32_t)
        TO_OBJ_STRING(int64_t)
        TO_OBJ_STRING(uint64_t)
        TO_OBJ_STRING(float)
        TO_OBJ_STRING(double)
#undef TO_OBJ_STRING

        inline void to_type(int16_t*& v, std::string& t)
        {*v = atoi(t.c_str());}

        inline void to_type(uint16_t*& v, std::string& t)
        {*v = atoi(t.c_str());}

        inline void to_type(int32_t*& v, std::string& t)
        {*v = atoi(t.c_str());}

        inline void to_type(uint32_t*& v, std::string& t)
        {*v = strtoul(t.c_str(), nullptr, 10);}

        inline void to_type(int64_t*& v, std::string& t)
        {*v = atol(t.c_str());}

        inline void to_type(uint64_t*& v, std::string& t)
        {*v = strtoull(t.c_str(), nullptr, 10);}

        inline void to_type(float*& v, std::string& t)
        {*v = atof(t.c_str());}

        inline void to_type(double*& v, std::string& t)
        {*v = strtod(t.c_str(), nullptr);}

    private:
        T* m_pPtr;
    };

private:
    template<typename I, typename S>
    class CBaseTypeToObj : public ISerialize
    {
    public:
        CBaseTypeToObj(I& i, S& s):m_oIt(i),m_oObj(s){};
        ~CBaseTypeToObj() = default;
    
    public:
        template<typename T>
        CBaseTypeToObj& operator|= (T& v)
        {
            if (m_oIt == m_oObj.end())
                return *this;
            ConversionValue<T> oV(v);
            oV << *m_oIt;
            ++ m_oIt;
            return *this;
        }

        template<typename T>
        CBaseTypeToObj& operator >> (T& o)
        {
            if (m_oIt == m_oObj.end())
                return *this;
            o.Serialize(*this);
            return *this;
        }
    
    private:
        I& m_oIt;
        S& m_oObj;
    };

private:
    OBJECT* m_pObj;

public:
    template<typename OBJ>
    CStlToObj& operator << (std::map<std::string, OBJ>& m)
    {
        CMapToObj<std::map<std::string, OBJ> > oMap(m);
        oMap >> *m_pObj;
        return *this;
    }

    template<typename OBJ>
    CStlToObj& operator << (std::map<uint32_t, OBJ>& m)
    {
        CMapToObj<std::map<uint32_t, OBJ> > oMap(m);
        oMap >> *m_pObj;
        return *this;
    }

    template <typename OBJ>
    CStlToObj& operator >> (std::vector<OBJ>& v)
    {
        typename OBJECT::iterator it = m_pObj->begin();
        for (; it != m_pObj->end();)
        {
            CBaseTypeToObj<typename OBJECT::iterator, OBJECT> oArray(it, *m_pObj);
            OBJ o;
            oArray >> o;
            v.push_back(o);
        }
        return *this;
    }

    template <typename OBJ>
    CStlToObj& operator >> (std::list<OBJ>& v)
    {
        typename OBJECT::iterator it = m_pObj->begin();
        for (; it != m_pObj->end();)
        {
            CBaseTypeToObj<typename OBJECT::iterator, OBJECT> oArray(it, *m_pObj);
            OBJ o;
            oArray >> o;
            v.push_back(o);
        }
        return *this;
    }

    template <typename OBJ>
    CStlToObj& operator >> (OBJ& o)
    {
        typename OBJECT::iterator it = m_pObj->begin();
        CBaseTypeToObj<typename OBJECT::iterator, OBJECT> oArray(it, *m_pObj);
        oArray >> o;
        return *this;
    }

public:
    template<typename OBJ>
    void ToObject(std::map<std::string, OBJ>& m)
    {
        this->operator<<(m);
    }

    template<typename OBJ>
    void ToObject(std::map<uint32_t, OBJ>& m)
    {
        this->operator<<(m);
    }

    template<typename OBJ>
    void ToObject(std::vector<OBJ>& v)
    {
        this->operator>>(v);
    }

    template<typename OBJ>
    void ToObject(std::list<OBJ>& v)
    {
        this->operator>>(v);
    }

    template<typename OBJ>
    void ToObject(OBJ& v)
    {
        this->operator>>(v);
    }
};

// =======================================================================
// Json 转 C++对象
// =======================================================================
class CJsonToObjet : public ISerialize
{
public:
    CJsonToObjet(const char* pszBuf) : m_pValue(nullptr)
    {
        m_oParse.Parse<0>(pszBuf);
    }

    CJsonToObjet(const std::string& sBuf) : m_pValue(nullptr)
    {
        m_oParse.Parse<0>(sBuf.c_str(), sBuf.size());
    }

#define JSON_TO_FIELD_VALUE(type, expression1, expression2) \
    CJsonToObjet& operator|= (type& o)                      \
    {                                                       \
        if (!m_pValue->HasMember(m_pszFieldName))           \
            return *this;                                   \
        if (!(*m_pValue)[m_pszFieldName].expression1)       \
            return *this;                                   \
        o = (type)(*m_pValue)[m_pszFieldName].expression2;  \
        return *this;                                       \
    }

    JSON_TO_FIELD_VALUE(bool, IsBool(), GetBool())
    JSON_TO_FIELD_VALUE(int, IsInt(), GetInt())
    JSON_TO_FIELD_VALUE(char, IsInt(), GetInt())
    JSON_TO_FIELD_VALUE(short, IsInt(), GetInt())
    JSON_TO_FIELD_VALUE(uint16_t, IsUint(), GetUint())
    JSON_TO_FIELD_VALUE(uint32_t, IsUint(), GetUint())
    JSON_TO_FIELD_VALUE(uint64_t, IsInt64(), GetInt64())
    JSON_TO_FIELD_VALUE(int64_t, IsUint64(), GetUint64())
    JSON_TO_FIELD_VALUE(float, IsFloat(), GetFloat())
    JSON_TO_FIELD_VALUE(double, IsDouble(), GetDouble())
    JSON_TO_FIELD_VALUE(std::string, IsString(), GetString())
#undef JSON_TO_FIELD_VALUE

    template<typename OBJ>
    CJsonToObjet& operator|= (OBJ& o)
    {
        rapidjson::Value* pValue;
        if (SaveValue(pValue))
            return *this;

        do
        {
            if (!(*m_pValue).IsObject())
                break;

            if ((*m_pValue).ObjectEmpty())
                break;

            o.Serialize(*this);
        } while (0);
        m_pValue = pValue;
        return *this;
    }

    template<typename K, typename V>
    CJsonToObjet& operator |= (std::map<K, V>& m)
    {
        rapidjson::Value* pValue;
        if (SaveValue(pValue))
            return *this;

        do
        {
            if (!m_pValue->IsArray())
                break;
        
            rapidjson::Value& ar = (*m_pValue);
            for (int i = 0; i < ar.Size(); ++ i)
            {
                m_pszFieldName = nullptr;
                m_pValue = &ar[i];
                if (ar[i].IsArray() || ar[i].IsObject())
                {
                    *this |= m;
                    continue;
                }

                K k;
                *this &= k;

                m_pszFieldName = nullptr;
                m_pValue = &ar[++ i];
                V v;
                *this &= v;

                m.insert(std::make_pair(k, v));
            }
        }while (0);
        m_pValue = pValue;
        return *this;
    }

#define JSON_TO_STL_ARRAY(type)                     \
    CJsonToObjet& operator |= (type& o)             \
    {                                               \
        rapidjson::Value* pValue;                   \
        if (SaveValue(pValue))                      \
            return *this;                           \
        do                                          \
        {                                           \
            if (!m_pValue->IsArray())               \
                break;                              \
            rapidjson::Value& ar = (*m_pValue);     \
            for (int i = 0; i < ar.Size(); ++ i)    \
            {                                       \
                m_pszFieldName = nullptr;           \
                m_pValue = &ar[i];                  \
                OBJ v;                              \
                *this &= v;                         \
                o.push_back(v);                     \
            }                                       \
        } while (0);                                \
        m_pValue = pValue;                          \
        return *this;                               \
    }

    template<typename OBJ>
    JSON_TO_STL_ARRAY(std::vector<OBJ>)
    template<typename OBJ>
    JSON_TO_STL_ARRAY(std::list<OBJ>)
#undef JSON_TO_STL_ARRAY

    template<typename OBJ>
    CJsonToObjet& operator |= (std::set<OBJ>& o)  
    {
        rapidjson::Value* pValue;
        if (SaveValue(pValue))
            return *this;
        do
        {
            if (!m_pValue->IsArray())
                break;

            rapidjson::Value& ar = *m_pValue;
            for (int i = 0; i < ar.Size(); ++ i)
            {
                m_pszFieldName = nullptr;
                m_pValue = &ar[i];
                OBJ v;
                *this &= v;
                o.insert(v);
            }
        } while (0);
        
        m_pValue = pValue;
        return *this;
    }

    template <typename T>
    bool ToObject(T& o)
    {
        if (m_oParse.HasParseError())
            return false;

        m_pValue = nullptr;
        *this |= o;
        return true;
    }

    template<typename T>
    CJsonToObjet& operator >> (T& o)
    {
        if (m_oParse.HasParseError())
            return *this;

        m_pValue = nullptr;
        *this |= o;
        return *this;
    }

    operator bool ()
    {
        if (m_oParse.HasParseError())
            return true;
        return false;
    }

    bool IsObject()
    {
        return m_oParse.IsObject();
    }

    bool IsArray()
    {
        return m_oParse.IsArray();
    }

    void Print()
    {
        if (m_oParse.HasParseError())
            fprintf(stderr, "parse error: (%d:%lu)%s\n", m_oParse.GetParseError(), m_oParse.GetErrorOffset(), rapidjson::GetParseError_En(m_oParse.GetParseError()));
    }

private:
    bool SaveValue(rapidjson::Value*& pValue)
    {
        pValue = m_pValue;
        if (!pValue)
        {
            if (m_pszFieldName)
            {
                if (!m_oParse.HasMember(m_pszFieldName))
                    return true;

                m_pValue = &m_oParse[m_pszFieldName];
            }
            else
                m_pValue = &m_oParse;
        }
        else
        {
            if (m_pszFieldName)
            {
                if (!m_pValue->HasMember(m_pszFieldName))
                    return true;
                
                m_pValue = &(*m_pValue)[m_pszFieldName];
            }
        }

        return false;
    }

#define JSON_ARRAY_VALE(type, expression1, expression2)     \
    CJsonToObjet& operator &= (type& c)                     \
    {                                                       \
        if ((*m_pValue).expression1)                        \
            c = (*m_pValue).expression2;                    \
        return *this;                                       \
    }

    JSON_ARRAY_VALE(std::string, IsString(), GetString())
    JSON_ARRAY_VALE(int, IsInt(), GetInt())
    JSON_ARRAY_VALE(short, IsInt(), GetInt())
    JSON_ARRAY_VALE(uint16_t, IsUint(), GetUint())
    JSON_ARRAY_VALE(uint32_t, IsUint(), GetUint())
    JSON_ARRAY_VALE(int64_t, IsInt64(), GetInt64())
    JSON_ARRAY_VALE(uint64_t, IsUint64(), GetUint64())
    JSON_ARRAY_VALE(float, IsFloat(), GetFloat())
    JSON_ARRAY_VALE(double, IsDouble(), GetDouble())
#undef JSON_ARRAY_VALE

    template<typename OBJ>
    CJsonToObjet& operator &= (OBJ& c)
    {
        if ((*m_pValue).IsArray() || (*m_pValue).IsObject())
            *this |= c;
        return *this;
    }

private:
    rapidjson::Document m_oParse;
    rapidjson::Value* m_pValue;
};

static std::string Base64Encode(const char* buf, int src_len)
{
    uint8_t * base64 = (uint8_t *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int i, j;
    std::string sBuf;
    sBuf.resize(src_len / 3 * 4 + 2);
    uint8_t *dst = reinterpret_cast<uint8_t*>(const_cast<char*>(sBuf.c_str()));
    uint8_t* p = dst;
    for(i = 0, j = 0; i <= src_len - 3; i += 3, j += 4)
    {
        dst[j] = (buf[i] & 0xFC) >> 2;
        dst[j + 1] = ((buf[i] & 0x03) << 4) + ((buf[i + 1] & 0xF0) >> 4);
        dst[j + 2] = ((buf[i + 1] & 0x0F) << 2) + ((buf[i + 2] & 0xC0) >> 6);
        dst[j + 3] = buf[i + 2] & 0x3F;
    }

    if(src_len % 3 == 1)
    {
        dst[j] = (buf[i] & 0xFC) >> 2;
        dst[j + 1] = ((buf[i] & 0x03) << 4);
        dst[j + 2] = 64;
        dst[j + 3] = 64;
        j += 4;
    }
    else if(src_len % 3 == 2)
    {
        dst[j] = (buf[i] & 0xFC) >> 2;
        dst[j + 1] = ((buf[i] & 0x03) << 4) + ((buf[i + 1] & 0xF0) >> 4);
        dst[j + 2] = ((buf[i + 1] & 0x0F) << 2);
        dst[j + 3] = 64;
        j += 4;
    }
    for(i = 0; i < j; i++) // map 6 bit value to base64 ASCII character 
        *p++ = base64[dst[i]];
    *p = 0;
    sBuf.resize(p - dst);
    return std::move(sBuf);
}

//=====================================================================================
// PB 转 Json
//=====================================================================================
class CPbToJson
{
public:
    CPbToJson(const ::google::protobuf::Message& oMsg, bool bEnumValue = true):m_pMsg(&oMsg), m_bIsEnumValue(bEnumValue){}
    ~CPbToJson(){}

public:
    CPbToJson& operator >> (std::string& oJson)
    {
        m_ssStream.str("");
        PbToJson(*m_pMsg);
        oJson.append(std::move(m_ssStream.str()));
        return *this;
    }

    void ToJson(std::string& oJson)
    {
        this->operator>>(oJson);
    }

private:
    void PbToJson(const ::google::protobuf::Message& oMsg)
    {
        const ::google::protobuf::Descriptor *descriptor = oMsg.GetDescriptor();
        const ::google::protobuf::Reflection *reflection = oMsg.GetReflection();

        const uint32_t count = descriptor->field_count();
        m_ssStream << "{";
        for (uint32_t i = 0; i < count; ++ i)
        {
            const ::google::protobuf::FieldDescriptor* field = descriptor->field(i);
            if (field == nullptr)
                continue;

            do
            {
                if (field->is_repeated())
                {
                    int iArraySize = reflection->FieldSize(oMsg, field);
                    if (iArraySize <= 0)
                        break;

                    if (i < count && i != 0)
                        m_ssStream << ",";

                    m_ssStream << "\"" << field->name() << "\":[";
                    JsonArray(oMsg, reflection, field, iArraySize);
                    m_ssStream << "]";
                }
                else
                {
                    if (!reflection->HasField(oMsg, field))
                        break;

                    if (i < count && i != 0)
                        m_ssStream << ",";

                    m_ssStream << "\"" << field->name() << "\":";
                    Json(oMsg, reflection, field, 0);
                }
            } while (0);
        }
        m_ssStream << "}";
    }

    void JsonArray(const ::google::protobuf::Message& oMsg, const ::google::protobuf::Reflection *reflection, 
                const ::google::protobuf::FieldDescriptor* field, int count)
    {
        for (int i = 0; i < count;)
        {
            Json(oMsg, reflection, field, i);
            ++ i;

            if (i < count)
                m_ssStream << ",";
        }
    }

    void Json(const ::google::protobuf::Message& oMsg, const ::google::protobuf::Reflection *reflection, 
            const ::google::protobuf::FieldDescriptor* field, int i)
    {
        switch(field->cpp_type())
        {
        case ::google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            if (field->is_repeated())
            {
                const ::google::protobuf::Message &oTepMsg = reflection->GetRepeatedMessage(oMsg, field, i);
                if (oTepMsg.ByteSize() != 0)
                    PbToJson(oTepMsg);
                break;
            }
            const ::google::protobuf::Message &oTepMsg = reflection->GetMessage(oMsg, field);
            if (oTepMsg.ByteSize() != 0)
            {
                m_ssStream << "\"" << field->name() << "\":";
                PbToJson(oTepMsg);
            }
        }
        break;

#define CASE_FIELD_TYPE_PB_TO_JSON(CASE_TYPE, method)                           \
        case ::google::protobuf::FieldDescriptor::CPPTYPE_##CASE_TYPE:          \
        {                                                                       \
            if (field->is_repeated())                                           \
                m_ssStream << reflection->GetRepeated##method(oMsg, field, i);  \
            else                                                                \
                m_ssStream << reflection->Get##method(oMsg, field);             \
        }                                                                       \
        break;

        CASE_FIELD_TYPE_PB_TO_JSON(INT32, Int32)
        CASE_FIELD_TYPE_PB_TO_JSON(UINT32, UInt32)
        CASE_FIELD_TYPE_PB_TO_JSON(INT64, Int64)
        CASE_FIELD_TYPE_PB_TO_JSON(UINT64, UInt64)
        CASE_FIELD_TYPE_PB_TO_JSON(FLOAT, Float)
        CASE_FIELD_TYPE_PB_TO_JSON(DOUBLE, Double)
#undef CASE_FIELD_TYPE_PB_TO_JSON

        case ::google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            std::string s;
            if (field->is_repeated())
            {
                s = reflection->GetRepeatedStringReference(oMsg, field, i, &s);
                if (::google::protobuf::FieldDescriptor::TYPE_BYTES == field->type())
                {
                    s = Base64Encode(s.c_str(), s.length());
                    m_ssStream << "\"" << s << "\"";
                    break;
                }
            }
            else
                s = reflection->GetString(oMsg, field);
            StrConv(m_ssStream, s);
        }
        break;

        case ::google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            bool b;
            if (field->is_repeated())
                b = reflection->GetRepeatedBool(oMsg, field, i);
            else
                b = reflection->GetBool(oMsg, field);
            if (b)
                m_ssStream << "true";
            else
                m_ssStream << "flase";
        }
        break;

        case ::google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            if (!m_bIsEnumValue)
            {
                if (field->is_repeated())
                    m_ssStream << "\"" << reflection->GetRepeatedEnum(oMsg, field, i)->name() << "\"";
                else
                    m_ssStream << "\"" << reflection->GetEnum(oMsg, field)->name() << "\"";
            }
            else
            {
                if (field->is_repeated())
                    m_ssStream << reflection->GetRepeatedEnum(oMsg, field, i)->number();
                else
                    m_ssStream << reflection->GetEnum(oMsg, field)->number();
            }
        }
        break;
        }
    }

private:
    std::stringstream m_ssStream;
    const ::google::protobuf::Message* m_pMsg;
    bool m_bIsEnumValue;
};

static std::string Base64Decode(const char* buf, int src_len)
{
        //根据base64表，以字符找到对应的十进制数据  
    int table[]=
    {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,
        0,  0,  0,  0,  0,  0,  0,  62, 0,  0,  0, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,
        0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
        7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  0,
        0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
        37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51
    };

    int len = src_len / 4 * 3;
    const char* e = buf + src_len - 1;
    while (*e == '=' && *e != 0)
        -- len;

    const uint8_t* dat = reinterpret_cast<const uint8_t*>(buf);

    std::string sBuf;
    sBuf.resize(len);
    uint8_t *res = reinterpret_cast<uint8_t*>(const_cast<char*>(sBuf.c_str()));

    //以4个字符为一位进行解码
    for(int i = 0, j = 0; i < src_len - 2; j += 3, i += 4)
    {
        //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的后2位进行组合
        res[j] = ((uint8_t)table[dat[i]]) << 2 | (((uint8_t)table[dat[i + 1]]) >> 4);
        //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应bas464表的十进制数的后4位进行组合
        res[j + 1] = (((uint8_t)table[dat[i + 1]]) << 4) | (((uint8_t)table[dat[i + 2]]) >> 2);
        //取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合
        res[j + 2] = (((uint8_t)table[dat[i + 2]]) <<6 ) | ((uint8_t)table[dat[i + 3]]);
    }

    return sBuf;
}

// Json to Pb
class CJsonToPb
{
public:
    CJsonToPb(const std::string& sJson):m_Ret(0)
    {
        m_oParse.Parse(sJson.c_str(), sJson.size());
    }

    CJsonToPb(const char* pszJson):m_Ret(0)
    {
        m_oParse.Parse(pszJson);
    }

public:
    CJsonToPb& operator >> (::google::protobuf::Message& oMsg)
    {
        if (m_oParse.HasParseError())
            return *this;

        m_Ret = 1;
        rapidjson::Value* pValue = &m_oParse;
        if (JsonToPb(&oMsg, pValue))
            return *this;

        m_Ret = 2;
        return *this;
    }

    operator bool()
    {
        if (m_oParse.HasParseError())
            return true;
        
        if (m_Ret == 0)
            return false;

        if (m_Ret == 1)
            return true;

        return false;
    }
    
private:
    bool JsonToPb(::google::protobuf::Message* pMsg, rapidjson::Value* pValue)
    {
        if (!pValue->IsObject())
            return true;
        
        const ::google::protobuf::Descriptor *descriptor = pMsg->GetDescriptor();
        const ::google::protobuf::Reflection *reflection = pMsg->GetReflection();
        const uint32_t count = descriptor->field_count();
        for (uint32_t i = 0; i < count; ++ i)
        {
            const ::google::protobuf::FieldDescriptor* field = descriptor->field(i);
            if (!field)
                continue;

            const std::string& name = field->name();
            if (field->is_repeated())
            {
                if (!pValue->HasMember(name.c_str()))
                    continue;

                rapidjson::Value& oTemp = (*pValue)[name.c_str()];
                if(!oTemp.IsArray())
                    return true;

                if (ToPb(pMsg, &oTemp, reflection, field))
                    return true;
                continue;
            }

            if (!pValue->HasMember(name.c_str()))
                continue;

            if (ToPb(pMsg, &(*pValue)[name.c_str()], reflection, field))
                return true;
        }
        return false;
    }

    bool ToPb(::google::protobuf::Message* pMsg, rapidjson::Value* pValue, 
            const ::google::protobuf::Reflection *reflection, const ::google::protobuf::FieldDescriptor* field)
    {
        switch (field->cpp_type())
        {
        case ::google::protobuf::FieldDescriptor::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            if (field->is_repeated())
            {
                uint32_t count = pValue->Size();
                for (uint32_t i = 0; i < count; ++ i)
                {
                    rapidjson::Value& oVal = (*pValue)[i];
                    if (JsonToPb(reflection->AddMessage(pMsg, field), &oVal))
                        return true;
                }
            }
            else
            {
                if (JsonToPb(reflection->MutableMessage(pMsg, field), pValue))
                    return true;
            }
        }
        break;

        case ::google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            if (field->is_repeated())
            {
                uint32_t count = pValue->Size();
                for (uint32_t i = 0; i < count; ++ i)
                {
                    rapidjson::Value& oVal = (*pValue)[i];
                    if (ConvertEnum(pMsg, oVal, reflection, field))
                        return true;
                }
            }
            else
            {
                if (ConvertEnum(pMsg, *pValue, reflection, field))
                    return true;
            }
        }
        break;

#define CASE_FIELD_JSON_TO_PB_TYPE(FIELD_TYPE, PbMethod, JsonMethod)                        \
        case ::google::protobuf::FieldDescriptor::FieldDescriptor::CPPTYPE_##FIELD_TYPE:    \
        {                                                                                   \
            if (field->is_repeated())                                                       \
            {                                                                               \
                uint32_t count = pValue->Size();                                            \
                for (uint32_t i = 0; i < count; ++ i)                                       \
                {                                                                           \
                    rapidjson::Value& oVal = (*pValue)[i];                                  \
                    if (!oVal.Is##JsonMethod())                                             \
                        return true;                                                        \
                    reflection->Add##PbMethod(pMsg, field, oVal.Get##JsonMethod());         \
                }                                                                           \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                if (!pValue->Is##JsonMethod())                                              \
                    return true;                                                            \
                reflection->Set##PbMethod(pMsg, field, pValue->Get##JsonMethod());          \
            }                                                                               \
        }                                                                                   \
        break;

        CASE_FIELD_JSON_TO_PB_TYPE(BOOL, Bool, Bool)
        CASE_FIELD_JSON_TO_PB_TYPE(INT32, Int32, Int)
        CASE_FIELD_JSON_TO_PB_TYPE(UINT32, UInt32, Uint)
        CASE_FIELD_JSON_TO_PB_TYPE(INT64, Int64, Int64)
        CASE_FIELD_JSON_TO_PB_TYPE(UINT64, UInt64, Uint64)
#undef CASE_FIELD_JSON_TO_PB_TYPE

        case ::google::protobuf::FieldDescriptor::TYPE_STRING:
        {
            if (field->is_repeated())
            {
                uint32_t count = pValue->Size();
                for (uint32_t i = 0; i < count; ++ i)
                {
                    rapidjson::Value& oVal = (*pValue)[i];
                    if (!oVal.IsString())
                        return true;
                    if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
                    {
                        std::string s = Base64Decode(oVal.GetString(), oVal.GetStringLength());
                        reflection->AddString(pMsg, field, s);
                    }
                    else
                        reflection->AddString(pMsg, field, oVal.GetString());
                }
            }
            else
            {
                if (!pValue->IsString())
                    return true;
                if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
                {
                    std::string s = Base64Decode(pValue->GetString(), pValue->GetStringLength());
                    reflection->SetString(pMsg, field, s);
                }
                else
                    reflection->SetString(pMsg, field, pValue->GetString());
            }
        }

        case ::google::protobuf::FieldDescriptor::FieldDescriptor::CPPTYPE_FLOAT:
        {
            if (field->is_repeated())
            {
                uint32_t count = pValue->Size();
                for (uint32_t i = 0; i < count; ++ i)
                {
                    rapidjson::Value& oVal = (*pValue)[i];
                    if (PbPushConvertDoubleFloatType(pMsg, oVal, reflection, field, 
                        &google::protobuf::Reflection::AddFloat, &rapidjson::Value::GetFloat))
                        return true;
                }
            }
            else
            {
                if (PbPushConvertDoubleFloatType(pMsg, *pValue, reflection, field, 
                    &google::protobuf::Reflection::SetFloat, &rapidjson::Value::GetFloat))
                    return true;
            }
        }
        break;

        case ::google::protobuf::FieldDescriptor::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            if (field->is_repeated())
            {
                uint32_t count = pValue->Size();
                for (uint32_t i = 0; i < count; ++ i)
                {
                    rapidjson::Value& oVal = (*pValue)[i];
                    if (PbPushConvertDoubleFloatType(pMsg, oVal, reflection, field, 
                        &google::protobuf::Reflection::AddDouble, &rapidjson::Value::GetDouble))
                        return true;
                }
            }
            else
            {
                if (PbPushConvertDoubleFloatType(pMsg, *pValue, reflection, field, 
                    &google::protobuf::Reflection::SetDouble, &rapidjson::Value::GetDouble))
                    return true;
            }
        }
        break;
        }
        return false;
    }

    bool ConvertEnum(::google::protobuf::Message* pMsg, rapidjson::Value& oItem,
            const ::google::protobuf::Reflection *reflection, const ::google::protobuf::FieldDescriptor* field)
    {
        const google::protobuf::EnumValueDescriptor * pEnumValueDescriptor = NULL;
        if (oItem.IsInt())
            pEnumValueDescriptor = field->enum_type()->FindValueByNumber(oItem.GetInt());
        else if (oItem.IsString())
            pEnumValueDescriptor = field->enum_type()->FindValueByName(oItem.GetString());
        
        if (!pEnumValueDescriptor)
            return true;
        
        if (field->is_repeated())
            reflection->AddEnum(pMsg, field, pEnumValueDescriptor);
        else
            reflection->SetEnum(pMsg, field, pEnumValueDescriptor);
        return false;
    }

    template<typename T, typename R>
    bool PbPushConvertDoubleFloatType(::google::protobuf::Message* pMsg, rapidjson::Value& oItem,
        const ::google::protobuf::Reflection *reflection, const ::google::protobuf::FieldDescriptor* field,
        void (::google::protobuf::Reflection::*func)(::google::protobuf::Message*,
        const ::google::protobuf::FieldDescriptor*, T) const,
        R (rapidjson::Value::*get)() const)
    {
        if (oItem.IsNumber())
        {
            if (field->is_repeated())
                (reflection->*func)(pMsg, field, (oItem.*get)());
            else
                (reflection->*func)(pMsg, field, (oItem.*get)());
        }
        else if (oItem.IsString())
        {
            if (ConvertDoubleFloatType(pMsg, oItem.GetString(), reflection, field, func))
                return true;
        }
        else
            return true;
        return false;
    }

    template<typename T>
    bool ConvertDoubleFloatType(::google::protobuf::Message* pMsg, const char* pszLimitType, 
        const ::google::protobuf::Reflection *reflection, const ::google::protobuf::FieldDescriptor* field,
        void (::google::protobuf::Reflection::*func)(::google::protobuf::Message*,
        const ::google::protobuf::FieldDescriptor*, T)const)
    {
        if (std::numeric_limits<T>::has_quiet_NaN && strcasecmp(pszLimitType, "NaN") == 0)
        {
            (reflection->*func)(pMsg, field, std::numeric_limits<T>::quiet_NaN());
            return false;
        }

        if (std::numeric_limits<T>::has_infinity && strcasecmp(pszLimitType, "Infinity") == 0)
        {
            (reflection->*func)(pMsg, field, std::numeric_limits<T>::infinity());
            return false;
        }

        if (std::numeric_limits<T>::has_infinity && strcasecmp(pszLimitType, "-Infinity") == 0)
        {
            (reflection->*func)(pMsg, field, -std::numeric_limits<T>::infinity());
            return false;
        }
        return true;
    }

public:
    rapidjson::Document m_oParse;
    int m_Ret;
};

}

#pragma GCC diagnostic pop

#endif
