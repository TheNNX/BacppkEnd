#pragma once

#include "Page.hpp"

struct IndexPage : public Page
{
    virtual ~IndexPage() = default;

    void GenerateContent(Tag* contentContainer);
    std::list<std::pair<std::string, std::string>> GetNavbar() const;
};
