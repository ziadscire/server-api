#include "httplib.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <cstdio>
#include <unordered_map>

using namespace std;

// =============================================================
//  CORE LOGIC  (identical to original)
// =============================================================

string printDescription(int inst) {
    ostringstream oss;
    oss << "Result: 0x"
        << hex << uppercase
        << setw(4) << setfill('0')
        << inst << " -> ";

    switch (inst) {
        case 0x7800: oss << "CLA: Clear Accumulator";         return oss.str();
        case 0x7400: oss << "CLE: Clear E";                   return oss.str();
        case 0x7200: oss << "CMA: Complement Accumulator";    return oss.str();
        case 0x7100: oss << "CME: Complement E";              return oss.str();
        case 0x7080: oss << "CIR: Circulate Right AC and E";  return oss.str();
        case 0x7040: oss << "CIL: Circulate Left AC and E";   return oss.str();
        case 0x7020: oss << "INC: Increment Accumulator";     return oss.str();
        case 0x7010: oss << "SPA: Skip if AC is Positive";    return oss.str();
        case 0x7008: oss << "SNA: Skip if AC is Negative";    return oss.str();
        case 0x7004: oss << "SZA: Skip if AC is Zero";        return oss.str();
        case 0x7002: oss << "SZE: Skip if E is Zero";         return oss.str();
        case 0x7001: oss << "HLT: Halt Computer";             return oss.str();
        case 0xF800: oss << "INP: Input character to AC";     return oss.str();
        case 0xF400: oss << "OUT: Output character from AC";  return oss.str();
        case 0xF200: oss << "SKI: Skip on input flag";        return oss.str();
        case 0xF100: oss << "SKO: Skip on output flag";       return oss.str();
        case 0xF080: oss << "ION: Interrupt On";              return oss.str();
        case 0xF040: oss << "IOF: Interrupt Off";             return oss.str();
    }

    int opcode = (inst >> 12) & 0x7;
    int I      = (inst >> 15) & 0x1;
    int addr   = inst & 0x0FFF;
    string mode = (I == 1) ? "Indirect" : "Direct";
    string ops[] = {"AND","ADD","LDA","STA","BUN","BSA","ISZ"};

    if (opcode < 7)
        oss << ops[opcode] << " at 0x" << hex << addr << " (" << mode << ")";
    else
        oss << "Unknown Instruction";

    return oss.str();
}

string interpret(string code) {
    try {
        if      (code.length() == 16) return printDescription(stoi(code, nullptr, 2));
        else if (code.length() ==  4) return printDescription(stoi(code, nullptr, 16));
        else    return "ERROR: Invalid length. Expected 4-digit HEX or 16-bit Binary.";
    }
    catch (...) {
        return "ERROR: Could not parse the instruction.";
    }
}

// =============================================================
//  JSON helpers
// =============================================================

string jsonEscape(const string& s) {
    string out;
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "";
        else                out += c;
    }
    return out;
}

// Parse a single value from a flat JSON object: {"key":"value"}
string jsonGet(const string& body, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = body.find(search);
    if (pos == string::npos) return "";
    pos = body.find(':', pos);
    if (pos == string::npos) return "";
    pos = body.find('"', pos);
    if (pos == string::npos) return "";
    size_t start = pos + 1;
    size_t end = body.find('"', start);
    while (end != string::npos && body[end-1] == '\\') end = body.find('"', end+1);
    if (end == string::npos) return "";
    return body.substr(start, end - start);
}

// =============================================================
//  Route handlers  (same logic as original)
// =============================================================

// POST /api/part1   body: {"content":"7800\nF400\n..."}
string handlePart1(const string& body) {
    string content = jsonGet(body, "content");
    if (content.empty()) return "{\"error\":\"No content provided\"}";

    // unescape \n sequences coming from JSON
    string lines_raw;
    for (size_t i = 0; i < content.size(); i++) {
        if (content[i] == '\\' && i+1 < content.size() && content[i+1] == 'n') {
            lines_raw += '\n'; i++;
        } else lines_raw += content[i];
    }

    vector<string> results;
    istringstream ss(lines_raw);
    string line;
    while (getline(ss, line)) {
        if (line.empty()) continue;
        string code = line;
        for (auto& c : code) c = toupper(c);
        string res = interpret(code);
        string entry = "{\"input\":\"" + jsonEscape(code) + "\",\"result\":\"" + jsonEscape(res) + "\"}";
        results.push_back(entry);
    }

    string json = "{\"count\":" + to_string(results.size()) + ",\"results\":[";
    for (size_t i = 0; i < results.size(); i++) {
        json += results[i];
        if (i + 1 < results.size()) json += ",";
    }
    json += "]}";
    return json;
}

