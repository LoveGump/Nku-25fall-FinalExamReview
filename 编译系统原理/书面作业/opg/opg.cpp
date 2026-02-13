#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <sstream>
#include <functional>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iomanip>

#ifndef ERROR
#define ERROR(...)                                                                     \
    do {                                                                               \
        char message[256];                                                             \
        sprintf(message, __VA_ARGS__);                                                 \
        std::cerr << "\033[;31;1m";                                                    \
        std::cerr << "ERROR: ";                                                        \
        std::cerr << "\033[0;37;1m";                                                   \
        std::cerr << message << std::endl;                                             \
        std::cerr << "\033[0;33;1m";                                                   \
        std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cerr << "\033[0m";                                                        \
        assert(false);                                                                 \
    } while (0)
#endif  // ERROR

#ifndef ASSERT
#define ASSERT(EXP)                                          \
    do {                                                     \
        if (!(EXP)) { ERROR("Assertion failed: %s", #EXP); } \
    } while (0)
#endif  // ASSERT

using namespace std;

string trim(const string& s)
{
    size_t i = 0, j = s.size();
    while (i < j && isspace((unsigned char)s[i])) ++i;
    while (j > i && isspace((unsigned char)s[j - 1])) --j;
    return s.substr(i, j - i);
}

void replaceAll(string& s, const string& a, const string& b)
{
    size_t pos = 0;
    while ((pos = s.find(a, pos)) != string::npos)
    {
        s.replace(pos, a.size(), b);
        pos += b.size();
    }
}

class OPGParser
{
  public:
    struct Token
    {
        static constexpr const char* Sentinel = "$";
        static constexpr const char  Conflict = '!';

        string_view name;
        bool        isNTerm;

        Token() = default;
        Token(string_view n, bool isNT) : name(n), isNTerm(isNT) {}
    };

    OPGParser() = default;
    ~OPGParser() { clear(); }

    map<Token*, set<Token*>>              firstVT;
    map<Token*, set<Token*>>              lastVT;
    set<Token*>                           nterms;
    set<Token*>                           terms;
    map<Token*, map<Token*, char>>        relation;
    vector<tuple<string, string, string>> trace;

  private:
    char relationOf(Token* x, Token* y) const
    {
        auto it = relation.find(x);
        ASSERT(it != relation.end());
        auto it2 = it->second.find(y);
        ASSERT(it2 != it->second.end());
        return it2->second;
    }

    void addTrace(const vector<Token*>& st, const vector<Token*>& input, size_t ip, const string& action)
    {
        string s, i;
        for (Token* c : st)
        {
            s += c->name;
            s += " ";
        }
        for (size_t p = ip; p < input.size(); ++p)
        {
            i += input[p]->name;
            i += " ";
        }
        trace.emplace_back(s, i, action);
    }

    bool findProductionMatch(vector<Token*>::const_iterator first, vector<Token*>::const_iterator last, Token*& outA,
        vector<Token*>& outRhs) const
    {
        size_t len = (size_t)distance(first, last);
        for (const auto& pr : productions)
        {
            Token*                        A    = pr.first;
            const vector<vector<Token*>>& rhss = pr.second;
            for (const auto& rhs : rhss)
            {
                if (rhs.size() != len) continue;
                size_t k  = 0;
                auto   it = first;
                for (; k < len; ++k, ++it)
                    if (rhs[k] != *it) break;

                if (k != len) continue;

                outA   = A;
                outRhs = rhs;
                return true;
            }
        }
        return false;
    }

    map<Token*, vector<vector<Token*>>> productions;
    Token*                              startTok{nullptr};
    set<string>                         tokenStorage;
    vector<Token*>                      tokenPool;
    map<string_view, Token*>            nameToTok;

    Token* getToken(string_view name)
    {
        auto it = nameToTok.find(name);
        if (it != nameToTok.end()) return it->second;
        tokenPool.emplace_back(new Token(name, false));
        Token* p        = tokenPool.back();
        nameToTok[name] = p;
        return p;
    }

    void clear()
    {
        productions.clear();
        nterms.clear();
        terms.clear();
        firstVT.clear();
        lastVT.clear();
        relation.clear();
        tokenStorage.clear();
        for (Token* p : tokenPool)
        {
            if (!p) continue;
            delete p;
            p = nullptr;
        }
        tokenPool.clear();
        nameToTok.clear();
        startTok = nullptr;
    }

  public:
    bool setGrammar(const map<string, vector<vector<string>>>& prods, const string& startSym)
    {
        clear();
        string_view startSv = *tokenStorage.insert(startSym).first;

        set<string_view> lhsNames;
        set<string_view> allNames;
        for (const auto& [lhs, rhss] : prods)
        {
            string_view A = *tokenStorage.insert(lhs).first;
            lhsNames.insert(A);
            allNames.insert(A);
            for (const auto& rhs : rhss)
                for (const auto& tk : rhs) allNames.insert(*tokenStorage.insert(tk).first);
        }

        ASSERT(!lhsNames.empty());

        for (auto sv : allNames) getToken(sv);
        for (auto sv : lhsNames) nameToTok[sv]->isNTerm = true;

        for (const auto& [lhs, rhss] : prods)
        {
            Token* A = nameToTok[*tokenStorage.insert(lhs).first];
            for (const auto& rhs : rhss)
            {
                vector<Token*> rr;
                rr.reserve(rhs.size());
                for (const auto& tk : rhs) rr.push_back(nameToTok[*tokenStorage.insert(tk).first]);
                if (!rr.empty()) productions[A].push_back(std::move(rr));
            }
        }

        if (productions.empty()) return false;

        for (auto sv : lhsNames) nterms.insert(nameToTok[sv]);
        terms.clear();
        for (auto sv : allNames)
        {
            Token* t = nameToTok[sv];
            if (!t->isNTerm) terms.insert(t);
        }

        Token* sent   = getToken(*tokenStorage.insert(Token::Sentinel).first);
        sent->isNTerm = false;
        terms.insert(sent);
        startTok = nameToTok[startSv];
        ASSERT(startTok && startTok->isNTerm);

        computeVT(
            firstVT,
            [this](Token* A, const vector<Token*>& rhs, map<Token*, set<Token*>>& vt) {
                if (rhs.empty()) return;
                Token* t0 = rhs[0];
                if (!t0->isNTerm) { vt[A].insert(t0); }
                else if (t0->isNTerm && rhs.size() >= 2 && !rhs[1]->isNTerm) { vt[A].insert(rhs[1]); }
            },
            [this](Token*, const vector<Token*>& rhs) -> vector<Token*> {
                if (!rhs.empty() && rhs[0]->isNTerm) return {rhs[0]};
                return {};
            });
        computeVT(
            lastVT,
            [this](Token* A, const vector<Token*>& rhs, map<Token*, set<Token*>>& vt) {
                if (rhs.empty()) return;
                Token* tLast = rhs.back();
                if (!tLast->isNTerm) { vt[A].insert(tLast); }
                else if (tLast->isNTerm && rhs.size() >= 2 && !rhs[rhs.size() - 2]->isNTerm)
                {
                    vt[A].insert(rhs[rhs.size() - 2]);
                }
            },
            [this](Token*, const vector<Token*>& rhs) -> vector<Token*> {
                if (!rhs.empty() && rhs.back()->isNTerm) return {rhs.back()};
                return {};
            });

        buildRelMat();

        return true;
    }

  private:
    void computeVT(map<Token*, set<Token*>>&                                            vt,
        const function<void(Token*, const vector<Token*>&, map<Token*, set<Token*>>&)>& addDirect,
        const function<vector<Token*>(Token*, const vector<Token*>&)>&                  getChildren)
    {
        for (Token* A : nterms) vt[A] = {};

        for (const auto& [A, rhss] : productions)
            for (const auto& rhs : rhss) addDirect(A, rhs, vt);

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (const auto& [B, rhss] : productions)
                for (const auto& rhs : rhss)
                {
                    auto children = getChildren(B, rhs);
                    for (Token* A : children)
                    {
                        size_t before = vt[B].size();
                        vt[B].insert(vt[A].begin(), vt[A].end());
                        if (vt[B].size() != before) changed = true;
                    }
                }
        }
    }

    void insertRel(Token* a, Token* b, char rel)
    {
        if (relation[a].count(b) == 0)
            relation[a][b] = rel;
        else
            ASSERT(relation[a][b] == rel);
    }

    void buildRelMat()
    {
        relation.clear();
        tokenStorage.insert(Token::Sentinel);
        for (const auto& [P, rhss] : productions)
            for (const auto& rhs : rhss)
            {
                const int n = static_cast<int>(rhs.size());
                for (int i = 0; i <= n - 2; ++i)
                {
                    Token* Xi = rhs[i];
                    Token* Xj = rhs[i + 1];
                    if (!Xi->isNTerm && !Xj->isNTerm) insertRel(Xi, Xj, '=');
                    if (i <= n - 3)
                    {
                        Token* Xk = rhs[i + 2];
                        if (!Xi->isNTerm && Xj->isNTerm && !Xk->isNTerm) insertRel(Xi, Xk, '=');
                    }
                    if (!Xi->isNTerm && Xj->isNTerm)
                        for (Token* a : firstVT[Xj]) insertRel(Xi, a, '<');
                    if (Xi->isNTerm && !Xj->isNTerm)
                        for (Token* a : lastVT[Xi]) insertRel(a, Xj, '>');
                }
            }

        Token* sentinel = nameToTok[*tokenStorage.find(Token::Sentinel)];
        for (Token* a : firstVT[startTok]) insertRel(sentinel, a, '<');
        for (Token* a : lastVT[startTok]) insertRel(a, sentinel, '>');
    }

    int getTopTermIdx(const vector<Token*>& st) const
    {
        for (int i = static_cast<int>(st.size()) - 1; i >= 0; --i)
            if (!st[i]->isNTerm) return i;
        ERROR("No terminal found in stack");
    }

    bool findHandleBounds(const vector<Token*>& st, int& lfBoundTermIdx, int& subStrIdx)
    {
        vector<int> termIdx;
        for (int i = 0; i < static_cast<int>(st.size()); ++i)
            if (!st[i]->isNTerm) termIdx.push_back(i);
        if (static_cast<int>(termIdx.size()) < 2) return false;

        int i = static_cast<int>(termIdx.size()) - 1;

        while (i > 0)
        {
            Token* L = st[termIdx[i - 1]];
            Token* R = st[termIdx[i]];
            char   r = relationOf(L, R);
            if (r == '=')
                --i;
            else
                break;
        }

        if (i == 0)
        {
            lfBoundTermIdx = termIdx[0];
            subStrIdx      = lfBoundTermIdx + 1;
            return true;
        }

        Token* L = st[termIdx[i - 1]];
        Token* R = st[termIdx[i]];
        char   r = relationOf(L, R);
        if (r != '<' && L->name != Token::Sentinel) return false;
        lfBoundTermIdx = termIdx[i - 1];
        subStrIdx      = lfBoundTermIdx + 1;
        return true;
    }

    bool reduce(vector<Token*>& st, string& action)
    {
        int lfBoundTermIdx = -1;
        int start          = -1;
        ASSERT(findHandleBounds(st, lfBoundTermIdx, start));
        ASSERT(start <= static_cast<int>(st.size()) - 1);

        Token*         A = nullptr;
        vector<Token*> rhs;
        auto           beginIt = st.begin() + start;
        auto           endIt   = st.end();
        bool           matched = findProductionMatch(beginIt, endIt, A, rhs);
        for (int shrink = 1; !matched && (start + shrink) < static_cast<int>(st.size()); ++shrink)
        {
            auto b2 = st.begin() + start + shrink;
            matched = findProductionMatch(b2, st.end(), A, rhs);
            if (matched) beginIt = b2;
        }

        ASSERT(matched);

        for (size_t k = 0; k < rhs.size(); ++k) st.pop_back();
        st.push_back(A);

        string rhsStr;
        for (auto sv : rhs)
        {
            rhsStr += sv->name;
            rhsStr += " ";
        }
        if (!rhsStr.empty()) rhsStr.pop_back();
        action = "reduce " + string(A->name) + " -> " + rhsStr;
        return true;
    }

  public:
    bool parse(const vector<string>& rawInputStr)
    {
        vector<Token*> input;
        input.reserve(rawInputStr.size() + 1);
        for (const string& s : rawInputStr)
        {
            string_view c  = *tokenStorage.insert(s).first;
            auto        it = nameToTok.find(c);
            ASSERT(it != nameToTok.end() && !it->second->isNTerm && c != Token::Sentinel);
            input.push_back(it->second);
        }
        input.push_back(nameToTok[*tokenStorage.insert(Token::Sentinel).first]);

        vector<Token*> st;
        st.push_back(nameToTok[*tokenStorage.insert(Token::Sentinel).first]);
        size_t ip = 0;

        trace.clear();

        bool accepted = false;
        bool stop     = false;
        while (!stop)
        {
            Token* b    = input[ip];
            int    idxA = getTopTermIdx(st);
            ASSERT(idxA >= 0);
            Token* a = st[idxA];

            if (a->name == Token::Sentinel && b->name == Token::Sentinel && st.size() == 2 &&
                st[0]->name == Token::Sentinel && st[1] == startTok)
            {
                addTrace(st, input, ip, "accept");
                accepted = true;
                break;
            }

            if (b->name == Token::Sentinel)
            {
                string action;
                if (st.size() == 2 && st[0]->name == Token::Sentinel && st[1]->isNTerm)
                {
                    Token* X       = st[1];
                    bool   chained = false;
                    for (const auto& [A, rhss] : productions)
                    {
                        for (const auto& rhs : rhss)
                        {
                            if (rhs.size() != 1 || rhs[0] != X) continue;
                            st.pop_back();
                            st.push_back(A);
                            action = string("reduce ") + string(A->name) + " -> " + string(X->name);
                            addTrace(st, input, ip, action);
                            chained = true;
                            break;
                        }
                        if (chained) break;
                    }
                    if (chained) continue;
                }

                bool ok = reduce(st, action);
                addTrace(st, input, ip, action);
                if (ok) continue;
                if (st.size() == 2 && st[0]->name == Token::Sentinel && st[1] == startTok)
                {
                    addTrace(st, input, ip, "accept");
                    return true;
                }
                ASSERT(false);
            }

            char   r = relationOf(a, b);
            string action;
            if (r == '<' || r == '=')
            {
                st.push_back(b);
                ++ip;
                action = string("shift '") + string(b->name) + "'";
                addTrace(st, input, ip, action);
                continue;
            }
            if (r == '>')
            {
                bool ok = reduce(st, action);
                addTrace(st, input, ip, action);
                ASSERT(ok);
                continue;
            }
            ERROR("undefined precedence");
        }

        return false;
    }
};

void loadGrammer(
    istream& in, map<string, vector<vector<string>>>& prodsOut, string& startSymbolOut, vector<string>& inputOut)
{
    prodsOut.clear();
    startSymbolOut.clear();
    inputOut.clear();

    string line;
    bool   startSet = false;
    bool   afterSep = false;
    bool   gotInput = false;
    while (getline(in, line))
    {
        string t = trim(line);
        if (t.empty()) continue;
        if (t == "%%")
        {
            afterSep = true;
            continue;
        }

        if (!afterSep)
        {
            if (t.rfind("%start", 0) == 0)
            {
                string rest = trim(t.substr(6));
                if (!rest.empty())
                {
                    startSymbolOut = rest;
                    startSet       = true;
                }
                continue;
            }

            size_t pos = t.find("->");
            if (pos == string::npos) continue;
            string lhs = trim(t.substr(0, pos));
            string rhs = trim(t.substr(pos + 2));
            if (lhs.empty() || rhs.empty()) continue;
            if (!startSet)
            {
                startSymbolOut = lhs;
                startSet       = true;
            }
            stringstream ss(rhs);
            string       alt;
            while (getline(ss, alt, '|'))
            {
                alt = trim(alt);
                if (alt.empty()) continue;
                stringstream   ts(alt);
                string         tok;
                vector<string> rhsVec;
                while (ts >> tok) rhsVec.push_back(tok);
                if (!rhsVec.empty()) prodsOut[lhs].push_back(rhsVec);
            }
        }
        else if (!gotInput)
        {
            stringstream ts(t);
            string       tok;
            while (ts >> tok) inputOut.push_back(tok);
            if (!inputOut.empty())
            {
                gotInput = true;
                break;
            }
        }
    }
    ASSERT(!prodsOut.empty());
    ASSERT(startSet);
    ASSERT(afterSep);
    ASSERT(gotInput);
}

int main()
{
    map<string, vector<vector<string>>> prods;
    string                              startSym;
    vector<string>                      inputTokens;
    loadGrammer(cin, prods, startSym, inputTokens);

    OPGParser opg;
    if (!opg.setGrammar(prods, startSym)) return 1;

    cout << "FIRSTVT:\n";
    for (const auto& [A, s] : opg.firstVT)
    {
        cout << "  " << A->name << " : { ";
        for (auto x : s) cout << x->name << " ";
        cout << "}\n";
    }
    cout << "LASTVT:\n";
    for (const auto& [A, s] : opg.lastVT)
    {
        cout << "  " << A->name << " : { ";
        for (auto x : s) cout << x->name << " ";
        cout << "}\n";
    }

    {
        vector<OPGParser::Token*> vt(opg.terms.begin(), opg.terms.end());
        sort(vt.begin(), vt.end(), [](OPGParser::Token* a, OPGParser::Token* b) { return a->name < b->name; });
        cout << "\nRelation Matrix:\n    ";
        for (auto b : vt) cout << b->name << " ";
        cout << "\n";
        for (auto a : vt)
        {
            cout << "  " << a->name << " ";
            for (auto b : vt)
            {
                auto it1 = opg.relation.find(a);
                char r   = ' ';
                if (it1 != opg.relation.end())
                {
                    auto it2 = it1->second.find(b);
                    if (it2 != it1->second.end()) r = it2->second;
                }
                cout << (r ? r : ' ') << " ";
            }
            cout << "\n";
        }
    }

    cout << "\nInput: ";
    for (const auto& s : inputTokens) cout << s << " ";
    cout << "\n";

    opg.parse(inputTokens);

    size_t maxStkLen   = 0;
    size_t maxInputLen = 0;
    for (const auto& [s, i, a] : opg.trace)
    {
        if (s.length() > maxStkLen) maxStkLen = s.length();
        if (i.length() > maxInputLen) maxInputLen = i.length();
    }

    cout << "\nstack" << string(maxStkLen - 3, ' ') << "| " << "input" << string(maxInputLen - 3, ' ') << "| "
         << "action" << "\n";
    for (const auto& [s, i, a] : opg.trace)
        cout << left << setw(maxStkLen + 2) << s << "| " << left << setw(maxInputLen + 2) << i << "| " << a << "\n";
}
