#pragma once

#include "Http.hpp"
#include "Html.hpp"

#include <functional>

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

    HttpResponse operator()(const Request& request);
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

        operator std::unique_ptr<Tag>() override;
    };

    struct NavBar : Pseudotag
    {
        std::list<std::pair<std::string, std::string>> m_NavBarContents;

        NavBar(std::list<std::pair<std::string, std::string>>&& contents);

        operator std::unique_ptr<Tag>() override;
    };

    struct Banner : Pseudotag
    {
        std::string m_Title;

        Banner(const std::string& title) :
            m_Title(title) {}

        operator std::unique_ptr<Tag>() override;
    };

    struct MainContent : Pseudotag
    {
        std::string m_Title;
        std::function<void(Tag*)> m_GenerateContent;

        MainContent(const std::string& title,
                    const decltype(m_GenerateContent)& generator) :
            m_Title(title), 
            m_GenerateContent(generator)
        { }

        operator std::unique_ptr<Tag>() override;
    };

    HttpResponse operator()(const Request& request);
    virtual void GenerateContent(Tag* contentContainer);
    virtual std::string GetTitle() const;
    virtual std::list<std::pair<std::string, std::string>> GetNavbar() const;
    virtual int GetStatus();
};

struct CenteredFocusImg : Pseudotag
{
    std::string m_Src = "";
    std::string m_Subtitle;
    std::string m_Alt = "";
    std::string m_Width = "";
    std::string m_Height = "";

    CenteredFocusImg(const std::string& src,
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

    operator std::unique_ptr<Tag>() override;
};