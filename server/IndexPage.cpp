#include "IndexPage.hpp"

void IndexPage::GenerateContent(Tag* contentContainer)
{
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