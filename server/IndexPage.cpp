#include "IndexPage.hpp"

#pragma once

#include "Http.hpp"
#include "Html.hpp"

#include <functional>

IndexPage::MainContent::operator std::unique_ptr<Tag>()
{
    auto contentContainer = std::make_unique<Tag>("div");

    if (m_GenerateContent != nullptr)
    {
        m_GenerateContent(contentContainer.get());
    }

    return contentContainer;
}

HttpResponse IndexPage::operator()(const Request& request)
{
    std::string content = "";

    auto html = Tag::Html();
    auto head = html->Head();

    head->Meta()->AddProperty("charset", "UTF-8");
    head->Title(GetTitle());

    auto metaViewport = head->Meta();
    metaViewport->AddProperty("name", "viewport");
    metaViewport->AddProperty("content", "width=device-width, initial-scale=1.0");

    auto body = html->Body();

    auto mainContent =
        body->AddTag(
            MainContent(
                GetTitle(),
                [&](Tag* contentContainer)
                {
                    this->GenerateContent(contentContainer);
                }));

    return HttpResponse(html->Emit(), GetStatus(), "text/html");
}

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

std::string IndexPage::GetTitle() const
{
    return "Main page";
}

std::list<std::pair<std::string, std::string>> IndexPage::GetNavbar() const
{
    return
    {
        {"Main page", "/"}
    };
}

int IndexPage::GetStatus()
{
    return 200;
}