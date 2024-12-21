#pragma once

#include "IndexPage.hpp"

struct ErrorPage : public IndexPage
{
    const int m_ErrorCode;

    constexpr ErrorPage(int errorCode) :
        m_ErrorCode(errorCode)
    {
    }

    std::string GetTitle() const override
    {
        return "Error";
    }

    int GetStatus() override
    {
        return this->m_ErrorCode;
    }

    virtual void GenerateContent(Tag* contentContainer)
    {
        std::string s = StringifyHttpCode(m_ErrorCode);

        auto h2 = contentContainer->H2(s);

        auto backlink = contentContainer->A("", "Back");
        backlink->AddProperty("id", "backlink");

        auto script = contentContainer->Script();
        script->AddContent("document.getElementById('backlink').setAttribute('href', document.referrer);");
    }
};