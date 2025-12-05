#include "LoginPage.hpp"

std::string LoginPage::GetTitle() const
{
    return "Login";
}

void LoginPage::GenerateContent(const Request&, Tag* container)
{
    container->H1("Login");
    auto form = container->Form("/login");
    form->AddProperty("method", "post");

    form->Label("username", "Username:");
    form->Input("text", "username", "");
    form->Label("password", "Password:");
    form->Input("password", "password", "");
    form->Input("submit", "", "Submit");
}