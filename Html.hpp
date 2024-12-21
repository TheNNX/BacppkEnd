#pragma once

#include <string>
#include <memory>
#include <list>
#include <map>
#include <regex>
#include <set>

template<typename T, typename TagType>
concept ConvertibleToTagUPtr = requires(T t)
{
    (std::unique_ptr<TagType>) t;
};

class Tag;

/* TODO */
class InnerHtml
{
private:
    Tag* m_Tag;
    friend class Tag;

    InnerHtml(Tag* t) : m_Tag(t)
    {

    }

    /* Deleted assignment */
    InnerHtml(const InnerHtml&) = delete;
    InnerHtml& operator=(const InnerHtml&) = delete;
public:
    operator std::string() const;
};

class TextContent;

/* TODO: Implement particular CSS class generators in subclasses */
class CssClass
{   
    std::set<std::string> m_Components;
public:
    CssClass(const std::set<std::string>& components) : m_Components(components) {};
    
    CssClass(const std::string& cssClass) 
    {
        std::regex regexz("\\s");
        m_Components = std::set<std::string>(
            std::sregex_token_iterator(
                cssClass.begin(), 
                cssClass.end(), 
                regexz, 
                -1),
            std::sregex_token_iterator());
    }
    
    CssClass(const char* cssClass) : CssClass(std::string(cssClass)) {}

    CssClass() = default;

    explicit operator std::string() const
    {
        std::string result = "";

        for (const auto& component : m_Components)
        {
            result += component + " ";
        }

        if (result.length() > 0)
        {
            result.erase(result.length() - 1);
        }
        
        return result;
    }

    operator const std::set<std::string>&() const
    {
        return this->m_Components;
    }

    operator std::set<std::string>&()
    {
        return this->m_Components;
    }

    std::set<std::string>* operator->()
    {
        return &this->m_Components;
    }

    const std::set<std::string>* operator->() const
    {
        return &this->m_Components;
    }
};

class Tag
{
protected:
    std::string m_TagName;
    std::list<std::unique_ptr<Tag>> m_Children;
    std::list<std::pair<std::string, std::string>> m_Properties; 

    CssClass m_CssClass;

    /* FIXME: this implies html reparsing on innerhtml change. This behaviour is NOT implemented. */
    InnerHtml m_InnerHtml;
    friend class InnerHtml;
private:
    Tag* ContentTag(const std::string& tagName, const std::string& content);

public:
    Tag(const std::string& name) : m_TagName(name), m_InnerHtml(this)
    {
    
    }

    virtual ~Tag() = default;

    template<typename TagType = Tag>
    TagType* AddTag(std::unique_ptr<TagType> tag)
    {
        auto result = m_Children.insert(m_Children.end(), std::move(tag));
        return (TagType*)result->get();
    }

    template<typename TagType = Tag, std::convertible_to<std::unique_ptr<TagType>> T>
    TagType* AddTag(T tag)
    {
        return AddTag((std::unique_ptr<TagType>) tag);
    }

    template<typename TagType = Tag>
    TagType* AddTag(const std::string& type)
    {
        return AddTag(std::make_unique<Tag>(type));
    }

    std::string GetPropertyString()
    {
        std::string result = "";
        for (auto kv : m_Properties)
        {
            result += " " + kv.first + "=\"" + kv.second + "\"";
        }

        if (!m_CssClass->empty())
        {
            result += " class=\"" + (std::string)m_CssClass + "\"";
        }

        return result;
    }

    void AddProperty(const std::string& name, const std::string& value)
    {
        m_Properties.push_back(std::make_pair(name, value));
    }

    void Class(const CssClass& cssClass)
    {
        this->m_CssClass = cssClass;
    }

    auto& GetPropertyList()
    {
        return this->m_Properties;
    }

    auto& GetChildrenList()
    {
        return this->m_Children;
    }

    virtual std::string GetContentString()
    {
        return m_InnerHtml;
    }

    virtual std::string Emit()
    {
        std::string result = std::string("<") + m_TagName + GetPropertyString() + ">";

        result += GetContentString();

        return result + "</" + m_TagName + ">";
    }

    void PropertyWrite(const std::string& propertyName, const std::string& value)
    {
        for (auto& property : m_Properties)
        {
            if (property.first == propertyName)
            {
                property.second = value;
                return;
            }
        }

        m_Properties.insert(m_Properties.end(), std::make_pair(propertyName, value));
    }

    const std::string& PropertyRead(const std::string& propertyName)
    {
        for (auto& property : m_Properties)
        {
            if (property.first == propertyName)
            {
                return property.second;
            }
        }

        return "";
    }

    static auto Html()
    {
        return std::make_unique<Tag>("html");
    }

    Tag* Head()
    {
        return AddTag(std::make_unique<Tag>("head"));
    }

    Tag* Body()
    {
        return AddTag(std::make_unique<Tag>("body"));
    }

    Tag* Div()
    {
        return AddTag(std::make_unique<Tag>("div"));
    }

    Tag* P(const std::string& content = "")
    {
        return this->ContentTag("p", content);
    }

