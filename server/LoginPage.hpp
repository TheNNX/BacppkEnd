#pragma once

#include "Page.hpp"

struct LoginPage : public Page
{
    void GenerateContent(const Request& request, Tag* contentContainer);
    std::string GetTitle() const;
};