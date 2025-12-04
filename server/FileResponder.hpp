#pragma once

#include "HttpServer.hpp"
#include <filesystem>

struct FileResponder
{
    std::filesystem::path m_File;
    std::string m_MimeType;

    FileResponder(const std::filesystem::path& file,
                  const std::string& mimeType = "");

    HttpResponse operator()(const Request& request);
};