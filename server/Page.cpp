#include "Page.hpp"

#include "Http.hpp"
#include "Html.hpp"

#include <functional>

Page::MainContent::operator std::unique_ptr<Tag>()
{
    auto contentContainer = std::make_unique<Tag>("div");

    if (m_GenerateContent != nullptr)
    {
        m_GenerateContent(contentContainer.get());
    }

    return contentContainer;
}

HttpResponse Page::operator()(const Request& request)
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
                    this->GenerateContent(request, contentContainer);
                }));

    return HttpResponse(html->Emit(), GetStatus(), "text/html");
}

std::string Page::GetTitle() const
{
    return "Page";
}

int Page::GetStatus()
{
    return 200;
}