    Tag* Form()
    {
        return AddTag(std::make_unique<Tag>("form"));
    }

    Tag* Input(const std::string& type, const std::string& name, const std::string& value = "")
    {
        /* TODO: id= */

        class InputTag : public Tag
        {
        public:
            InputTag() : Tag("input")
            {

            }

            std::string Emit() override
            {
                return std::string("<") + m_TagName + GetPropertyString() + ">";
            }
        };
        
        auto result = AddTag(std::make_unique<InputTag>());
        result->AddProperty("name", name);
        result->AddProperty("type", type);

        if (value != "")
        {
            result->AddProperty("value", value);
        }

        return result;
    }

    Tag* Br()
    {
        class BrTag : public Tag
        {
        public:
            BrTag() : Tag("br")
            {

            }

            std::string Emit() override
            {
                return "<" + m_TagName + ">";
            }
        };

        return AddTag(std::make_unique<BrTag>());
    }

    Tag* Label(const std::string& forName, const std::string& content)
    {
        auto label = ContentTag("label", content);
        label->AddProperty("for", forName);
        return label;
    }

    Tag* A(const std::string& href, const std::string& content);

    Tag* A(const std::string& to)
    {
        return A(to, to);
    }

    Tag* Meta()
    {
        return AddTag(std::make_unique<Tag>("meta"));
    }

    Tag* Link(const std::string& href, const std::string& rel);

    Tag* Script(const std::string& src)
    {
        auto script = Script();
        script->AddProperty("src", src);
        return script;
    }

    Tag* Script()
    {
        return AddTag(std::make_unique<Tag>("script"));
    }

    Tag* Text(const std::string& content = "")
    {
        return ContentTag("text", content);
    }

    Tag* H1(const std::string& content = "")
    {
        return ContentTag("h1", content);
    }

    Tag* H2(const std::string& content = "")
    {
        return ContentTag("h2", content);
    }

    Tag* H3(const std::string& content = "")
    {
        return ContentTag("h3", content);
    }

    Tag* H4(const std::string& content = "")
    {
        return ContentTag("h4", content);
    }

    Tag* H5(const std::string& content = "")
    {
        return ContentTag("h5", content);
    }

    Tag* Title(const std::string& content = "")
    {
        return ContentTag("title", content);
    }

    Tag* Img(const std::string& src, const std::string& alt = "", const std::string& width = "", const std::string& height = "")
    {
        auto img = std::make_unique<Tag>("img");
        img->AddProperty("src", src);
        if (alt != "")
        {
            img->AddProperty("alt", alt);
        }
        if (width != "")
        {
            img->AddProperty("width", width);
        }
        if (height != "")
        {
            img->AddProperty("height", height);
        }

        return AddTag(std::move(img));
    }

    Tag* Button(const std::string& text, const std::string& onClick = "")
    {
        auto button = ContentTag("button", text);

        if (onClick != "")
        {
            button->AddProperty("onclick", onClick);
        }

        return button;
    }

    Tag* Style(const std::string& content)
    {
        return ContentTag("style", content);
    }

    void AddContent(const std::string& content);
};

class TextContent : public Tag
{
private:
    std::string m_Text;
public:
    TextContent(const std::string& text) : m_Text(text), 
        Tag("")
    {

    }

    std::string Emit() override
    {
        return m_Text;
    }
};

inline InnerHtml::operator std::string() const
{
    std::string result = "";

    for (auto& child : m_Tag->m_Children)
    {
        result += child->Emit();
    }

    return result;
}

inline void Tag::AddContent(const std::string& content)
{
    AddTag((std::unique_ptr<Tag>)std::make_unique<TextContent>(content));
}

inline Tag* Tag::ContentTag(const std::string& tagName, const std::string& content)
{
    auto text = std::make_unique<Tag>(tagName);

    if (content != "")
    {
        text->AddContent(content);
    }

    return AddTag(std::move(text));
}

inline Tag* Tag::Link(const std::string& href, const std::string& rel)
{
    auto text = std::make_unique<Tag>("link");
    text->PropertyWrite("href", href);
    text->PropertyWrite("rel", rel);

    return AddTag(std::move(text));
}

inline Tag* Tag::A(const std::string& href, const std::string& content)
{
    auto text = std::make_unique<Tag>("a");
    text->PropertyWrite("href", href);

    if (content != "")
    {
        text->AddTag((std::unique_ptr<Tag>)std::make_unique<TextContent>(content));
    }

    return AddTag(std::move(text));
}

class TailwindStyleTag : public Tag
{
public:
    std::map<std::string, std::list<std::string>> m_Rules;

    TailwindStyleTag() : Tag("style")
    {
        this->AddProperty("type", "text/tailwindcss");
    }

    std::string GetContentString() override
    {
        std::string result = "";

        for (auto rule : m_Rules)
        {
            result += rule.first + " {\n";

            for (auto ruleLine : rule.second)
            {
                result += "    " + ruleLine;
            }

            result += "\n}\n";
        }

        return result;
    }
};

class Pseudotag
{
public:
    virtual operator std::unique_ptr<Tag>() = 0;
};