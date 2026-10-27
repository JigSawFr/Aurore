# Security policy

## Reporting a vulnerability

Please report it privately, not in a public issue: on GitHub, **Security › Report a vulnerability**
(<https://github.com/JigSawFr/aurore/security/advisories/new>). Say what is affected (a theme file, an image, XML, a
URL, a platform), how to reproduce it, and what it allows.

You get an answer within a week. A confirmed issue is fixed on `main`, released (`vX.Y.Z`, with a `Security` entry in
`CHANGELOG.md`) and published as a GitHub security advisory; PlayGuard and PlayShelf, which use Aurore, are updated
with it. Credit is given unless you ask otherwise.

## Supported versions

The latest release and `main`. Apps embed Aurore as a submodule: they get a fix by updating it.

## Scope

What Aurore reads from outside the app is in scope: theme files and folders (`Theme::loadFile`, `@include`, images),
pictures, XML layouts, Markdown, translations, URLs it opens, and the build (CI, downloaded tools). Bugs in the bundled
third-party code (`extern/`) are reported upstream too.
