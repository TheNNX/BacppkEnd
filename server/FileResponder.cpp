#include "FileResponder.hpp"

#define BUFFER_SIZE 8192

FileResponder::FileResponder(
    const std::filesystem::path& file,
    const std::string& mimeType) :
    m_File(file), m_MimeType(mimeType)
{
    if (mimeType == "")
    {
        m_MimeType = "application/octet-stream";

        const std::map<std::string, std::string> mimeMapping =
        {
            { ".png", "image/png" },
            { ".jpg", "image/jpeg" },
            { ".jpeg", "image/jpeg" },
            { ".txt", "text/plain" },
            { ".c", "text/plain" },
            { ".cpp", "text/plain" },
            { ".h", "text/plain" },
            { ".hpp", "text/plain" },
            { "Makefile", "text/plain" },
            { "makefile", "text/plain" },
            { ".html", "text/html" },
            { ".htm", "text/html" },
            { ".zip", "application/zip" },
            { ".wasm", "application/wasm" }
        };

        auto toupper = [](std::string str) -> std::string
            {
                std::transform(str.begin(), str.end(), str.begin(), ::toupper);
                return str;
            };

        for (auto kv : mimeMapping)
        {
            if (toupper(file.filename().string()).ends_with(toupper(kv.first)))
            {
                m_MimeType = kv.second;
                break;
            }
        }
    }

    std::cout << file << " MIME type " << m_MimeType << "\n";
}

HttpResponse FileResponder::operator()(const Request& request)
{
    std::ifstream file(m_File, std::ios::binary | std::ios::ate);
    std::string content = "";

    int sizeLeft = file.tellg();

    if (sizeLeft < 0)
    {
        throw std::runtime_error("FileResponder couldn't locate file " + this->m_File.string());
    }

    file.seekg(file.beg);
    auto method = request.m_Method;

    struct Promise : public ContentPromise
    {
        const Request& request;
        int sizeLeft;
        std::ifstream file;

        Promise(const Request& request, int sizeLeft, std::ifstream&& file) :
            request(request), sizeLeft(sizeLeft), file(std::move(file)) 
        {
        }

        std::string Fulfill() override
        {
            char buffer[BUFFER_SIZE] = { 0 };
            std::string result = "";

            if (request.m_Method == "GET")
            {
                while (sizeLeft > 0)
                {
                    auto currentSize = std::min(sizeLeft, BUFFER_SIZE);
                    file.read(buffer, currentSize);
                    sizeLeft -= currentSize;

                    result += std::string(buffer, buffer + currentSize);
                }
            }

            return result;
        }
    };

    auto resp = HttpResponse(std::make_unique<Promise>(request, sizeLeft, std::move(file)), 200, m_MimeType);
    resp.m_Headers["Content-Length"] = std::to_string(sizeLeft);
    return resp;
}