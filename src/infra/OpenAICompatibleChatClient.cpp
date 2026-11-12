#include "localrag/infra/OpenAICompatibleChatClient.h"
#include <windows.h>
#include <winhttp.h>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace localrag {

namespace {

std::wstring widen(const std::string& value) {
    if (value.empty()) {
        return L"";
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        throw std::runtime_error("failed to convert utf8 to wide string");
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

struct ParsedUrl {
    std::wstring host;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
    std::wstring path;
    bool https = true;
};

ParsedUrl parseUrl(const std::string& baseUrl) {
    const std::wstring wide = widen(baseUrl);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(wide.c_str(), static_cast<DWORD>(wide.size()), 0, &parts)) {
        throw std::runtime_error("invalid base_url");
    }

    ParsedUrl parsed;
    parsed.host.assign(parts.lpszHostName, parts.dwHostNameLength);
    parsed.port = parts.nPort;
    parsed.https = parts.nScheme == INTERNET_SCHEME_HTTPS;
    parsed.path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parsed.path.empty() || parsed.path == L"/") {
        parsed.path = L"/v1";
    }
    if (!parsed.path.empty() && parsed.path.back() == L'/') {
        parsed.path.pop_back();
    }
    parsed.path += L"/chat/completions";
    return parsed;
}

std::string buildRequestBody(const OpenAIChatConfig& config, const std::vector<ChatMessage>& messages) {
    std::ostringstream body;
    body << "{\"model\":\"" << escapeJsonString(config.model) << "\",\"messages\":[";
    for (std::size_t i = 0; i < messages.size(); ++i) {
        if (i != 0) {
            body << ",";
        }
        body << "{\"role\":\"" << escapeJsonString(messages[i].role) << "\",\"content\":\"" << escapeJsonString(messages[i].content) << "\"}";
    }
    body << "]}";
    return body.str();
}

std::string readResponse(HINTERNET request) {
    std::string response;
    DWORD available = 0;
    do {
        if (!WinHttpQueryDataAvailable(request, &available)) {
            throw std::runtime_error("failed to query response size");
        }
        if (available == 0) {
            break;
        }

        std::string buffer(available, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &read)) {
            throw std::runtime_error("failed to read response");
        }
        buffer.resize(read);
        response += buffer;
    } while (available > 0);

    return response;
}

std::string unescapeJsonString(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '\\' || i + 1 >= value.size()) {
            result += value[i];
            continue;
        }
        const char escaped = value[++i];
        switch (escaped) {
        case 'n':
            result += '\n';
            break;
        case 'r':
            result += '\r';
            break;
        case 't':
            result += '\t';
            break;
        case '"':
            result += '"';
            break;
        case '\\':
            result += '\\';
            break;
        default:
            result += escaped;
            break;
        }
    }
    return result;
}

} // namespace

std::string escapeJsonString(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (std::size_t index = 0; index < value.size();) {
        const auto byte = static_cast<unsigned char>(value[index]);
        std::size_t length = 0;
        if (byte <= 0x7FU) {
            length = 1;
        } else if (byte >= 0xC2U && byte <= 0xDFU) {
            length = 2;
        } else if (byte >= 0xE0U && byte <= 0xEFU) {
            length = 3;
        } else if (byte >= 0xF0U && byte <= 0xF4U) {
            length = 4;
        }

        bool validUtf8 = length > 0 && index + length <= value.size();
        for (std::size_t offset = 1; validUtf8 && offset < length; ++offset) {
            validUtf8 = (static_cast<unsigned char>(value[index + offset]) & 0xC0U) == 0x80U;
        }
        if (validUtf8 && length == 3) {
            const auto second = static_cast<unsigned char>(value[index + 1]);
            validUtf8 = (byte != 0xE0U || second >= 0xA0U) && (byte != 0xEDU || second <= 0x9FU);
        }
        if (validUtf8 && length == 4) {
            const auto second = static_cast<unsigned char>(value[index + 1]);
            validUtf8 = (byte != 0xF0U || second >= 0x90U) && (byte != 0xF4U || second <= 0x8FU);
        }
        if (!validUtf8) {
            escaped += "\xEF\xBF\xBD";
            ++index;
            continue;
        }

        if (length != 1) {
            escaped.append(value, index, length);
            index += length;
            continue;
        }

        const char ch = value[index++];
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20U) {
                constexpr char hex[] = "0123456789ABCDEF";
                escaped += "\\u00";
                escaped += hex[(static_cast<unsigned char>(ch) >> 4U) & 0x0FU];
                escaped += hex[static_cast<unsigned char>(ch) & 0x0FU];
            } else {
                escaped += ch;
            }
            break;
        }
    }
    return escaped;
}

std::string extractChatCompletionContent(const std::string& responseBody) {
    const std::string marker = "\"content\"";
    const auto markerPos = responseBody.find(marker);
    if (markerPos == std::string::npos) {
        throw std::runtime_error("chat response does not contain message content: " + responseBody.substr(0, 300));
    }

    const auto colonPos = responseBody.find(':', markerPos + marker.size());
    const auto quoteStart = responseBody.find('"', colonPos + 1);
    if (colonPos == std::string::npos || quoteStart == std::string::npos) {
        throw std::runtime_error("chat response content is malformed");
    }

    std::string raw;
    bool escaping = false;
    for (std::size_t i = quoteStart + 1; i < responseBody.size(); ++i) {
        const char ch = responseBody[i];
        if (escaping) {
            raw += '\\';
            raw += ch;
            escaping = false;
            continue;
        }
        if (ch == '\\') {
            escaping = true;
            continue;
        }
        if (ch == '"') {
            return unescapeJsonString(raw);
        }
        raw += ch;
    }

    throw std::runtime_error("chat response content string is not closed");
}

OpenAICompatibleChatClient::OpenAICompatibleChatClient(OpenAIChatConfig config)
    : config_(std::move(config)) {}

std::string OpenAICompatibleChatClient::complete(const std::vector<ChatMessage>& messages) {
    if (config_.apiKey.empty()) {
        throw std::runtime_error("api key is empty");
    }
    if (config_.model.empty()) {
        throw std::runtime_error("chat model is empty");
    }

    const auto url = parseUrl(config_.baseUrl);
    const auto body = buildRequestBody(config_, messages);

    HINTERNET session = WinHttpOpen(L"LocalRAG Agent/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        throw std::runtime_error("failed to open WinHTTP session");
    }

    HINTERNET connection = WinHttpConnect(session, url.host.c_str(), url.port, 0);
    if (!connection) {
        WinHttpCloseHandle(session);
        throw std::runtime_error("failed to connect to API host");
    }

    const DWORD flags = url.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connection, L"POST", url.path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("failed to open HTTP request");
    }

    const std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer " + widen(config_.apiKey) + L"\r\n";
    const BOOL sent = WinHttpSendRequest(
        request,
        headers.c_str(),
        static_cast<DWORD>(headers.size()),
        const_cast<char*>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0);
    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        throw std::runtime_error("failed to send chat completion request");
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    const auto response = readResponse(request);
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    if (statusCode < 200 || statusCode >= 300) {
        throw std::runtime_error("chat API returned HTTP " + std::to_string(statusCode) + ": " + response.substr(0, 500));
    }

    return extractChatCompletionContent(response);
}

} // namespace localrag
