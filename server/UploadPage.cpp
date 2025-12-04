#include "UploadPage.hpp"


std::string UploadImagePage::GetTitle() const
{
    return "Upload file";
}

void UploadImagePage::GenerateContent(Tag* contentContainer)
{
    const std::string buttonClass =
        "bg-gray-100 hover:bg-gray-200 text-gray-800 font-semibold py-4 px-4 rounded shadow border-0";
    const std::string fileClass =
        "file:bg-gray-100 file:hover:bg-gray-200 file:text-gray-800 file:font-semibold file:py-2 file:px-4 file:rounded file:shadow "
        "bg-white py-2 px-2 rounded shadow file:border-0 border-0 text-sm file:text-sm";

    std::string script =
        "async function transfer(file, id)"
        "{"
        "    fetch('/uploadFile?id=' + id, {method: 'POST', body: await file.bytes()})"
        "}"
        "async function sendAll()"
        "{"
        "    let result = '';"
        "    let input = document.getElementById('upload');"
        "    for (let i = 0; i < input.files.length; i++)"
        "    {"
        "        result += input.files[i].size + ' ' + input.files[i].name+'\\n';"
        "    }"
        "    let response = (await fetch('/upload', {method: 'POST', body: result}));"
        "    let text = await response.text();console.log(text);"
        "    let lines = text.split(/\\r?\\n/);"
        "    console.log(lines);"
        "    for (let i = 0; i < lines.length; i++)"
        "    {"
        "        if(lines[i].length == '') continue;"
        "        console.log(lines[i]);"
        "        let record = lines[i].split(/;/);"
        "        let name = record[2];"
        "        console.log(name);console.log(Array.from(input.files));"
        "        let file = Array.from(input.files).find((f) => name.endsWith(f.name));"
        "        if (record[3] == 'false')"
        "            transfer(file, record[0]);"
        "    }"
        "}; sendAll();";

    auto uploadInput = contentContainer->Input("file", "files[]");
    uploadInput->AddProperty("id", "upload");
    uploadInput->AddProperty("accept", "image/*");
    uploadInput->AddProperty("multiple");
    uploadInput->Class(fileClass);

    auto sendButton = contentContainer->Input("button", "send", "Wyœlij");
    sendButton->Class(buttonClass);
    sendButton->AddProperty("onclick", script);
    //auto script = contentContainer->Script();
    //script->AddContent("document.getElementById('backlink').setAttribute('href', document.referrer);");
}