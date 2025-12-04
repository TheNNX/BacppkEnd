#pragma once

#include "IndexPage.hpp"

struct UploadImagePage : public IndexPage
{
    std::string GetTitle() const override;
    void GenerateContent(Tag* contentContainer) override;
};