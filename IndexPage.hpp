#pragma once

#include "Http.hpp"
#include "Html.hpp"

constexpr std::string_view StylesheetPath = "/styler.css";

struct Styler
{
    inline const static CssClass m_Div1 =
        CssClass({ "flex", "shadow-md", "shadow-black", "text-3xl", "text-white", "bg-zinc-900", "fixed", "w-full", "flex-none", "top-0", "overflow-visible" });
    inline const static CssClass m_NavPadding =
        CssClass({ "flex-initial", "w-full", "text-center" });
    inline const static CssClass m_NavLink =
        CssClass({ "navbar", "text-nowrap", "flex-auto", "grow", "my-1.5", "mx-3", "text-xl" });
    inline const static CssClass m_NavMenuBtn =
        CssClass({ "md:hidden", "text-nowrap", "flex-auto", "grow", "my-1.5", "mx-3", "text-xl" });
    inline const static CssClass m_MainContent =
        CssClass({ "my-4", "lg:mx-48", "xl:mx-72", "h-fit", "min-h-full", "bg-white", "shadow-md", "shadow-black" });
    inline const static CssClass m_Banner =
        CssClass("py-16 lg:py-24 text-white bg-zinc-600 shadow-black shadow-sm");
    inline const static CssClass m_BannerText =
        CssClass("grow text-5xl md:text-7xl font-bold text-center");
    inline const static CssClass m_ContentContainer =
        CssClass("text-md px-10 lg:px-12 py-4");
    inline const static CssClass m_Menu =
        CssClass("absolute top-[100%] bg-zinc-900 p-3 flex flex-col max-w-full min-w-fit");

    inline const static CssClass m_MXAuto =
        CssClass("mx-auto");

    HttpResponse operator()(const Request& request)
    {
        std::string result = "";

        return HttpResponse(result, 200, "text/css");
    }
};

struct IndexPage
{
    virtual ~IndexPage() = default;

    struct NavLink : Pseudotag
    {
        std::string m_Href;
        std::string m_Content;

        NavLink(const std::string& href, const std::string& content) :
            m_Href(href), m_Content(content) {}

        operator std::unique_ptr<Tag>() override
        {
            auto navLink = std::make_unique<Tag>("div");
            navLink->Class(Styler::m_NavLink);
            auto A = navLink->A(m_Href, m_Content);
            return navLink;
        }
    };

    struct NavBar : Pseudotag
    {
        std::list<std::pair<std::string, std::string>> m_NavBarContents;

        NavBar(std::list<std::pair<std::string, std::string>>&& contents)
        {
            m_NavBarContents = std::move(contents);
        }

        operator std::unique_ptr<Tag>() override
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
    };

    struct Banner : Pseudotag
    {
        std::string m_Title;

        Banner(const std::string& title) :
            m_Title(title) {}

        operator std::unique_ptr<Tag>() override
        {
            auto banner = std::make_unique<Tag>("div");
            banner->Class(Styler::m_Banner);

            auto h1 = banner->H1(m_Title);
            h1->Class(Styler::m_BannerText);

            return banner;
        }
    };

    struct MainContent : Pseudotag
    {
        std::string m_Title;
        std::function<void(Tag*)> m_GenerateContent;

        MainContent(
            const std::string& title, 
            const decltype(m_GenerateContent)& generator) :
            m_Title(title), m_GenerateContent(generator)
        { }

        operator std::unique_ptr<Tag>() override
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
    };

    HttpResponse operator()(const Request& request)
    {
        std::string content = "";

        auto html = Tag::Html();
        auto head = html->Head();

        head->Script("https://cdn.tailwindcss.com");
        head->Link((std::string) StylesheetPath, "stylesheet");

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

    virtual void GenerateContent(Tag* contentContainer)
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

    virtual std::string GetTitle() const
    {
        return "Main page";
    }

    virtual std::list<std::pair<std::string, std::string>> GetNavbar() const
    {
        return {
            {"Main page", "/"}
        };
    }

    virtual int GetStatus()
    {
        return 200;
    }
};

struct CenteredFocusImg : Pseudotag
{
    std::string m_Src = "";
    std::string m_Subtitle;
    std::string m_Alt = "";
    std::string m_Width = "";
    std::string m_Height = "";

    CenteredFocusImg(
        const std::string& src,
        const std::string& subtitle,
        const std::string& alt = "",
        const std::string& width = "",
        const std::string& height = "") :
        m_Src(src), 
        m_Subtitle(subtitle), 
        m_Alt(alt), 
        m_Width(width), 
        m_Height(height)
    {
    }

    operator std::unique_ptr<Tag>() override
    {
        auto div = std::make_unique<Tag>("div");
        auto img = div->Img(m_Src, m_Alt, m_Width, m_Height);
        img->Class(Styler::m_MXAuto);
        div->H4(m_Subtitle)->Class("text-center bold");

        return div;
    }
};