# Changelog

## 0.1.0 (2026-10-31)


### ⚠ BREAKING CHANGES

* **core:** Aurore's headers no longer bring fmt in. An app that calls fmt:: formats with std::format or links fmt itself, and Logger's arguments must be formattable by std::format.
* **cmake:** a project that links Aurore builds as C++23, with CMake 3.20 or later.
* **apple:** the demo and the documented builds need Xcode 27 and run from iOS 27 and macOS 27 on Apple silicon; CMAKE_OSX_DEPLOYMENT_TARGET, CMAKE_OSX_ARCHITECTURES and DEPLOYMENT_TARGET still lower them.
* **graphics:** AURORE_GRAPHICS=D3D11 is gone; AURORE_GRAPHICS=SDLGPU draws with Direct3D 12 on Windows.
* **sdl:** SDL3 by default on the desktop, in a browser, on Android and iOS ([#92](https://github.com/JigSawFr/Aurore/issues/92))
* **api:** Application::setCadran() and isCadran() are removed; an app that called setCadran(false) is drawn in Cadran's look.
* **api:** only Aurore's names are read. Rename brls::, <borealis.hpp>, BRLS_..., brls:Box, brls/text, BRLS_LANG, -DPLATFORM_DESKTOP=ON (-DAURORE_PLATFORM=DESKTOP), USE_* (AURORE_WINDOW, AURORE_GRAPHICS, AURORE_LIBROMFS), CUSTOM_RESOURCES_DIR (AURORE_RESOURCES_ROOT), ASYNC_RETAIN/TOKEN/RELEASE (aliveToken()) as docs/migrating-from-borealis.md says.

### Features

* **3ds:** antialiased edges in the citro3d renderer ([#114](https://github.com/JigSawFr/Aurore/issues/114)) ([f5a42c4](https://github.com/JigSawFr/Aurore/commit/f5a42c4a2d5acce57374d4fe484ebecd9bb6af13))
* **3ds:** nanovg's citro3d renderer, a test card on the bottom screen ([#90](https://github.com/JigSawFr/Aurore/issues/90)) ([16ffe70](https://github.com/JigSawFr/Aurore/commit/16ffe70bec73bb4c40e3351e77c71577bf8646a2))
* **3ds:** read the buttons, the sticks and the touch screen ([#103](https://github.com/JigSawFr/Aurore/issues/103)) ([7de8c94](https://github.com/JigSawFr/Aurore/commit/7de8c947c7dead18c9455c222fa15f83e1cf856f))
* **3ds:** the demo built for the New 3DS, next to the test card ([#113](https://github.com/JigSawFr/Aurore/issues/113)) ([4ad6f9f](https://github.com/JigSawFr/Aurore/commit/4ad6f9fb6e352d78306ed1a4f3006a01e3906dfa))
* **3ds:** the header and the focus's preview on the top screen ([#110](https://github.com/JigSawFr/Aurore/issues/110)) ([0de4bd9](https://github.com/JigSawFr/Aurore/commit/0de4bd96951f77e464ec4c4ac751ee412921b0c7))
* **3ds:** the system's keyboard, the Wi-Fi, the log, the threads on the third core ([#115](https://github.com/JigSawFr/Aurore/issues/115)) ([c9b1263](https://github.com/JigSawFr/Aurore/commit/c9b126341ab2b618801e27fbf8bd53032d3b8292))
* **3ds:** the top screen, each activity's view or a default header ([#105](https://github.com/JigSawFr/Aurore/issues/105)) ([c341056](https://github.com/JigSawFr/Aurore/commit/c3410568580e997096e66629b370bad4f0f9ce1f))
* **a11y:** screen readers hear each control's value, state and place, and every Cadran piece ([#46](https://github.com/JigSawFr/Aurore/issues/46)) ([d2d05d1](https://github.com/JigSawFr/Aurore/commit/d2d05d1f2ee6004dfff012015b2b7166bec1e14c))
* **cadran:** a Density preference beside the text size ([#78](https://github.com/JigSawFr/Aurore/issues/78)) ([cc8a343](https://github.com/JigSawFr/Aurore/commit/cc8a343675d584773a83402f69870e434bf20a64))
* **cadran:** the kit's sizes follow the density ([#73](https://github.com/JigSawFr/Aurore/issues/73)) ([6b5ed77](https://github.com/JigSawFr/Aurore/commit/6b5ed77a2275bba9458412eaf593b2c48949aaab))
* **compat:** move_only_function, generator and stacktrace where libc++ has none ([#125](https://github.com/JigSawFr/Aurore/issues/125)) ([72e507a](https://github.com/JigSawFr/Aurore/commit/72e507a17280572b74e85273f445d88e969aa66a))
* **core:** safe areas, so notches, rounded corners, system bars and a TV's overscan stay clear, edge to edge ([#62](https://github.com/JigSawFr/Aurore/issues/62)) ([c1e7b8e](https://github.com/JigSawFr/Aurore/commit/c1e7b8e8c31f31dccab4e242731e51ca7327a4fa))
* **demo:** a guided showcase of Aurore, a tab per subject ([#81](https://github.com/JigSawFr/Aurore/issues/81)) ([e45daa4](https://github.com/JigSawFr/Aurore/commit/e45daa45da7515ab160feca344b6660f842c9cdc))
* **demo:** key art for the Library's covers, drawn as SVG and rendered in Chromium ([#108](https://github.com/JigSawFr/Aurore/issues/108)) ([ac570be](https://github.com/JigSawFr/Aurore/commit/ac570be2dbdfaf2018e0ae5e2c9a9d8d3a842093))
* **demo:** simulated platforms, the console drawn around its screen ([#93](https://github.com/JigSawFr/Aurore/issues/93)) ([c3b2e07](https://github.com/JigSawFr/Aurore/commit/c3b2e07f4ad0903b8987c53895e690fb4a24d34e))
* **demo:** the demo split between the New 3DS's two screens ([#111](https://github.com/JigSawFr/Aurore/issues/111)) ([6c3b146](https://github.com/JigSawFr/Aurore/commit/6c3b146a018acbd4b504f75d012d3f7884b42eea))
* **dev-tools:** Application::isSettled(), and AURORE_SETTLE_FILE for a test driving the app from outside ([#99](https://github.com/JigSawFr/Aurore/issues/99)) ([bbfbafa](https://github.com/JigSawFr/Aurore/commit/bbfbafa4ec6e39c963c0373d56e101e53b7356b4))
* **dev-tools:** Application::setBusyCheck(), the app's own work in isSettled() ([#107](https://github.com/JigSawFr/Aurore/issues/107)) ([c5ec75e](https://github.com/JigSawFr/Aurore/commit/c5ec75e89e3faef62eed8e3ab97e68fb4bee10d3))
* **graphics:** SDL_GPU, Direct3D 12 in place of Direct3D 11 ([#104](https://github.com/JigSawFr/Aurore/issues/104)) ([be0ae30](https://github.com/JigSawFr/Aurore/commit/be0ae30d5805525ad709aa7875bb333bf537bb91))
* **layout:** classes of screen width, compact: attributes in XML, and compact layouts for small screens ([#74](https://github.com/JigSawFr/Aurore/issues/74)) ([cbd174e](https://github.com/JigSawFr/Aurore/commit/cbd174e57713a2673ec4c97c0fa1f96541f19c46))
* **layout:** every screen of the demo and Cadran's sheets fit a compact screen ([#77](https://github.com/JigSawFr/Aurore/issues/77)) ([d77f9b9](https://github.com/JigSawFr/Aurore/commit/d77f9b90a48eeaf59aaf83a4b43d3a019e2cffe5))
* **layout:** layout presets, a small screen's in one go ([#79](https://github.com/JigSawFr/Aurore/issues/79)) ([3030a23](https://github.com/JigSawFr/Aurore/commit/3030a234f98762c62050725dfe5aad47f660f348))
* **names:** borealis' environment variables, XML tags and theme keys still read ([02bbcf1](https://github.com/JigSawFr/Aurore/commit/02bbcf1e37b3b4e4aa5c91170854638a1027a711))
* **names:** the borealis headers and names, kept as deprecated aliases ([8e018d0](https://github.com/JigSawFr/Aurore/commit/8e018d0f1a0acb9ec9ccd63681376d585ed3ad0c))
* **sdl:** SDL3 beside SDL2 on the desktop, in a browser, on Android and iOS ([#70](https://github.com/JigSawFr/Aurore/issues/70)) ([69e735a](https://github.com/JigSawFr/Aurore/commit/69e735aebb32bfdf2b491801402521ef3939b872))
* **sdl:** SDL3 by default on the desktop, in a browser, on Android and iOS ([#92](https://github.com/JigSawFr/Aurore/issues/92)) ([01223ed](https://github.com/JigSawFr/Aurore/commit/01223ed632dd37e352b98c4500902d2b4cdba3ef))
* **sdl:** the battery as SDL reads it, Linux's included ([#116](https://github.com/JigSawFr/Aurore/issues/116)) ([301382d](https://github.com/JigSawFr/Aurore/commit/301382d7286ba1ab30b7816df7d926e100b36fd3))
* **site:** a menu of Boréale's sites beside the name ([#64](https://github.com/JigSawFr/Aurore/issues/64)) ([4e69379](https://github.com/JigSawFr/Aurore/commit/4e69379eba184b5383e2296a7a208240f0dc0a8b))
* **site:** a page on what Aurore adds to and changes from borealis ([#80](https://github.com/JigSawFr/Aurore/issues/80)) ([0f2bf9f](https://github.com/JigSawFr/Aurore/commit/0f2bf9fb54e9e7d2fbee8fa2fa0e417a7ec16efa))
* **site:** Atlas and Lantern in the menu of Boréale's sites ([#133](https://github.com/JigSawFr/Aurore/issues/133)) ([e72417d](https://github.com/JigSawFr/Aurore/commit/e72417dc9e269d31b9bc9f8cfbaaba78b8bef22e))
* **site:** Boréale's footer column, a tidier phone header and easier reading of the documentation ([#69](https://github.com/JigSawFr/Aurore/issues/69)) ([7b394a8](https://github.com/JigSawFr/Aurore/commit/7b394a8ab455ae6923efc5dda21d67500a38ee48))
* **site:** count the audience with Umami Cloud ([#60](https://github.com/JigSawFr/Aurore/issues/60)) ([cec9d03](https://github.com/JigSawFr/Aurore/commit/cec9d03b7c9ac0a96dc41c154154d8daa7e01e51))
* **site:** count the published demo's visits with Umami, without cookies ([#124](https://github.com/JigSawFr/Aurore/issues/124)) ([b91d17d](https://github.com/JigSawFr/Aurore/commit/b91d17d957e880aaf4f0e1e6720f56948b34e3cb))
* **site:** tag links to our sites with UTM parameters for Umami ([#127](https://github.com/JigSawFr/Aurore/issues/127)) ([bdbd3f8](https://github.com/JigSawFr/Aurore/commit/bdbd3f80d46ef5812135e814e9eb157d6fe408dd))
* **site:** the API reference made of the site's own pages ([#76](https://github.com/JigSawFr/Aurore/issues/76)) ([2c6e326](https://github.com/JigSawFr/Aurore/commit/2c6e32648bb6aad619808ff1e01ce55050356518))
* **site:** the demo on a console, the code in an editor beside its screen, and the night down the page ([#72](https://github.com/JigSawFr/Aurore/issues/72)) ([e8f7649](https://github.com/JigSawFr/Aurore/commit/e8f76498388644d3d05c326160b92bbc28dc7ed3))
* **site:** the demo on each console, "On every screen" on the home page ([#98](https://github.com/JigSawFr/Aurore/issues/98)) ([15baf34](https://github.com/JigSawFr/Aurore/commit/15baf346f726e7534cb6ce426ae96f279d7cc372))
* **site:** the documentation's bar and light reading ([c0a6d7d](https://github.com/JigSawFr/Aurore/commit/c0a6d7dc1c88dfa1d015fe3fba09fae64134f690))
* **site:** the header and hero of Boréale's other sites ([#143](https://github.com/JigSawFr/Aurore/issues/143)) ([9e3dee9](https://github.com/JigSawFr/Aurore/commit/9e3dee97ab3e3c746ecd3be8d5224aad4ccbd7f1))
* **site:** the page not found under the aurora, and Figtree preloaded ([79c98b3](https://github.com/JigSawFr/Aurore/commit/79c98b3a8bfc6a284e785ef48eadce4382a29cda))
* **site:** the projects' versions in Boréale's menu, and a Boréale column in the footer ([7a4a035](https://github.com/JigSawFr/Aurore/commit/7a4a035a3a78a9c083d692ed58f711bedb8cf693))
* **site:** the website in the consoles' 15 languages, with flags in the language menu ([#75](https://github.com/JigSawFr/Aurore/issues/75)) ([486df13](https://github.com/JigSawFr/Aurore/commit/486df13645581eb92877d6fedbd34555d6bef33a))
* **site:** the website, with the documentation, the API reference and the demo in a browser ([#59](https://github.com/JigSawFr/Aurore/issues/59)) ([fb48bec](https://github.com/JigSawFr/Aurore/commit/fb48becb909c35930f3f987f3c2be3ff1a8c87c0))
* **site:** the whole name opens the menu of Boréale's sites ([#65](https://github.com/JigSawFr/Aurore/issues/65)) ([0aabe8f](https://github.com/JigSawFr/Aurore/commit/0aabe8fe5cdc498076a7ee9cfb80cf1cc47d2444))
* **site:** under the hood, the versions and how the code is checked, read from the repository ([#85](https://github.com/JigSawFr/Aurore/issues/85)) ([74cff27](https://github.com/JigSawFr/Aurore/commit/74cff27bfa7a73e8b46d49dc30740edab19c4578))
* **style:** a density setting for the lengths, and dp in XML ([#67](https://github.com/JigSawFr/Aurore/issues/67)) ([bc1fd1c](https://github.com/JigSawFr/Aurore/commit/bc1fd1ca2655c20103f9539bd025f787102c22e4))
* **theme:** a schema key for the format of a theme file ([#126](https://github.com/JigSawFr/Aurore/issues/126)) ([9609b68](https://github.com/JigSawFr/Aurore/commit/9609b68ca3716dc685f718cabf43f7a962c04ea4))
* **web:** the demo's page as PlayGuard's and PlayShelf's simulators ([#106](https://github.com/JigSawFr/Aurore/issues/106)) ([7fc21e4](https://github.com/JigSawFr/Aurore/commit/7fc21e40755314f3f33b17fd22b6f7235a936941))


### Bug Fixes

* **core:** tasks coming back to a deleted view do nothing, and events and logging are thread-safe ([#49](https://github.com/JigSawFr/Aurore/issues/49)) ([a0ff125](https://github.com/JigSawFr/Aurore/commit/a0ff125c4a1782e2b67396c4cbb60a1b10506bf9))
* **deko3d:** keep the compiled shaders so an app builds the same bytes twice ([#144](https://github.com/JigSawFr/Aurore/issues/144)) ([31a9aa0](https://github.com/JigSawFr/Aurore/commit/31a9aa0be7c64eb2ef3283a8090e9596131f9aef))
* **demo:** one Umami visit on a first load, not two ([#139](https://github.com/JigSawFr/Aurore/issues/139)) ([2f8ebd8](https://github.com/JigSawFr/Aurore/commit/2f8ebd89fdfc24af85b5594b250b0eb2c08d30ba))
* **fonts:** free fonts and pictures in place of Nintendo's, CJK text from the system ([#45](https://github.com/JigSawFr/Aurore/issues/45)) ([0012fa3](https://github.com/JigSawFr/Aurore/commit/0012fa3d78b11e6b26a4adcafadfcae1c720aed0))
* **image:** refuse oversized pictures before decoding, and harden the image and font parsers ([#41](https://github.com/JigSawFr/Aurore/issues/41)) ([85dca92](https://github.com/JigSawFr/Aurore/commit/85dca92ebcce7caae812a78addc0d0890d774eb6))
* **input:** quick clicks count, X/Y/L/R/+/− have keys and Left leaves a carousel ([#47](https://github.com/JigSawFr/Aurore/issues/47)) ([76fabf9](https://github.com/JigSawFr/Aurore/commit/76fabf9434d8d51b6ef6156d5fc2fd598884e032))
* **platform:** open links in the browser without a shell, http and https only ([#42](https://github.com/JigSawFr/Aurore/issues/42)) ([d287ebb](https://github.com/JigSawFr/Aurore/commit/d287ebb5d25eebcf4d48e4eb3c327602b9b36502))
* **sdl:** keep the web canvas at the console's size, and leave the browser's battery out ([#122](https://github.com/JigSawFr/Aurore/issues/122)) ([6e28c0e](https://github.com/JigSawFr/Aurore/commit/6e28c0ea2f7be40b770e8ecb702a7e7065d188a3))
* **sdl:** the middle mouse button as Ⓧ and the right one as Ⓑ, as under GLFW ([#119](https://github.com/JigSawFr/Aurore/issues/119)) ([4da73de](https://github.com/JigSawFr/Aurore/commit/4da73dee7bfe47a7a8c02a723688056ba2ebae73))
* **security:** no debugging aids from the environment in release builds, Android 14 target, and safer theme pictures, input text and decoding ([#54](https://github.com/JigSawFr/Aurore/issues/54)) ([00f75b1](https://github.com/JigSawFr/Aurore/commit/00f75b1574a52eb03f570e5848eeba0ff0222b71))
* **site:** links out say nothing of the visit, but to our own sites ([#121](https://github.com/JigSawFr/Aurore/issues/121)) ([5591f5f](https://github.com/JigSawFr/Aurore/commit/5591f5f57f6131a4c5234e2029db0ff4be813ca6))
* **site:** no e-mail address in the footer, contact goes through GitHub ([#138](https://github.com/JigSawFr/Aurore/issues/138)) ([6dcb013](https://github.com/JigSawFr/Aurore/commit/6dcb0134fc8d3036e2fc94336cf09bafe3e9dc16))
* **site:** the gallery's captions say what the new demo's pictures show ([#83](https://github.com/JigSawFr/Aurore/issues/83)) ([55f431d](https://github.com/JigSawFr/Aurore/commit/55f431d965667101b25bffdbf817365f45f5f94b))
* **site:** the phone's header on two tidy rows, GitHub left to the footer ([b4cfafc](https://github.com/JigSawFr/Aurore/commit/b4cfafc1bdd6b01177789b0e14ff9203bba07b61))
* **switch:** the video context is null until it is built ([#145](https://github.com/JigSawFr/Aurore/issues/145)) ([97e479b](https://github.com/JigSawFr/Aurore/commit/97e479b0b9b4b23b0202c61087e219e82fb3ddbc))
* **theme:** a theme can no longer hang the app or read files outside its folder ([#43](https://github.com/JigSawFr/Aurore/issues/43)) ([fac029e](https://github.com/JigSawFr/Aurore/commit/fac029e026a0e83929069014a1bfa282963e2982))
* **ux:** hot reload keeps the screen, the desktop follows BRLS_LANG, a very large text size and a Cadran text field ([#53](https://github.com/JigSawFr/Aurore/issues/53)) ([d245c41](https://github.com/JigSawFr/Aurore/commit/d245c41fafb953b6dbaac6312779ad40de7bebb0))
* **ux:** readable Off values, contrast checked on more pairs, XML errors with file and line, reduced motion in every view ([#48](https://github.com/JigSawFr/Aurore/issues/48)) ([592d7c9](https://github.com/JigSawFr/Aurore/commit/592d7c9f2ee065dc0235b09f8382e99ce7f0cce2))
* **view:** no self-hiding scroll indicator in still frames ([#140](https://github.com/JigSawFr/Aurore/issues/140)) ([2f1602a](https://github.com/JigSawFr/Aurore/commit/2f1602a0a30e149b419178b30530c9e6019f052f))
* **xml:** layouts with runaway nesting or numbers and huge Markdown no longer crash or stall the app ([#50](https://github.com/JigSawFr/Aurore/issues/50)) ([5774573](https://github.com/JigSawFr/Aurore/commit/577457359e7a34b3e72cbc792d31830291c3b401))


### Code Refactoring

* **api:** Cadran is Aurore's only look, setCadran() is gone ([#86](https://github.com/JigSawFr/Aurore/issues/86)) ([c820e70](https://github.com/JigSawFr/Aurore/commit/c820e70d396f8a10808354d575f6f5521930e1a9))
* **api:** drop borealis' names and everything deprecated ([#63](https://github.com/JigSawFr/Aurore/issues/63)) ([4442ba1](https://github.com/JigSawFr/Aurore/commit/4442ba1b6c6e4f303328a16a1a3915c5c533d97c))
* **core:** constexpr string tables ([#136](https://github.com/JigSawFr/Aurore/issues/136)) ([8af8b62](https://github.com/JigSawFr/Aurore/commit/8af8b62a1d72583ef76b65fb37eb30b545ee7d89))
* **core:** contains, starts_with and ends_with in place of find and count ([#129](https://github.com/JigSawFr/Aurore/issues/129)) ([155a856](https://github.com/JigSawFr/Aurore/commit/155a8566b93c99082bdb8a7f7ddc9de2f88b00ee))
* **core:** nodiscard on the calls that say they failed ([#137](https://github.com/JigSawFr/Aurore/issues/137)) ([225d252](https://github.com/JigSawFr/Aurore/commit/225d2528505884f822041d23147359e0a66660af))
* **core:** std::format in place of fmt, which is no longer bundled ([#128](https://github.com/JigSawFr/Aurore/issues/128)) ([378182a](https://github.com/JigSawFr/Aurore/commit/378182a334b46146e60db9ccc34d39ff4416a145))
* **core:** std::ranges::contains in place of std::find against end() ([#132](https://github.com/JigSawFr/Aurore/issues/132)) ([3c76dda](https://github.com/JigSawFr/Aurore/commit/3c76dda291942d49daf541cecd4933bdd2982dc9))
* **core:** std::to_underlying in place of casts from an enum ([#134](https://github.com/JigSawFr/Aurore/issues/134)) ([0e3bdfc](https://github.com/JigSawFr/Aurore/commit/0e3bdfc391a1023eda026aef0e34f53cc0d633d5))
* **names:** Aurore in the comments and messages of the code ([4d61fc3](https://github.com/JigSawFr/Aurore/commit/4d61fc35f26bb78d1727b1fe4e4f74b4bee9d33b))
* **names:** AURORE_ replaces the BRLS_ and BOREALIS_ prefixes ([f608bc2](https://github.com/JigSawFr/Aurore/commit/f608bc2f86a5c33d00f582133b2222722c0bf461))
* **names:** aurore: XML tags and aurore/ theme and style keys ([24a62e9](https://github.com/JigSawFr/Aurore/commit/24a62e95efc04b6a478500acd83d204adb016062))
* **names:** Aurore's own names in the API, with borealis' kept as deprecated aliases until 1.0 ([#56](https://github.com/JigSawFr/Aurore/issues/56)) ([dc3efeb](https://github.com/JigSawFr/Aurore/commit/dc3efeb0c449abc207f100ecb18b2673c264d26e))
* **names:** the aurore namespace replaces brls ([035c080](https://github.com/JigSawFr/Aurore/commit/035c080d24701bafa411e255dcb640fc27bffe75))
* **names:** the headers move to include/aurore ([7e5bdd8](https://github.com/JigSawFr/Aurore/commit/7e5bdd85cf364738fdf055395711d120d94254b4))
* **site:** build on boreale-site, the sites' shared base ([#141](https://github.com/JigSawFr/Aurore/issues/141)) ([b0f34c9](https://github.com/JigSawFr/Aurore/commit/b0f34c90bd5f75dcb6ff1c708a898c0cefc6336a))
* **site:** move to boreale-site v3, drop Aurore's own copies ([#142](https://github.com/JigSawFr/Aurore/issues/142)) ([62b3c78](https://github.com/JigSawFr/Aurore/commit/62b3c78c32664ce1243f8177e67e227f7d9be1a9))
* **site:** the header and page chrome come from boreale-site ([#146](https://github.com/JigSawFr/Aurore/issues/146)) ([a219c78](https://github.com/JigSawFr/Aurore/commit/a219c78a2db27c34953a361b3b1d964decc5abe2))


### Documentation

* **android:** what the Bluetooth permissions are for ([#100](https://github.com/JigSawFr/Aurore/issues/100)) ([addc2fe](https://github.com/JigSawFr/Aurore/commit/addc2fe783293340051349dfd69132f494f6a5cb))
* **contributing:** how to build, test and send a change, and how to report a vulnerability ([#39](https://github.com/JigSawFr/Aurore/issues/39)) ([1f867ef](https://github.com/JigSawFr/Aurore/commit/1f867efc84965ccf496fc6cbb698296f58dbc8c2))
* **environment:** drop what the removed names and the Cadran switch left behind ([#94](https://github.com/JigSawFr/Aurore/issues/94)) ([6eb3cad](https://github.com/JigSawFr/Aurore/commit/6eb3cad4891a2a84e303fa1c4ac4ba7a7f5e47cf))
* **guides:** guides for migrating, platforms, porting and architecture, and the API reference for Cadran ([#52](https://github.com/JigSawFr/Aurore/issues/52)) ([e1ff480](https://github.com/JigSawFr/Aurore/commit/e1ff48038c808691f90508de923e303d198fb890))
* **names:** Aurore's names, and borealis' kept until 1.0 ([926d429](https://github.com/JigSawFr/Aurore/commit/926d4297d7e960f864a414c0fedd42a68a8a9043))
* **platforms:** the New 3DS family on every page and picture ([#118](https://github.com/JigSawFr/Aurore/issues/118)) ([702b8f0](https://github.com/JigSawFr/Aurore/commit/702b8f0253f1d96ce29e2f94a8285c52684efac2))
* **readme:** the screenshots' captions after the showcase, and the density and small screens among what Aurore adds ([#82](https://github.com/JigSawFr/Aurore/issues/82)) ([8396ba5](https://github.com/JigSawFr/Aurore/commit/8396ba56456de2ebdb3b8600f1a7eb96d12b4a1e))
* **site:** the safe areas page on the site, and the docs caught up with Cadran alone, SDL3 and the New 3DS ([#102](https://github.com/JigSawFr/Aurore/issues/102)) ([dff3c1b](https://github.com/JigSawFr/Aurore/commit/dff3c1baa7d8bb69f16b2da3edef7aa6908090bd))


### Build System

* **3ds:** the New 3DS platform, a smoke test and its CI job ([#84](https://github.com/JigSawFr/Aurore/issues/84)) ([997e089](https://github.com/JigSawFr/Aurore/commit/997e089c10e97ee9be371baad2270731a4b1314d))
* **android:** Android Gradle Plugin 9.5, Gradle 9.8, NDK r30 and API 36 ([#71](https://github.com/JigSawFr/Aurore/issues/71)) ([0f265d7](https://github.com/JigSawFr/Aurore/commit/0f265d725010801cb647f9373a29775bd4b66d3a))
* **apple:** iOS 27 and macOS 27 on Apple silicon, built by Xcode 27 ([#120](https://github.com/JigSawFr/Aurore/issues/120)) ([d2ed31c](https://github.com/JigSawFr/Aurore/commit/d2ed31c9fef81546cc9e6354302d080eb02ced46))
* **cmake:** C++23 for the library, C++20 still the minimum for the projects that link it ([#109](https://github.com/JigSawFr/Aurore/issues/109)) ([d679294](https://github.com/JigSawFr/Aurore/commit/d679294a9d9de7e41d1492bc1517047696b44d4d))
* **cmake:** C++23 for the projects that link Aurore, and CMake 3.20 ([#123](https://github.com/JigSawFr/Aurore/issues/123)) ([3490eea](https://github.com/JigSawFr/Aurore/commit/3490eea9b1475b64bc9a7de1aa38031b7459418f))
* **cmake:** prefixed options, a relocatable install, warnings everywhere, and CI for every build variant ([#55](https://github.com/JigSawFr/Aurore/issues/55)) ([979851e](https://github.com/JigSawFr/Aurore/commit/979851e3131b1d2132c721dfac72b1ccd8a99ad7))
* **deps:** GLFW 3.5.1, xfangfang's fixes carried over ([#91](https://github.com/JigSawFr/Aurore/issues/91)) ([b30e095](https://github.com/JigSawFr/Aurore/commit/b30e09598ce0fdb85d9d11db9b1bb0d51d83e2ea))
* **deps:** Tweeny 4.2.0, for its easings only ([#89](https://github.com/JigSawFr/Aurore/issues/89)) ([2e7cc32](https://github.com/JigSawFr/Aurore/commit/2e7cc3237ce706897e053ab9f058e839b7accf1a))
* **emoji:** Twemoji 17.0.3, fetched by tools/make_emoji.py ([#68](https://github.com/JigSawFr/Aurore/issues/68)) ([b8cf804](https://github.com/JigSawFr/Aurore/commit/b8cf804e05fcd70c6786025ac7a39406fe665e02))
* **gl:** glad 2.0.8, and a context the driver cannot use is said, not crashed on ([#66](https://github.com/JigSawFr/Aurore/issues/66)) ([3f5f365](https://github.com/JigSawFr/Aurore/commit/3f5f36515629036e925c721722c525145d7dd17d))
* **names:** borealis' BRLS_ options and variables and brls_add_tests() still work ([8ae2992](https://github.com/JigSawFr/Aurore/commit/8ae2992a5c9751aab6cb074ba066b6570b1d8693))
* **ps4:** LLVM 23's clang and libc++ in place of PacBrew's LLVM 12, for C++23 ([#117](https://github.com/JigSawFr/Aurore/issues/117)) ([251fb0d](https://github.com/JigSawFr/Aurore/commit/251fb0d2722230a9bee32b7ffb8a74185580c41c))
* **psv:** the Vita's OpenGL ES 2 image built from its recipe, with GCC 15.2 and SDL 2.32 ([#112](https://github.com/JigSawFr/Aurore/issues/112)) ([7692dca](https://github.com/JigSawFr/Aurore/commit/7692dca806349f4c1b1812f3caa8e4af55c50a2b))
* **release:** version Aurore from 0.1.0, with release-please and a changelog ([#40](https://github.com/JigSawFr/Aurore/issues/40)) ([c0e6aae](https://github.com/JigSawFr/Aurore/commit/c0e6aae15c5285cc1f6d0990fe8b4563a9019222))
* **sdl:** SDL2 2.32.10, and controller presses shorter than a frame are no longer lost ([#57](https://github.com/JigSawFr/Aurore/issues/57)) ([57fc880](https://github.com/JigSawFr/Aurore/commit/57fc880f14bd5d60434be9053f210f60d1a1be08))
* **web:** Emscripten 6.0.13 ([#88](https://github.com/JigSawFr/Aurore/issues/88)) ([e3cc8b1](https://github.com/JigSawFr/Aurore/commit/e3cc8b195527628f0b12359f7b3cac6f973527cf))


### Tests

* **cadran:** kit host tests live in Aurore, and screens of their own are set by a hook ([#51](https://github.com/JigSawFr/Aurore/issues/51)) ([84e66da](https://github.com/JigSawFr/Aurore/commit/84e66da39f9f621a342fd9724785748c7e811e1d))
* **names:** every name borealis gave the library still works ([4eea6b1](https://github.com/JigSawFr/Aurore/commit/4eea6b113e62f2a37d792ea70b1540204c95dfda))

## Changelog

Aurore's versions and what each changes, written by release-please from the commit messages
(Conventional Commits). Before 1.0, a minor version may break the API: read its entry before updating the submodule.
