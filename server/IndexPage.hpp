#pragma once

#include "Page.hpp"

struct IndexPage : public Page
{
    virtual ~IndexPage() = default;

    void GenerateContent(const Request& request, Tag* contentContainer) override;
    std::list<std::pair<std::string, std::string>> GetNavbar() const;
};
