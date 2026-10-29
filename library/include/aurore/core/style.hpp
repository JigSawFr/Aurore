/*
    Copyright 2019-2020 natinusala
    Copyright 2019 WerWolv
    Copyright 2019 p-sam

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

#pragma once

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>

namespace aurore
{

/// What a metric is: a length (a size, a margin, a radius, in layout units)
/// follows the density (setDensity), anything else (a duration, a font size,
/// an opacity, a ratio) does not.
enum class MetricKind
{
    OTHER,
    LENGTH,
};

/// A metric of the base, with its kind.
struct StyleMetric
{
    StyleMetric(std::string name, float value, MetricKind kind)
        : name(std::move(name))
        , value(value)
        , kind(kind)
    {
    }

    std::string name;
    float value;
    MetricKind kind;
};

/// The density: lengths drawn that many times their size (aurore::dp()).
inline constexpr float DENSITY_COMFORTABLE = 1.0f; ///< the default
inline constexpr float DENSITY_COMPACT     = 0.85f;
inline constexpr float DENSITY_DENSE       = 0.7f; ///< small screens
inline constexpr float DENSITY_MIN         = 0.5f;
inline constexpr float DENSITY_MAX         = 1.5f;

/**
 * Sets the density, between DENSITY_MIN and DENSITY_MAX (a value outside is
 * brought back in, NaN ignored). It scales the metrics of kind LENGTH, the XML
 * numbers in `dp` and aurore::dp(), never the text (Application::setTextScale).
 * The views read their sizes when they are made: set it before the screens
 * are built, or build them again.
 */
void setDensity(float density);
/// The density in use (DENSITY_COMFORTABLE by default).
float getDensity();
/// `length` at the density in use: what `16dp` is in XML.
float dp(float length);
/**
 * A density by name: "comfortable", "compact", "dense", or a number
 * ("0.8"); false for anything else (and NaN).
 */
bool parseDensity(const std::string& text, float* density);

/// The metrics (sizes, margins, durations) by name, "aurore/sidebar/width"…: the ones
/// the code adds, and over them a theme file's `[metrics]`.
class StyleValues
{
  public:
    /// The base metrics, of kind OTHER.
    StyleValues(std::initializer_list<std::pair<std::string, float>> list);
    /// The base metrics, each with its kind.
    StyleValues(std::initializer_list<StyleMetric> list);

    /// A metric of the base: what it is without a theme file. A name already
    /// known keeps its kind; a new one is of kind OTHER.
    void addMetric(const std::string&, float value);
    /// A length of the base (kind LENGTH): it follows the density.
    void addLength(const std::string& name, float value);
    /// A metric, a length at the density in use; an unknown name is logged as
    /// an error and gives 0.
    float getMetric(const std::string& name);
    /// The metric as written, before the density.
    float getRawMetric(const std::string& name);
    /// The kind of a metric (OTHER when unknown).
    MetricKind getKind(const std::string& name) const;
    /// Whether the metric exists.
    bool hasMetric(const std::string& name) const;

    /// The theme file's metrics, over the base (empty: the base alone).
    void setOverlay(const std::unordered_map<std::string, float>& overlay);

  private:
    std::unordered_map<std::string, float> base;
    std::unordered_map<std::string, float> overlay;
    std::unordered_map<std::string, float> values; ///< base, then overlay
    std::unordered_map<std::string, MetricKind> kinds; ///< the lengths only
};

/// Simple wrapper around StyleValues for the array operator
class Style
{
  public:
    /// A handle on `values` (not owned).
    Style(StyleValues* values);
    /// The metric `name` (getMetric()).
    float operator[](const std::string& name);

    /// Adds or changes a metric of the base (StyleValues::addMetric).
    void addMetric(const std::string& name, float value);
    /// Adds or changes a length of the base (StyleValues::addLength).
    void addLength(const std::string& name, float value);
    /// A metric; an unknown name is logged as an error and gives 0.
    float getMetric(const std::string& name);
    /// Whether the metric exists.
    bool hasMetric(const std::string& name);
    /// The metrics behind the handle.
    StyleValues* getValues() { return this->values; }

  private:
    StyleValues* values;
};

/// The app's metrics: what XML reads with `@style/…`.
Style getStyle();

} // namespace aurore
