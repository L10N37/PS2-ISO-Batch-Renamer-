#include "core/OnlineDatabase.h"

#include "core/Database.h"

#include <limits>
#include <string>
#include <utility>

#if PS2BR_ENABLE_ONLINE_DATABASE && defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#elif PS2BR_ENABLE_ONLINE_DATABASE
#include <curl/curl.h>
#endif

namespace ps2br {
namespace {

OnlineDatabaseResult validatedResult(std::string text) {
    OnlineDatabaseResult result;
    if (!validateOnlineDatabase(text, result.error)) {
        return result;
    }
    result.success = true;
    result.text = std::move(text);
    return result;
}

#if PS2BR_ENABLE_ONLINE_DATABASE && defined(_WIN32)

class InternetHandle {
public:
    explicit InternetHandle(HINTERNET value = nullptr) : value_(value) {}
    ~InternetHandle() { if (value_) WinHttpCloseHandle(value_); }
    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;
    operator HINTERNET() const { return value_; }
    explicit operator bool() const { return value_ != nullptr; }
private:
    HINTERNET value_ = nullptr;
};

std::string windowsError(const char* operation) {
    return std::string(operation) + " failed (Windows error " + std::to_string(GetLastError()) + ")";
}

#elif PS2BR_ENABLE_ONLINE_DATABASE

struct CurlWriteState {
    std::string* text = nullptr;
    bool oversized = false;
};

std::size_t curlWrite(char* data, std::size_t size, std::size_t count, void* userdata) {
    auto* state = static_cast<CurlWriteState*>(userdata);
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
        state->oversized = true;
        return 0;
    }
    const std::size_t bytes = size * count;
    if (state->text->size() > kOnlineDatabaseMaximumBytes ||
        bytes > kOnlineDatabaseMaximumBytes - state->text->size()) {
        state->oversized = true;
        return 0;
    }
    state->text->append(data, bytes);
    return bytes;
}

#endif

} // namespace

bool validateOnlineDatabase(std::string_view text, std::string& error) {
    if (text.size() > kOnlineDatabaseMaximumBytes) {
        error = "Downloaded database exceeds the 4 MiB safety limit";
        return false;
    }
    GameDatabase candidate;
    if (!candidate.loadText(text, "Downloaded online database", error)) {
        return false;
    }
    if (candidate.recordCount() < kOnlineDatabaseMinimumRecords) {
        error = "Downloaded database appears incomplete (fewer than " +
                std::to_string(kOnlineDatabaseMinimumRecords) + " records)";
        return false;
    }
    if (candidate.recordCount() != candidate.uniqueIdCount()) {
        error = "Downloaded database contains duplicate game IDs";
        return false;
    }
    error.clear();
    return true;
}

OnlineDatabaseResult downloadOnlineDatabase() {
#if !PS2BR_ENABLE_ONLINE_DATABASE
    return {false, {}, "Online database support is disabled in this build"};
#elif defined(_WIN32)
    InternetHandle session(WinHttpOpen(L"PS2-Batch-Renamer/4.1",
                                       WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session) return {false, {}, windowsError("WinHttpOpen")};
    WinHttpSetTimeouts(session, 3000, 3000, 3000, 8000);

    InternetHandle connection(WinHttpConnect(session, L"raw.githubusercontent.com",
                                              INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!connection) return {false, {}, windowsError("WinHttpConnect")};

    InternetHandle request(WinHttpOpenRequest(
        connection, L"GET", L"/L10N37/PS2-ISO-Batch-Renamer-/main/gameid.txt",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (!request) return {false, {}, windowsError("WinHttpOpenRequest")};
    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        return {false, {}, windowsError("WinHttpSendRequest")};
    }
    if (!WinHttpReceiveResponse(request, nullptr)) {
        return {false, {}, windowsError("WinHttpReceiveResponse")};
    }
    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size,
                             WINHTTP_NO_HEADER_INDEX)) {
        return {false, {}, windowsError("WinHttpQueryHeaders")};
    }
    if (status != 200) {
        return {false, {}, "Online database returned HTTP " + std::to_string(status)};
    }

    std::string text;
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            return {false, {}, windowsError("WinHttpQueryDataAvailable")};
        }
        if (available == 0) break;
        if (text.size() > kOnlineDatabaseMaximumBytes ||
            static_cast<std::size_t>(available) > kOnlineDatabaseMaximumBytes - text.size()) {
            return {false, {}, "Downloaded database exceeds the 4 MiB safety limit"};
        }
        const std::size_t offset = text.size();
        text.resize(offset + available);
        DWORD received = 0;
        if (!WinHttpReadData(request, text.data() + offset, available, &received)) {
            return {false, {}, windowsError("WinHttpReadData")};
        }
        text.resize(offset + received);
    }
    return validatedResult(std::move(text));
#else
    static const bool curl_initialized = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    if (!curl_initialized) return {false, {}, "Could not initialize libcurl"};
    CURL* curl = curl_easy_init();
    if (!curl) return {false, {}, "Could not initialize the online database request"};

    std::string text;
    CurlWriteState state{&text, false};
    curl_easy_setopt(curl, CURLOPT_URL, kOnlineDatabaseUrl.data());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PS2-Batch-Renamer/4.1");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 3000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 8000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &state);
    const CURLcode code = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    const std::string curl_error = curl_easy_strerror(code);
    curl_easy_cleanup(curl);
    if (state.oversized) {
        return {false, {}, "Downloaded database exceeds the 4 MiB safety limit"};
    }
    if (code != CURLE_OK) {
        return {false, {}, "Online database request failed: " + curl_error};
    }
    if (status != 200) {
        return {false, {}, "Online database returned HTTP " + std::to_string(status)};
    }
    return validatedResult(std::move(text));
#endif
}

} // namespace ps2br
