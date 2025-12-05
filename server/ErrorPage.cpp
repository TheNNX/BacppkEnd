#include "ErrorPage.hpp"

std::string ErrorPage::GetTitle() const
{
    return "Error";
}

int ErrorPage::GetStatus()
{
    return this->m_ErrorCode;
}

void ErrorPage::GenerateContent(const Request& request, Tag* contentContainer)
{
    std::string s = StringifyHttpCode(m_ErrorCode);

    auto h2 = contentContainer->H2(s);

    auto backlink = contentContainer->A("", "Back");
    backlink->AddProperty("id", "backlink");

    auto script = contentContainer->Script();
    script->AddContent("document.getElementById('backlink').setAttribute('href', document.referrer);");
}