// POST /api/part2   body: {"code":"7800"}
string handlePart2(const string& body) {
    string code = jsonGet(body, "code");
    if (code.empty()) return "{\"error\":\"No code provided\"}";
    for (auto& c : code) c = toupper(c);
    string res = interpret(code);
    return "{\"input\":\"" + jsonEscape(code) + "\",\"result\":\"" + jsonEscape(res) + "\"}";
}

// POST /api/part3   body: {"symbol":"ADD","address":"0A1","mode":"D"}
string handlePart3(const string& body) {
    string symbol = jsonGet(body, "symbol");
    if (symbol.empty()) return "{\"error\":\"No symbol provided\"}";
    for (auto& c : symbol) c = toupper(c);

    unordered_map<string,int> regMap = {
        {"CLA",0x7800},{"CLE",0x7400},{"CMA",0x7200},{"CME",0x7100},
        {"CIR",0x7080},{"CIL",0x7040},{"INC",0x7020},{"SPA",0x7010},
        {"SNA",0x7008},{"SZA",0x7004},{"SZE",0x7002},{"HLT",0x7001},
        {"INP",0xF800},{"OUT",0xF400},{"SKI",0xF200},{"SKO",0xF100},
        {"ION",0xF080},{"IOF",0xF040}
    };
    unordered_map<string,int> memMap = {
        {"AND",0},{"ADD",1},{"LDA",2},{"STA",3},{"BUN",4},{"BSA",5},{"ISZ",6}
    };

    int inst_code = -1;

    if (regMap.count(symbol)) {
        inst_code = regMap[symbol];
    } else if (memMap.count(symbol)) {
        string addrHex = jsonGet(body, "address");
        string modeStr = jsonGet(body, "mode");
        if (addrHex.empty()) return "{\"error\":\"Address required for memory instruction\"}";
        for (auto& c : modeStr) c = toupper(c);
        int address, opcode = memMap[symbol];
        try { address = stoi(addrHex, nullptr, 16) & 0x0FFF; }
        catch (...) { return "{\"error\":\"Invalid hex address\"}"; }
        int addrMode = (modeStr == "I") ? 1 : 0;
        inst_code = (addrMode << 15) | (opcode << 12) | address;
    } else {
        return "{\"error\":\"Unknown symbol: " + symbol + "\"}";
    }

    char buffer[5];
    sprintf(buffer, "%04X", inst_code);
    string hex4(buffer);
    string res = interpret(hex4);
    return "{\"symbol\":\"" + symbol + "\",\"hex\":\"" + hex4 + "\",\"result\":\"" + jsonEscape(res) + "\"}";
}

// =============================================================
//  Helper: add CORS headers to every response
// =============================================================
void setCORS(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// =============================================================
//  main
// =============================================================
int main() {
    httplib::Server server;

    // ── CORS preflight (OPTIONS) for all /api/* routes ──────
    server.Options("/api/.*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    // ── POST /api/part1 ──────────────────────────────────────
    server.Post("/api/part1", [](const httplib::Request& req, httplib::Response& res) {
        string body = handlePart1(req.body);
        res.set_content(body, "application/json");
        setCORS(res);
    });

    // ── POST /api/part2 ──────────────────────────────────────
    server.Post("/api/part2", [](const httplib::Request& req, httplib::Response& res) {
        string body = handlePart2(req.body);
        res.set_content(body, "application/json");
        setCORS(res);
    });

    // ── POST /api/part3 ──────────────────────────────────────
    server.Post("/api/part3", [](const httplib::Request& req, httplib::Response& res) {
        string body = handlePart3(req.body);
        res.set_content(body, "application/json");
        setCORS(res);
    });

    // ── Start ─────────────────────────────────────────────────
    const int PORT = 8080;
    cout << "========================================\n";
    cout << "  Instruction Set Interpreter - Server  \n";
    cout << "  Running on http://localhost:" << PORT << "\n";
    cout << "  Open index.html in your browser.\n";
    cout << "  Press Ctrl+C to stop.\n";
    cout << "========================================\n";

    if (!server.listen("localhost", PORT)) {
        cerr << "ERROR: Could not start server on port " << PORT << "\n";
        return 1;
    }

    return 0;
}
