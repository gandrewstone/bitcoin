// Copyright 2014 BitPay Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iomanip>
#include <sstream>
#include <stdio.h>
#include "univalue.h"
#include "univalue_escapes.h"

/* static */
void UniValue::jsonEscape(Stream & ss, std::string_view inS)
{
    for (const auto ch : inS)
    {
        const char * const escStr = escapes[uint8_t(ch)];

        if (escStr)
        {
            ss << escStr;
        }
        else
        {
            ss.put(ch);
        }
    }
}

static std::string json_escape(const std::string& inS)
{
    std::string outS;
    outS.reserve(inS.size() * 2);

    for (unsigned int i = 0; i < inS.size(); i++) {
        unsigned char ch = inS[i];
        const char *escStr = escapes[ch];

        if (escStr)
            outS += escStr;
        else
            outS += ch;
    }

    return outS;
}

std::string UniValue::write(unsigned int prettyIndent,
                            unsigned int indentLevel) const
{
    std::string s;
    s.reserve(1024);

    unsigned int modIndent = indentLevel;
    if (modIndent == 0)
        modIndent = 1;

    switch (typ) {
    case VNULL:
        s += "null";
        break;
    case VOBJ:
        writeObject(prettyIndent, modIndent, s);
        break;
    case VARR:
        writeArray(prettyIndent, modIndent, s);
        break;
    case VSTR:
        s += "\"" + json_escape(val) + "\"";
        break;
    case VNUM:
        s += val;
        break;
    case VBOOL:
        s += (val == "1" ? "true" : "false");
        break;
    }

    return s;
}

static void indentStr(unsigned int prettyIndent, unsigned int indentLevel, std::string& s)
{
    s.append(prettyIndent * indentLevel, ' ');
}

void UniValue::writeArray(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const
{
    s += "[";
    if (prettyIndent)
        s += "\n";

    for (unsigned int i = 0; i < values.size(); i++) {
        if (prettyIndent)
            indentStr(prettyIndent, indentLevel, s);
        s += values[i].write(prettyIndent, indentLevel + 1);
        if (i != (values.size() - 1)) {
            s += ",";
        }
        if (prettyIndent)
            s += "\n";
    }

    if (prettyIndent)
        indentStr(prettyIndent, indentLevel - 1, s);
    s += "]";
}

void UniValue::writeObject(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const
{
    s += "{";
    if (prettyIndent)
        s += "\n";

    // we do this to preserve insertion order of the keys as the internal std::map reorders them using
    // the < operator
    std::vector<std::string> vkeys(keys.size());
    for (auto &key : keys)
    {
        vkeys[key.second] = key.first;
    }

    for (size_t i = 0; i < vkeys.size(); ++i)
    {
        if (prettyIndent)
            indentStr(prettyIndent, indentLevel, s);
        s += "\"" + json_escape(vkeys[i]) + "\":";
        if (prettyIndent)
            s += " ";
        s += values.at(i).write(prettyIndent, indentLevel + 1);
        if (i != (values.size() - 1))
            s += ",";
        if (prettyIndent)
            s += "\n";
    }

    if (prettyIndent)
        indentStr(prettyIndent, indentLevel - 1, s);
    s += "}";
}

/* static */
void UniValue::stringify(Stream& ss, const UniValue& value, const unsigned int prettyIndent, const unsigned int indentLevel)
{
    std::string s;
    switch (value.typ) {
    case VNULL:
        ss << "null";
        break;
    case VBOOL:
        s += (value.val == "1" ? "true" : "false");
        ss << s;
        break;
    case VOBJ:
        value.writeObject(prettyIndent, indentLevel, s);
        ss << s;
        break;
    case VARR:
        value.writeArray(prettyIndent, indentLevel, s);
        ss << s;
        break;
    case VNUM:
        ss << value.val;
        break;
    case VSTR:
        stringify(ss, std::string_view(value.get_str()), prettyIndent, indentLevel);
        break;
    }
}

/* static */
void UniValue::stringify(Stream& ss, std::string_view string, const unsigned int prettyIndent, const unsigned int indentLevel)
{
    ss.put('"');
    jsonEscape(ss, string);
    ss.put('"');
}
