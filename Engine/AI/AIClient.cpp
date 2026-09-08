#include "AIClient.h"
#include "Core/Log.h"

#include <sstream>
#include <vector>

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

namespace Muk {

static std::string EscapeJson(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '\\': o += "\\\\"; break;
            case '"': o += "\\\""; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) continue;
                o += c;
                break;
        }
    }
    return o;
}

static std::string ExtractJsonStringField(const std::string& json, const char* field) {
    std::string key = std::string("\"") + field + "\"";
    auto pos = json.find(key);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos);
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos);
    if (pos == std::string::npos) return {};
    size_t start = pos + 1;
    std::string out;
    for (size_t i = start; i < json.size(); ++i) {
        if (json[i] == '\\' && i + 1 < json.size()) {
            char n = json[i + 1];
            if (n == 'n') out += '\n';
            else if (n == '"') out += '"';
            else if (n == '\\') out += '\\';
            else if (n == 't') out += '\t';
            else out += n;
            ++i;
            continue;
        }
        if (json[i] == '"') break;
        out += json[i];
    }
    return out;
}

std::string AIClient::BuildRequestBody(const std::vector<AIMessage>& messages, float temperature) const {
    std::ostringstream oss;
    oss << "{"
        << "\"model\":\"" << EscapeJson(m_Settings.ActiveModel()) << "\","
        << "\"temperature\":" << temperature << ","
        << "\"max_tokens\":1024,"
        << "\"messages\":[";
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i) oss << ",";
        oss << "{\"role\":\"" << EscapeJson(messages[i].Role)
            << "\",\"content\":\"" << EscapeJson(messages[i].Content) << "\"}";
    }
    oss << "]}";
    return oss.str();
}

AIResponse AIClient::HttpPostJson(const std::string& url, const std::string& apiKey, const std::string& body) {
    AIResponse result;
#ifdef MUK_PLATFORM_WINDOWS
    std::wstring wurl(url.begin(), url.end());
    URL_COMPONENTS uc = {};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256] = {};
    wchar_t path[2048] = {};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) {
        result.Error = "Invalid URL: " + url;
        return result;
    }

    HINTERNET hSession = WinHttpOpen(L"MukGameEngine/0.6",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { result.Error = "WinHttpOpen failed"; return result; }

    // Timeouts: resolve/connect/send/receive (ms)
    WinHttpSetTimeouts(hSession, 15000, 15000, 30000, 60000);

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        result.Error = "WinHttpConnect failed";
        return result;
    }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path, nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        result.Error = "WinHttpOpenRequest failed";
        return result;
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    headers += L"Authorization: Bearer ";
    headers += std::wstring(apiKey.begin(), apiKey.end());
    headers += L"\r\n";
    // OpenRouter recommends these optional headers
    headers += L"HTTP-Referer: https://github.com/JagX-JRILICENSE/MukGameEngine\r\n";
    headers += L"X-Title: Muk Game Engine\r\n";

    BOOL ok = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1,
        (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        result.Error = "HTTP request failed (network / TLS)";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

    std::string response;
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
        std::vector<char> buf(available);
        DWORD read = 0;
        if (!WinHttpReadData(hRequest, buf.data(), available, &read) || read == 0) break;
        response.append(buf.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    result.RawJson = response;

    if (status == 401 || status == 403) {
        result.Error = "Auth failed (HTTP " + std::to_string(status) + ") — check API key";
        auto msg = ExtractJsonStringField(response, "message");
        if (msg.empty()) msg = ExtractJsonStringField(response, "error");
        if (!msg.empty()) result.Error += ": " + msg;
        return result;
    }
    if (status == 429) {
        result.Error = "Rate limited (HTTP 429) — free tiers are limited; wait and retry";
        return result;
    }
    if (status >= 400) {
        auto msg = ExtractJsonStringField(response, "message");
        if (msg.empty()) msg = ExtractJsonStringField(response, "error");
        result.Error = "HTTP " + std::to_string(status);
        if (!msg.empty()) result.Error += ": " + msg;
        else if (response.size() < 400) result.Error += ": " + response;
        else result.Error += " — check model id / key";
        return result;
    }

    // Prefer choices[0].message.content
    auto choices = response.find("\"choices\"");
    std::string content;
    if (choices != std::string::npos) {
        auto msg = response.find("\"message\"", choices);
        if (msg != std::string::npos)
            content = ExtractJsonStringField(response.substr(msg), "content");
    }
    if (content.empty())
        content = ExtractJsonStringField(response, "content");

    if (!content.empty()) {
        result.Content = content;
        result.Success = true;
        return result;
    }

    result.Error = "Could not parse assistant content";
    if (response.find("\"error\"") != std::string::npos) {
        auto msg = ExtractJsonStringField(response, "message");
        if (!msg.empty()) result.Error = msg;
    }
#else
    (void)url; (void)apiKey; (void)body;
    result.Error = "AI client only on Windows (WinHTTP)";
#endif
    return result;
}

AIResponse AIClient::Chat(const std::vector<AIMessage>& messages, float temperature) {
    if (!m_Settings.HasAnyKey()) {
        AIResponse r;
        r.Error = "No API key. OpenRouter: openrouter.ai/keys | NVIDIA: build.nvidia.com → Get API Key";
        return r;
    }
    if (m_Settings.ActiveModel().empty()) {
        AIResponse r;
        r.Error = "No model selected — pick a FREE model in the AI panel";
        return r;
    }

    std::string base = m_Settings.ActiveBaseUrl();
    if (base.empty()) {
        AIResponse r;
        r.Error = "Base URL empty";
        return r;
    }
    if (base.back() == '/') base.pop_back();
    std::string url = base + "/chat/completions";
    std::string body = BuildRequestBody(messages, temperature);
    MUK_CORE_INFO("AI request {0} model={1}", url.c_str(), m_Settings.ActiveModel().c_str());
    return HttpPostJson(url, m_Settings.ActiveApiKey(), body);
}

AIResponse AIClient::AskEngineControl(const std::string& userPrompt) {
    std::vector<AIMessage> msgs;
    msgs.push_back({
        "system",
        "You are Muk Engine AI Assistant for a C++/DX12 game engine. "
        "Be practical and concise. When changing the scene, emit ACTION lines:\n"
        "ACTION: set_camera eye=0,2,-5 target=0,0,0\n"
        "ACTION: spawn_cube x=1 y=0 z=0\n"
    });
    msgs.push_back({ "user", userPrompt });
    return Chat(msgs);
}

} // namespace Muk
