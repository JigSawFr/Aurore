# XML attributes

Every attribute the library's views read from XML, by view: those of `View` are on every view, those of `Box` on every box (most views are boxes). Written by `tools/xml_attributes.py` from the sources; a test checks it is up to date. Styles per state (`focused:…`) are in [styles.md](styles.md), the theme keys (`@theme/…`) in [themes.md](themes.md).

A misspelt attribute or a value outside the list stops the app with the file, the line and the closest name or the values expected (`View::fatalXML`).

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

## Every box (`<brls:Box>`)

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

## `<brls:AppletFrame>`

| Attribute | Value |
|---|---|
| `footerHidden` | `true` / `false` |
| `headerHidden` | `true` / `false` |
| `style` | `popup` · `regular` |

## `<brls:Button>`

| Attribute | Value |
|---|---|
| `state` | `disabled` · `enabled` |
| `style` | `bordered` · `borderless` · `default` · `highlight` · `primary` |
| `textColor` | colour (`#rrggbb`, `@theme/…`) |

## `<brls:DetailCell>`

| Attribute | Value |
|---|---|
| `title` | text (`@i18n/…`) |

## `<brls:HRecyclerFrame>`

| Attribute | Value |
|---|---|
| `itemSpace` | number |
| `itemWidth` | number |
| `preFetchLine` | number |
| `spanCount` | number |

## `<brls:HScrollingFrame>`

| Attribute | Value |
|---|---|
| `scrollingBehavior` | `centered` · `natural` |

## `<brls:Header>`

| Attribute | Value |
|---|---|
| `subtitle` | text (`@i18n/…`) |
| `title` | text (`@i18n/…`) |

## `<brls:Hints>`

| Attribute | Value |
|---|---|
| `addBaseAction` | `true` / `false` |
| `allowAButtonTouch` | `true` / `false` |
| `forceShown` | `true` / `false` |

## `<brls:IconButton>`

| Attribute | Value |
|---|---|
| `glyph` | text (`@i18n/…`) |
| `icon` | file (`@res/…`) |
| `iconSize` | number |

## `<brls:Image>`

| Attribute | Value |
|---|---|
| `image` | file (`@res/…`) |
| `imageAlign` | `bottom` · `center` · `left` · `right` · `top` |
| `interpolation` | `linear` · `nearest` |
| `scalingType` | `center` · `fill` · `fit` · `stretch` |
| `themeImage` | text (`@i18n/…`) |

## `<brls:InputCell>`

| Attribute | Value |
|---|---|
| `secure` | `true` / `false` |

## `<brls:Label>`

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

## `<brls:ProgressSpinner>`

| Attribute | Value |
|---|---|
| `barColor` | colour (`#rrggbb`, `@theme/…`) |
| `size` | `large` · `normal` |

## `<brls:RadioCell>`

| Attribute | Value |
|---|---|
| `title` | text (`@i18n/…`) |

## `<brls:Rectangle>`

| Attribute | Value |
|---|---|
| `color` | colour (`#rrggbb`, `@theme/…`) |

## `<brls:RecyclerFrame>`

| Attribute | Value |
|---|---|
| `padding` | number |
| `paddingBottom` | number |
| `paddingLeft` | number |
| `paddingRight` | number |
| `paddingTop` | number |

## `<brls:RecyclingGrid>`

| Attribute | Value |
|---|---|
| `columnWidth` | number |
| `itemHeight` | number |
| `itemSpace` | number |
| `preFetchLine` | number |
| `spanCount` | number |

## `<brls:ScrollingFrame>`

| Attribute | Value |
|---|---|
| `scrollingBehavior` | `centered` · `natural` |
