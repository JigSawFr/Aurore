// SPDX-FileCopyrightText: 2019 natinusala
// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <aurore/core/inspector.hpp>
#include <aurore/core/style.hpp>
#include <aurore/core/util.hpp>

namespace aurore
{

static StyleValues styleValues = {
    // Animations
    { "aurore/animations/show", 200.0f, MetricKind::OTHER },
    { "aurore/animations/show_slide", 125.0f, MetricKind::OTHER },

    { "aurore/animations/highlight", 200.0f, MetricKind::OTHER },
    { "aurore/animations/highlight_shake", 15.0f, MetricKind::OTHER },

    { "aurore/animations/label_scrolling_timer", 1500.0f, MetricKind::OTHER },
    { "aurore/animations/label_scrolling_speed", 0.05f, MetricKind::OTHER },
    { "aurore/animations/notification_timeout", 4000.0f, MetricKind::OTHER },
    { "aurore/animations/notification_show", 300.0f, MetricKind::OTHER },

    // Notification
    { "aurore/notification/width", 280.0f, MetricKind::LENGTH },
    { "aurore/notification/padding", 16.0f, MetricKind::LENGTH },
    { "aurore/notification/slide", 40.0f, MetricKind::LENGTH },
    { "aurore/notification/corner_radius", 0.0f, MetricKind::LENGTH },
    { "aurore/notification/margin_top", 0.0f, MetricKind::LENGTH },
    { "aurore/notification/margin_right", 0.0f, MetricKind::LENGTH },
    { "aurore/notification/spacing", 0.0f, MetricKind::LENGTH },

    // Highlight
    { "aurore/highlight/stroke_width", 5.0f, MetricKind::LENGTH },
    { "aurore/highlight/corner_radius", 6.0f, MetricKind::LENGTH },
    { "aurore/highlight/shadow_width", 2.0f, MetricKind::LENGTH },
    { "aurore/highlight/shadow_offset", 10.0f, MetricKind::LENGTH },
    { "aurore/highlight/shadow_feather", 10.0f, MetricKind::LENGTH },
    { "aurore/highlight/shadow_opacity", 128.0f, MetricKind::OTHER },

    // AppletFrame
    { "aurore/applet_frame/padding_sides", 30.0f, MetricKind::LENGTH },

    { "aurore/applet_frame/header_height", 88.0f, MetricKind::LENGTH },
    { "aurore/applet_frame/header_padding_top_bottom", 15.0f, MetricKind::LENGTH },
    { "aurore/applet_frame/header_padding_sides", 35.0f, MetricKind::LENGTH },
    { "aurore/applet_frame/header_image_title_spacing", 18.0f, MetricKind::LENGTH },
    { "aurore/applet_frame/header_title_font_size", 28.0f, MetricKind::OTHER },
    { "aurore/applet_frame/header_title_top_offset", 7.0f, MetricKind::LENGTH },

    { "aurore/applet_frame/footer_height", 73.0f, MetricKind::LENGTH },
    { "aurore/applet_frame/footer_padding_top_bottom", 20.0f, MetricKind::LENGTH },
    { "aurore/applet_frame/footer_padding_sides", 25.0f, MetricKind::LENGTH },

    // TabFrame
    { "aurore/tab_frame/sidebar_width", 410.0f, MetricKind::LENGTH },
    { "aurore/tab_frame/content_padding_top_bottom", 42.0f, MetricKind::LENGTH }, // a tab's content, for the apps' XML (Cadran's margins with it)
    { "aurore/tab_frame/content_padding_sides", 60.0f, MetricKind::LENGTH }, // a tab's content, for the apps' XML

    // Sidebar
    { "aurore/sidebar/border_height", 16.0f, MetricKind::LENGTH },
    { "aurore/sidebar/padding_top", 32.0f, MetricKind::LENGTH },
    { "aurore/sidebar/padding_bottom", 47.0f, MetricKind::LENGTH },
    { "aurore/sidebar/padding_left", 80.0f, MetricKind::LENGTH },
    { "aurore/sidebar/padding_right", 40.0f, MetricKind::LENGTH },
    { "aurore/sidebar/item_height", 70.0f, MetricKind::LENGTH },
    { "aurore/sidebar/item_accent_margin_top_bottom", 9.0f, MetricKind::LENGTH },
    { "aurore/sidebar/item_accent_margin_sides", 8.0f, MetricKind::LENGTH },
    { "aurore/sidebar/item_accent_rect_width", 4.0f, MetricKind::LENGTH },
    { "aurore/sidebar/item_font_size", 22.0f, MetricKind::OTHER },
    { "aurore/sidebar/separator_height", 30.0f, MetricKind::LENGTH },

    // Tab Details
    { "aurore/tab_details/padding_top", 32.0f, MetricKind::LENGTH },
    { "aurore/tab_details/padding_bottom", 47.0f, MetricKind::LENGTH },
    { "aurore/tab_details/padding_left", 60.0f, MetricKind::LENGTH },
    { "aurore/tab_details/padding_right", 80.0f, MetricKind::LENGTH },

    // Label
    { "aurore/label/default_font_size", 20.0f, MetricKind::OTHER },
    { "aurore/label/default_line_height", 1.65f, MetricKind::OTHER },
    { "aurore/label/scrolling_animation_spacing", 50.0f, MetricKind::LENGTH },
    { "aurore/label/highlight_padding", 2.0f, MetricKind::LENGTH },

    // Header
    { "aurore/header/padding_top_bottom", 11.0f, MetricKind::LENGTH },
    { "aurore/header/padding_right", 11.0f, MetricKind::LENGTH },
    { "aurore/header/rectangle_width", 5.0f, MetricKind::LENGTH },
    { "aurore/header/rectangle_height", 33.0f, MetricKind::LENGTH },
    { "aurore/header/rectangle_margin", 10.0f, MetricKind::LENGTH },
    { "aurore/header/font_size", 18.0f, MetricKind::OTHER },

    // Button
    { "aurore/button/padding_top_bottom", 15.0f, MetricKind::LENGTH },
    { "aurore/button/padding_sides", 25.0f, MetricKind::LENGTH },
    { "aurore/button/corner_radius", 5.0f, MetricKind::LENGTH },
    { "aurore/button/text_size", 18.0f, MetricKind::OTHER },
    { "aurore/button/primary_highlight_padding", 2.0f, MetricKind::LENGTH },
    { "aurore/button/border_thickness", 2.0f, MetricKind::LENGTH },

    // Generic shadow
    { "aurore/shadow/width", 2.0f, MetricKind::LENGTH },
    { "aurore/shadow/feather", 10.0f, MetricKind::LENGTH },
    { "aurore/shadow/opacity", 63.75f, MetricKind::OTHER },
    { "aurore/shadow/offset", 10.0f, MetricKind::LENGTH },

    // Dropdown
    { "aurore/dropdown/listPadding", 40.0f, MetricKind::LENGTH },
    { "aurore/dropdown/listPaddingSides", 232.0f, MetricKind::LENGTH },
    { "aurore/dropdown/listItemHeight", 60.0f, MetricKind::LENGTH },
    { "aurore/dropdown/listItemTextSize", 20.0f, MetricKind::OTHER },

    { "aurore/dropdown/header_height", 70.0f, MetricKind::LENGTH },
    { "aurore/dropdown/header_title_font_size", 24.0f, MetricKind::OTHER },

    // ListItem
    { "aurore/listitem/descriptionIndent", 16.0f, MetricKind::LENGTH },
    { "aurore/listitem/indent", 40.0f, MetricKind::LENGTH },
    { "aurore/listitem/selectRadius", 15.0f, MetricKind::LENGTH },

    // Hints
    { "aurore/hints/footer_margin_sides", 30.0f, MetricKind::LENGTH },
    { "aurore/hints/footer_padding_sides", 25.0f, MetricKind::LENGTH },
    { "aurore/hints/footer_padding_top_bottom", 8.0f, MetricKind::LENGTH },

    // Spinner
    { "aurore/spinner/center_gap_multiplier_large", 0.207f, MetricKind::OTHER },
    { "aurore/spinner/bar_width_multiplier_large", 0.034f, MetricKind::OTHER },
    { "aurore/spinner/center_gap_multiplier", 0.2f, MetricKind::OTHER },
    { "aurore/spinner/bar_width_multiplier", 0.06f, MetricKind::OTHER },
    { "aurore/spinner/animation_duration", 1000, MetricKind::OTHER },

    // Dialog
    { "aurore/dialog/paddingTopBottom", 65, MetricKind::LENGTH },
    { "aurore/dialog/paddingLeftRight", 115, MetricKind::LENGTH },

    { "aurore/dialog/fontSize", 24, MetricKind::OTHER },
};

static Style style(&styleValues);

static float density = DENSITY_COMFORTABLE;

void setDensity(float value)
{
    if (std::isnan(value))
        return;
    density = std::clamp(value, DENSITY_MIN, DENSITY_MAX);
}

float getDensity()
{
    return density;
}

float dp(float length)
{
    return length * density;
}

bool parseDensity(const std::string& text, float* value)
{
    if (text == "comfortable")
        *value = DENSITY_COMFORTABLE;
    else if (text == "compact")
        *value = DENSITY_COMPACT;
    else if (text == "dense")
        *value = DENSITY_DENSE;
    else
    {
        try
        {
            size_t used  = 0;
            float number = std::stof(text, &used);
            if (used != text.size() || std::isnan(number))
                return false;
            *value = number;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return true;
}

Style getStyle()
{
    return style;
}

StyleValues::StyleValues(std::initializer_list<std::pair<std::string, float>> list)
{
    for (std::pair<std::string, float> metric : list)
        this->base.insert(metric);
    this->values = this->base;
}

StyleValues::StyleValues(std::initializer_list<StyleMetric> list)
{
    for (const StyleMetric& metric : list)
    {
        this->base[metric.name] = metric.value;
        if (metric.kind == MetricKind::LENGTH)
            this->kinds[metric.name] = MetricKind::LENGTH;
    }
    this->values = this->base;
}

void StyleValues::addMetric(const std::string& key, float metric)
{
    this->base[key] = metric;
    if (this->overlay.count(key) == 0)
        this->values[key] = metric;
}

void StyleValues::addLength(const std::string& key, float metric)
{
    this->kinds[key] = MetricKind::LENGTH;
    this->addMetric(key, metric);
}

MetricKind StyleValues::getKind(const std::string& name) const
{
    auto it = this->kinds.find(name);
    return it == this->kinds.end() ? MetricKind::OTHER : it->second;
}

bool StyleValues::hasMetric(const std::string& name) const
{
    return this->values.count(name) > 0;
}

void StyleValues::setOverlay(const std::unordered_map<std::string, float>& overlay)
{
    this->overlay = overlay;
    this->values  = this->base;
    for (auto& [name, metric] : overlay)
        this->values[name] = metric;
}

float StyleValues::getRawMetric(const std::string& name)
{
    auto it = this->values.find(name);
    if (it == this->values.end())
    {
        aurore::Logger::error("Unknown style metric {} in size: {}", name, std::to_string(this->values.size()));
        return 0;
    }

    float metric = it->second;
    if (Inspector::recording)
        Inspector::recordMetric(it->first, metric);
    return metric;
}

float StyleValues::getMetric(const std::string& name)
{
    float metric = this->getRawMetric(name);
    return this->getKind(name) == MetricKind::LENGTH ? dp(metric) : metric;
}

Style::Style(StyleValues* values)
    : values(values)
{
}

float Style::getMetric(const std::string& name)
{
    return this->values->getMetric(name);
}

bool Style::hasMetric(const std::string& name)
{
    return this->values->hasMetric(name);
}

void Style::addMetric(const std::string& name, float metric)
{
    return this->values->addMetric(name, metric);
}

void Style::addLength(const std::string& name, float metric)
{
    return this->values->addLength(name, metric);
}

float Style::operator[](const std::string& name)
{
    return this->getMetric(name);
}

/*
HorizonStyle::HorizonStyle()
{
    this->AppletFrame = {
        .headerHeightRegular = ,
        .headerHeightPopup   = 129,

        .imageLeftPadding = 64,
        .imageTopPadding  = 20,
        .imageSize        = 52,
        .separatorSpacing = 30,

        .titleSize   = 28,
        .titleStart  = 130,
        .titleOffset = 5,

        .footerTextSize    = 22,
        .footerTextSpacing = 30,

        .slideAnimation = 20
    };

    this->Highlight = {

    };

    this->Background = {
        .sidebarBorderHeight = 16
    };

    this->Sidebar = {
        .width   = 410,
        .spacing = 0,

        .marginLeft   = 88,
        .marginRight  = 30,
        .marginTop    = 40,
        .marginBottom = 40,

        .Item = {
            .height   = 70,
            .textSize = 22,
            .padding  = 9,

            .textOffsetX       = 14,
            .activeMarkerWidth = 4,
        },

        .Separator = { .height = 28 }
    };

    this->List = {
        .marginLeftRight = 60,
        .marginTopBottom = 42,
        .spacing         = 61,

        .Item = {
            .height             = 69, // offset by 1 to have the highlight hide the separator
            .heightWithSubLabel = 99,
            .valueSize          = 20,
            .padding            = 15,
            .thumbnailPadding   = 11,

            .descriptionIndent  = 20,
            .descriptionSpacing = 16,

            .indent = 40,

            .selectRadius = 15 }
    };

    this->Label = {
        .regularFontSize      = 20,
        .mediumFontSize       = 18,
        .smallFontSize        = 16,
        .descriptionFontSize  = 16,
        .crashFontSize        = 24,
        .buttonFontSize       = 24,
        .listItemFontSize     = 24,
        .notificationFontSize = 18,
        .dialogFontSize       = 24,
        .hintFontSize         = 22,

        .lineHeight             = 1.65f,
        .notificationLineHeight = 1.35f
    };

    this->CrashFrame = {
        .labelWidth     = 0.60f,
        .boxStrokeWidth = 5,
        .boxSize        = 64,
        .boxSpacing     = 90,
        .buttonWidth    = 356,
        .buttonHeight   = 60,
        .buttonSpacing  = 47
    };

    this->Button = {
        .cornerRadius = 5.0f,

        .highlightInset = 2,

        .shadowWidth   = 2.0f,
        .shadowFeather = 10.0f,
        .shadowOpacity = 63.75f,
        .shadowOffset  = 10.0f,

        .borderedBorderThickness = 2,
        .regularBorderThickness  = 2
    };

    this->TableRow = {
        .headerHeight   = 60,
        .headerTextSize = 22,

        .bodyHeight   = 38,
        .bodyIndent   = 40,
        .bodyTextSize = 18,

        .padding = 15
    };

    this->Dropdown = {
        .listWidth   = 720,
        .listPadding = 40,

        .listItemHeight   = 60,
        .listItemTextSize = 20,

        .headerHeight   = 71,
        .headerFontSize = 24,
        .headerPadding  = 70
    };

    this->PopupFrame = {
        .edgePadding      = 120,
        .separatorSpacing = 30,
        .footerHeight     = 73,
        .imageLeftPadding = 60,
        .imageTopPadding  = 17,
        .imageSize        = 100,
        .contentWidth     = 1040,
        .contentHeight    = 518,

        .headerTextLeftPadding = 180,
        .headerTextTopPadding  = 64,

        .subTitleLeftPadding = 182,
        .subTitleTopPadding  = 95,
        .subTitleSpacing     = 20,

        .subTitleSeparatorLeftPadding = 280,
        .subTitleSeparatorTopPadding  = 92,
        .subTitleSeparatorHeight      = 20,

        .headerFontSize   = 28,
        .subTitleFontSize = 16
    };

    this->StagedAppletFrame = {
        .progressIndicatorSpacing          = 4,
        .progressIndicatorRadiusUnselected = 5 - 1, // minus half of border width
        .progressIndicatorRadiusSelected   = 8,
        .progressIndicatorBorderWidth      = 2
    };

    this->ProgressSpinner = {
        .centerGapMultiplier = 0.2f,
        .barWidthMultiplier  = 0.06f
    };

    this->ProgressDisplay = {
        .percentageLabelWidth = 70
    };

    this->Header = {
        .height  = 44,
        .padding = 11,

        .rectangleWidth = 5,

        .fontSize = 18
    };

    this->FramerateCounter = {
        .width  = 125,
        .height = 26
    };

    this->ThumbnailSidebar = {
        .marginLeftRight = 109, // used for the image only = (410 - 192) / 2, image size is 192*192 with a 410px wide sidebar
        .marginTopBottom = 47,

        .buttonHeight = 70,
        .buttonMargin = 60
    };

    this->AnimationDuration = {
        .showSlide = 125,

        .highlight = 100,
        .shake     = 15,

        .collapse = 100,

        .progress = 1000,

        .notificationTimeout = 4000
    };

    this->Notification = {
        .width   = 280,
        .padding = 16,

        .slideAnimation = 40
    };

    this->Dialog = {
        .width  = 770,
        .height = 220,

        .paddingTopBottom = 65,
        .paddingLeftRight = 115,

        .cornerRadius = 5.0f,

        .buttonHeight          = 72,
        .buttonSeparatorHeight = 2,

        .shadowWidth   = 2.0f,
        .shadowFeather = 10.0f,
        .shadowOpacity = 63.75f,
        .shadowOffset  = 10.0f
    };
}
*/
} // namespace aurore
