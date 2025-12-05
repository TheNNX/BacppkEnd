#pragma once

#include "IndexPage.hpp"

struct ErrorPage : public IndexPage
{
    const int m_ErrorCode;

    constexpr ErrorPage(int errorCode) :
        m_ErrorCode(errorCode)
    {
    }

    std::string GetTitle() const override;
    int GetStatus() override;

    void GenerateContent(const Request& request, Tag* contentContainer) override;
};