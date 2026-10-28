/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

// XML layouts from outside: a string read to its size, nesting bounded,
// numbers and durations in range, a custom resource in error falling back
// to the built-in one. Cadran's safe_text and Markdown are tested without a
// window, in tests/host/markdown_tests.cpp.

#include <borealis.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/test/harness.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

using brls::test::Harness;

namespace
{

void writeFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

// The message of the error creating the view from `xml` throws ("" when
// none)
std::string xmlError(const std::string& xml)
{
    try
    {
        delete brls::View::createFromXMLString(xml);
        return "";
    }
    catch (const std::exception& e)
    {
        return std::string(typeid(e).name()) + ": " + e.what();
    }
}

} // namespace

BRLS_TEST(xml_string_view_is_read_to_its_size)
{
    Harness harness;
    // The view ends before the text after it, which is no XML: read past
    // its size (as a C string), it made the parse fail
    std::string buffer = "<brls:Box id=\"sized\" width=\"40\"/><<<not xml";
    std::string_view xml(buffer.data(), buffer.find("<<<"));
    brls::View* view = brls::View::createFromXMLString(xml);
    BRLS_CHECK(view != nullptr && view->getId() == "sized");
    delete view;

    struct Inflated : brls::Box
    {
        using brls::Box::inflateFromXMLString;
    };
    Inflated* box     = new Inflated();
    std::string inner = "<brls:Box id=\"inflated\"><brls:Rectangle/></brls:Box>&&&";
    box->inflateFromXMLString(std::string_view(inner.data(), inner.size() - 3));
    BRLS_CHECK_EQ(box->getChildren().size(), (size_t)1);
    delete box;
}

BRLS_TEST(xml_nesting_is_bounded)
{
    Harness harness;
    // Deep but reasonable: read
    std::string deep;
    for (int i = 0; i < 40; i++)
        deep += "<brls:Box>";
    for (int i = 0; i < 40; i++)
        deep += "</brls:Box>";
    BRLS_CHECK_EQ(xmlError(deep), std::string());

    // Past the bound: an error, not a stack overflow
    std::string deeper;
    for (int i = 0; i < 100; i++)
        deeper += "<brls:Box>";
    for (int i = 0; i < 100; i++)
        deeper += "</brls:Box>";
    BRLS_CHECK(xmlError(deeper).find("nested more than") != std::string::npos);
}

BRLS_TEST(xml_file_including_itself_is_an_error)
{
    namespace fs  = std::filesystem;
    fs::path root = fs::temp_directory_path() / ("aurore-xml-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    // Hot reload reads @res/ from this folder: its cell.xml includes itself
    writeFile(root / "xml/cells/cell.xml", "<brls:Box><brls:View xml=\"@res/xml/cells/cell.xml\"/></brls:Box>");

    Harness harness;
    brls::HotReload::setEnabled(true, root.string() + "/");
    std::string error;
    try
    {
        delete brls::View::createFromXMLResource("cells/cell.xml");
    }
    catch (const std::exception& e)
    {
        error = e.what();
    }
    brls::HotReload::setEnabled(false);
    // An error hot reload logs (the screen stays as it was), not a stack
    // overflow
    BRLS_CHECK(error.find("nested more than") != std::string::npos);
    fs::remove_all(root);
}

BRLS_TEST(xml_custom_resource_in_error_falls_back_to_the_built_in_one)
{
    namespace fs  = std::filesystem;
    fs::path root = fs::temp_directory_path() / ("aurore-custom-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    writeFile(root / "xml/cells/cell.xml", "<brls:Box><brls:Unknown/></brls:Box>");

    Harness harness;
    std::string before                = brls::View::CUSTOM_RESOURCES_PATH;
    brls::View::CUSTOM_RESOURCES_PATH = root.string() + "/";
    brls::View* view                  = nullptr;
    try
    {
        view = brls::View::createFromXMLResource("cells/cell.xml");
    }
    catch (const std::exception& e)
    {
        brls::Logger::error("{}", e.what());
    }
    brls::View* invalid = nullptr;
    try
    {
        writeFile(root / "xml/cells/cell.xml", "<brls:Box><unclosed></brls:Box>");
        brls::View::forgetXMLCache((root / "xml/cells/cell.xml").string());
        invalid = brls::View::createFromXMLResource("cells/cell.xml");
    }
    catch (const std::exception& e)
    {
        brls::Logger::error("{}", e.what());
    }
    brls::View::CUSTOM_RESOURCES_PATH = before;

    // The built-in ones: an image and a label each
    for (brls::View* built : { view, invalid })
    {
        brls::Box* box = dynamic_cast<brls::Box*>(built);
        BRLS_CHECK(box != nullptr);
        if (box)
            BRLS_CHECK_EQ(box->getChildren().size(), (size_t)2);
        delete built;
    }
    fs::remove_all(root);
}

BRLS_TEST(xml_numbers_out_of_range_are_refused)
{
    Harness harness;
    BRLS_CHECK_EQ(xmlError("<brls:Box width=\"40\" height=\"25%\" marginTop=\"2.5px\"/>"), std::string());
    // Not a number, infinite, past a float: an error on the attribute (not
    // NaN in the layout, nor an out_of_range thrown from std::stof)
    for (const char* value : { "nan", "inf", "-inf", "1e99", "nanpx", "1e99px", "nan%", "1e99%" })
    {
        std::string error = xmlError(std::string("<brls:Box width=\"") + value + "\"/>");
        BRLS_CHECK(!error.empty());
        BRLS_CHECK(error.find("out_of_range") == std::string::npos);
        BRLS_CHECK(error.find(value) != std::string::npos);
    }
}

BRLS_TEST(xml_durations_out_of_range_are_refused)
{
    Harness harness;
    brls::Box box;
    BRLS_CHECK(box.setTransition("backgroundColor 250ms"));
    BRLS_CHECK(box.setTransition("backgroundColor 2s cubicOut"));
    BRLS_CHECK(box.setTransition("backgroundColor 60s"));
    for (const char* spec : { "backgroundColor 1e30s", "backgroundColor 3e9ms", "backgroundColor nans", "backgroundColor infms", "backgroundColor -1s", "backgroundColor 61s" })
        BRLS_CHECK(!box.setTransition(spec));
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
