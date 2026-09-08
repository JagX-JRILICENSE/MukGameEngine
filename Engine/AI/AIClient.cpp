#include "AIClient.h"
#include "Core/Log.h"

#include <sstream>

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
            default: o += c; break;
        }
    }
    return o;
}

std::string AIClient::BuildRequestBody(const std::vector<AIMessage>& messages, float temperature) const {
    std::ostringstream oss;
    oss << "{"
        << "\"model\":\"" << EscapeJson(m_Settings.ActiveModel()) << "\","
        << "\"temperature\":" << temperature << ","
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
    // Parse URL
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
        result.Error = "Invalid URL";
        return result;
    }

    HINTERNET hSession = WinHttpOpen(L"MukGameEngine/0.4",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { result.Error = "WinHttpOpen failed"; return result; }

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

    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer ";
    headers += std::wstring(apiKey.begin(), apiKey.end());
    headers += L"\r\n";

    BOOL ok = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)headers.size(),
        (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        result.Error = "HTTP request failed";
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    std::string response;
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
        std::vector<char> buf(available + 1);
        DWORD read = 0;
        WinHttpReadData(hRequest, buf.data(), available, &read);
        response.append(buf.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    result.RawJson = response;

    // Very small JSON extract of choices[0].message.content
    auto pos = response.find("\"content\"");
    if (pos != std::string::npos) {
        pos = response.find(':', pos);
        if (pos != std::string::npos) {
            pos = response.find('"', pos);
            if (pos != std::string::npos) {
                size_t start = pos + 1;
                std::string content;
                for (size_t i = start; i < response.size(); ++i) {
                    if (response[i] == '\\' && i + 1 < response.size()) {
                        char n = response[i + 1];
                        if (n == 'n') content += '\n';
                        else if (n == '"') content += '"';
                        else if (n == '\\') content += '\\';
                        else content += n;
                        ++i;
                        continue;
                    }
                    if (response[i] == '"') break;
                    content += response[i];
                }
                result.Content = content;
                result.Success = true;
                return result;
            }
        }
    }

    if (response.find("\"error\"") != std::string::npos) {
        result.Error = "Provider error — check key/model (see raw JSON)";
    } else {
        result.Error = "Could not parse assistant content";
    }
#else
    (void)url; (void)apiKey; (void)body;
    result.Error = "AI client only implemented on Windows (WinHTTP)";
#endif
    return result;
}

AIResponse AIClient::Chat(const std::vector<AIMessage>& messages, float temperature) {
    if (!m_Settings.HasAnyKey()) {
        AIResponse r;
        r.Error = "No API key set. Add your key in settings.ini (OpenRouter / NVIDIA / OpenAI / custom).";
        return r;
    }

    std::string base = m_Settings.ActiveBaseUrl();
    if (!base.empty() && base.back() == '/') base.pop_back();
    std::string url = base + "/chat/completions";
    std::string body = BuildRequestBody(messages, temperature);
    return HttpPostJson(url, m_Settings.ActiveApiKey(), body);
}

AIResponse AIClient::AskEngineControl(const std::string& userPrompt) {
    std::vector<AIMessage> msgs;
    msgs.push_back({
        "system",
        "You are Muk Engine AI Assistant. Help the user control and design a C++/DX12 game engine "
        "similar in ambition to Unreal. Reply with clear steps, code sketches, or editor actions. "
        "If the user asks to change scene parameters, respond with a short ACTION block like:\n"
        "ACTION: set_camera eye=0,2,-5 target=0,0,0\n"
        "ACTION: spawn_cube x=1 y=0 z=0\n"
        "Keep answers practical and concise."
    });
    msgs.push_back({ "user", userPrompt });
    return Chat(msgs);
}

} // namespace Muk
