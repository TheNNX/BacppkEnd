#pragma once

#include "Http.hpp"

#include <filesystem>

struct UploadApi
{
    HttpResponse operator()(const Request& request);

    std::filesystem::path m_ServerUploadDirectory =
#ifndef WIN32
        absolute("/mnt/data/shared/upload");
#else
        std::filesystem::absolute("upload");
#endif

    UploadApi(std::filesystem::path uploadDir) : m_ServerUploadDirectory(uploadDir) { }
    UploadApi() = default;
};

struct UploadFileApi
{
    HttpResponse operator()(const Request& request);
};