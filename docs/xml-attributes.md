# XML attributes

Every attribute the library's views read from XML, by view: those of `View` are on every view, those of `Box` on every box (most views are boxes). Written by `tools/xml_attributes.py` from the sources; a test checks it is up to date. Styles per state (`focused:…`) are in [styles.md](styles.md), the theme keys (`@theme/…`) in [themes.md](themes.md).

A misspelt attribute or a value outside the list stops the app with the file, the line and the closest name or the values expected (`View::fatalXML`).

A number is written `16`, `16px` (the same), `16dp` (16 at the density in use: 11.2 when dense, see [styles.md](styles.md#density-of-the-lengths)), `25%` where a percentage is taken, or `@style/<name>` (a length of the style follows the density too).

Any attribute prefixed with `compact:`, `medium:` or `expanded:` applies on a screen of that width only, over the plain one ([styles.md](styles.md#screen-width)).

## Every view (`View`)

| Attribute | Value |
|---|---|
| `accessibilityLabel` | text (`@i18n/…`) |
| `accessibilityRole` | text (`@i18n/…`) |
| `accessibilitySelected` | `true` / `false` |
| `accessibilityValue` | text (`@i18n/…`) |
| `alignSelf` | `auto` · `baseline` · `center` · `flexEnd` · `flexStart` · `spaceAround` · `spaceBetween` · `stretch` |
| `alpha` | number |
| `aspectRatio` | number |
| `background` | `backdrop` · `horizontal_linear` · `sidebar` · `vertical_linear` |
| `backgroundBottomLeftRadius` | number |
| `backgroundBottomRightRadius` | number |
| `backgroundColor` | colour (`#rrggbb`, `@theme/…`) |
| `backgroundEndColor` | colour (`#rrggbb`, `@theme/…`) |
| `backgroundStartColor` | colour (`#rrggbb`, `@theme/…`) |
| `backgroundTopLeftRadius` | number |
| `backgroundTopRightRadius` | number |
| `bind` | text (`@i18n/…`) |
| `borderColor` | colour (`#rrggbb`, `@theme/…`) |
| `borderThickness` | number |
| `boxSizing` | `borderBox` · `contentBox` |
| `clipsToBounds` | `true` / `false` |
| `cornerBottomLeftRadius` | number |
| `cornerBottomRightRadius` | number |
| `cornerRadius` | number |
| `cornerTopLeftRadius` | number |
| `cornerTopRightRadius` | number |
| `culled` | `true` / `false` |
| `detachedX` | number |
| `detachedY` | number |
| `focusable` | `true` / `false` |
| `focusDown` | text (`@i18n/…`) |
| `focusLeft` | text (`@i18n/…`) |
| `focusRight` | text (`@i18n/…`) |
| `focusUp` | text (`@i18n/…`) |
| `grow` | number |
| `height` | `auto`, number, percentage (`50%`) |
| `hideClickAnimation` | `true` / `false` |
| `hideHighlight` | `true` / `false` |
| `hideHighlightBackground` | `true` / `false` |
| `hideHighlightBorder` | `true` / `false` |
| `highlightCornerRadius` | number |
| `highlightPadding` | number |
| `icon` | file (`@res/…`) |
| `id` | text (`@i18n/…`) |
| `lineBottom` | number |
| `lineColor` | colour (`#rrggbb`, `@theme/…`) |
| `lineLeft` | number |
| `lineRight` | number |
| `lineTop` | number |
| `margin` | `auto`, number |
| `marginBottom` | `auto`, number |
| `marginLeft` | `auto`, number |
| `marginRight` | `auto`, number |
| `marginTop` | `auto`, number |
| `maxHeight` | `auto`, number, percentage (`50%`) |
| `maxWidth` | `auto`, number, percentage (`50%`) |
| `minHeight` | `auto`, number, percentage (`50%`) |
| `minWidth` | `auto`, number, percentage (`50%`) |
| `positionBottom` | number, percentage (`50%`) |
| `positionLeft` | number, percentage (`50%`) |
| `positionRight` | number, percentage (`50%`) |
| `positionTop` | number, percentage (`50%`) |
| `positionType` | `absolute` · `relative` · `static` |
| `scale` | number |
| `shadowType` | `custom` · `generic` · `none` |
| `shrink` | number |
| `title` | text (`@i18n/…`) |
| `transition` | text (`@i18n/…`) |
| `visibility` | `gone` · `invisible` · `visible` |
| `width` | `auto`, number, percentage (`50%`) |
| `wireframe` | `true` / `false` |

## Every box (`<aurore:Box>`)

| Attribute | Value |
|---|---|
| `alignItems` | `auto` · `baseline` · `center` · `flexEnd` · `flexStart` · `spaceAround` · `spaceBetween` · `stretch` |
| `axis` | `column` · `row` |
| `columnGap` | number |
| `direction` | `inherit` · `leftToRight` · `rightToLeft` |
| `flexWrap` | `noWrap` · `wrap` · `wrapReverse` |
| `gap` | number |
| `justifyContent` | `center` · `flexEnd` · `flexStart` · `spaceAround` · `spaceBetween` · `spaceEvenly` |
| `padding` | number |
| `paddingBottom` | number |
| `paddingLeft` | number |
| `paddingRight` | number |
| `paddingTop` | number |
| `rowGap` | number |
| `safeArea` | edges joined with `\|`: `top` · `right` · `bottom` · `left` · `vertical` · `horizontal` · `all` · `none`; or `auto` ([safe areas](safe-areas.md)) |

## `CadranDigits`

| Attribute | Value |
|---|---|
| `fitParent` | `true` / `false` |
| `fitWidth` | number |
| `followsTextSize` | `true` / `false` |
| `fontSize` | number |
| `text` | text (`@i18n/…`) |
| `textColor` | colour (`#rrggbb`, `@theme/…`) |

## `CadranIcon`

| Attribute | Value |
|---|---|
| `color` | colour (`#rrggbb`, `@theme/…`) |
| `icon` | text (`@i18n/…`) |

## `SidebarItem`

| Attribute | Value |
|---|---|
| `label` | text (`@i18n/…`) |

## `<aurore:AppletFrame>`

| Attribute | Value |
|---|---|
| `footerHidden` | `true` / `false` |
| `headerHidden` | `true` / `false` |
| `style` | `popup` · `regular` |

## `<aurore:Button>`

| Attribute | Value |
|---|---|
| `state` | `disabled` · `enabled` |
| `style` | `bordered` · `borderless` · `default` · `highlight` · `primary` |
| `textColor` | colour (`#rrggbb`, `@theme/…`) |

## `<aurore:DetailCell>`

| Attribute | Value |
|---|---|
| `title` | text (`@i18n/…`) |

## `<aurore:HRecyclerFrame>`

| Attribute | Value |
|---|---|
| `itemSpace` | number |
| `itemWidth` | number |
| `preFetchLine` | number |
| `spanCount` | number |

## `<aurore:HScrollingFrame>`

| Attribute | Value |
|---|---|
| `scrollingBehavior` | `centered` · `natural` |

## `<aurore:Header>`

| Attribute | Value |
|---|---|
| `subtitle` | text (`@i18n/…`) |
| `title` | text (`@i18n/…`) |

## `<aurore:Hints>`

| Attribute | Value |
|---|---|
| `addBaseAction` | `true` / `false` |
| `allowAButtonTouch` | `true` / `false` |
| `forceShown` | `true` / `false` |

## `<aurore:IconButton>`

| Attribute | Value |
|---|---|
| `glyph` | text (`@i18n/…`) |
| `icon` | file (`@res/…`) |
| `iconSize` | number |

## `<aurore:Image>`

| Attribute | Value |
|---|---|
| `image` | file (`@res/…`) |
| `imageAlign` | `bottom` · `center` · `left` · `right` · `top` |
| `interpolation` | `linear` · `nearest` |
| `scalingType` | `center` · `fill` · `fit` · `stretch` |
| `themeImage` | text (`@i18n/…`) |

## `<aurore:InputCell>`

| Attribute | Value |
|---|---|
| `secure` | `true` / `false` |

## `<aurore:Label>`

| Attribute | Value |
|---|---|
| `animated` | `true` / `false` |
| `autoAnimate` | `true` / `false` |
| `cursor` | number |
| `fontFamily` | text (`@i18n/…`) |
| `fontQuality` | number |
| `fontSize` | number |
| `horizontalAlign` | `center` · `left` · `right` |
| `lineHeight` | number |
| `singleLine` | `true` / `false` |
| `text` | text (`@i18n/…`) |
| `textColor` | colour (`#rrggbb`, `@theme/…`) |
| `verticalAlign` | `baseline` · `bottom` · `center` · `top` |

## `<aurore:ProgressSpinner>`

| Attribute | Value |
|---|---|
| `barColor` | colour (`#rrggbb`, `@theme/…`) |
| `size` | `large` · `normal` |

## `<aurore:RadioCell>`

| Attribute | Value |
|---|---|
| `title` | text (`@i18n/…`) |

## `<aurore:Rectangle>`

| Attribute | Value |
|---|---|
| `color` | colour (`#rrggbb`, `@theme/…`) |

## `<aurore:RecyclerFrame>`

| Attribute | Value |
|---|---|
| `padding` | number |
| `paddingBottom` | number |
| `paddingLeft` | number |
| `paddingRight` | number |
| `paddingTop` | number |

## `<aurore:RecyclingGrid>`

| Attribute | Value |
|---|---|
| `columnWidth` | number |
| `itemHeight` | number |
| `itemSpace` | number |
| `preFetchLine` | number |
| `spanCount` | number |

## `<aurore:ScrollingFrame>`

| Attribute | Value |
|---|---|
| `scrollingBehavior` | `centered` · `natural` |
