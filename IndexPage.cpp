#include "IndexPage.hpp"

#pragma once

#include "Http.hpp"
#include "Html.hpp"

#include <functional>

HttpResponse Styler::operator()(const Request& request)
{
    std::string result = "";

    return HttpResponse(result, 200, "text/css");
}

IndexPage::NavLink::operator std::unique_ptr<Tag>()
{
    auto navLink = std::make_unique<Tag>("div");
    navLink->Class(Styler::m_NavLink);
    auto A = navLink->A(m_Href, m_Content);
    return navLink;
}

IndexPage::NavBar::NavBar(std::list<std::pair<std::string, std::string>>&& contents)
{
    m_NavBarContents = std::move(contents);
}

IndexPage::Banner::operator std::unique_ptr<Tag>()
{
    auto banner = std::make_unique<Tag>("div");
    banner->Class(Styler::m_Banner);

    auto h1 = banner->H1(m_Title);
    h1->Class(Styler::m_BannerText);

    return banner;
}

IndexPage::NavBar::operator std::unique_ptr<Tag>()
{
    auto navBar = std::make_unique<Tag>("div");
    navBar->Class(Styler::m_Div1);

    auto navBarMenuBtn = navBar->Button(
        "Menu",
        "document.getElementById('menu').toggleStatus();");
    navBarMenuBtn->AddProperty("id", "menuButton");
    navBarMenuBtn->Class(Styler::m_NavMenuBtn);

    auto navBarPaddingDiv = navBar->Div();
    navBarPaddingDiv->Class(Styler::m_NavPadding);

    auto navBarHelperDiv = navBar->Div();
    navBarHelperDiv->Class(CssClass("max-md:hidden w-fit h-fit flex"));
    for (auto a : m_NavBarContents)
    {
        navBarHelperDiv->AddTag(NavLink(a.second, a.first));
    }

    auto menu = navBar->Div();
    menu->AddProperty("id", "menu");
    menu->AddProperty("style", "display: none");
    menu->Class(Styler::m_Menu);

    for (auto a : m_NavBarContents)
    {
        menu->AddTag(NavLink(a.second, a.first));
    }

    auto menuShowScript = navBar->Script();
    menuShowScript->AddContent(
        "\ndocument.getElementById('menu').getStatus = function(){\n"
        "   return this.style.display != 'none';\n"
        "};\n"
        "document.getElementById('menu').setStatus = function(status){\n"
        "   if (status) this.style.display = 'block';\n"
        "   else this.style.display = 'none';\n"
        "};\n"
        "document.getElementById('menu').toggleStatus = function(){\n"
        "   this.setStatus(!this.getStatus());\n"
        "};\n"
        "window.addEventListener('click',(evt)=>{\n"
        "   let menu = document.getElementById('menu');\n"
        "   let button = document.getElementById('menuButton');\n"
        "   const path = evt.composedPath();\n"
        "   if (menu.getStatus() && !path.includes(button))\n"
        "       menu.setStatus(path.includes(menu));\n"
        "});");

    return navBar;
}

IndexPage::MainContent::operator std::unique_ptr<Tag>()
{
    auto mainDiv = std::make_unique<Tag>("div");
    mainDiv->Class(Styler::m_MainContent);

    mainDiv->AddTag(Banner(m_Title));

    auto contentContainer = mainDiv->Div();
    contentContainer->Class(Styler::m_ContentContainer);

    if (m_GenerateContent != nullptr)
    {
        m_GenerateContent(contentContainer);
    }

    return mainDiv;
}

HttpResponse IndexPage::operator()(const Request& request)
{
    std::string content = "";

    auto html = Tag::Html();
    auto head = html->Head();

    head->Script("https://cdn.tailwindcss.com");
    head->Link((std::string)StylesheetPath, "stylesheet");

    TailwindStyleTag* result = head->AddTag(std::make_unique<TailwindStyleTag>());

    result->m_Rules["a:not(.navbar *)"] = { "@apply hover:underline text-blue-800 visited:text-purple-600" };

    result->m_Rules["h1"] = { "@apply text-6xl font-bold mb-2" };
    result->m_Rules["h2"] = { "@apply text-4xl font-bold mb-2" };
    result->m_Rules["h3"] = { "@apply text-2xl font-bold mb-2" };

    result->m_Rules["body"] = { "@apply h-full max-h-full bg-zinc-200" };

    result->m_Rules["p"] = { "@apply mb-3" };

    head->Meta()->AddProperty("charset", "UTF-8");
    head->Title(GetTitle());

    auto metaViewport = head->Meta();
    metaViewport->AddProperty("name", "viewport");
    metaViewport->AddProperty("content", "width=device-width, initial-scale=1.0");

    auto frontendScript = head->Script("/loadwasm.js");

    auto body = html->Body();

    auto navBar = body->AddTag(NavBar(GetNavbar()));

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

CenteredFocusImg::operator std::unique_ptr<Tag>()
{
    auto div = std::make_unique<Tag>("div");
    auto img = div->Img(m_Src, m_Alt, m_Width, m_Height);
    img->Class(Styler::m_MXAuto);
    div->H4(m_Subtitle)->Class("text-center bold");

    return div;
}