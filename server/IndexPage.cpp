#include "IndexPage.hpp"

#include "LoginApi.hpp"

void IndexPage::GenerateContent(const Request& request, Tag* contentContainer)
{
    SessionHandle session;

    auto cookies = request.GetCookies();
    if (cookies.contains("sessionId"))
    {
        session = SessionHandle(cookies.at("sessionId"));
    }

    if (session)
    {
        contentContainer->Text("Hello, " + session.ReadProperty("username"));
    }

    auto pages = contentContainer->P("Pages:<br>");
    auto p = GetNavbar();

    for (auto kv : p)
    {
        if (kv.second != "/")
        {
            pages->A(kv.second, kv.first);
            pages->AddContent("<br>");
        }
    }
}

std::list<std::pair<std::string, std::string>> IndexPage::GetNavbar() const
{
    return
    {
        {"Main page", "/"}
    };
}