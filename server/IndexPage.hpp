#pragma once

#include "Http.hpp"
#include "Html.hpp"

#include <functional>

struct IndexPage
{
    virtual ~IndexPage() = default;

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
