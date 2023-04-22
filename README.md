# calory

> Desktop web apps on a diet

Calory is a proof-of-concept framework for creating cross-platform desktop web apps much
smaller than those created with Electron. It depends on a compatible Chromium-based browser
already installed on the user's computer, and does not provide a backend runtime.

Calory locates and launches the browser with an isolated profile, in chromeless `--app`
mode, with an extension embedded which bridges communications between the frontend app
and the provided backend. This backend can use Node.js, Bun, Deno, C++, etc. - whatever
you want.

**Warning: Currently Calory does not help at all with securing your app. Any included scripts will have access to your filesystem and any other resources provided by your backend process.**

## Browsers

- [x] Chromium
- [x] Chrome
- [x] Edge
- [x] Vivaldi

## Operating Systems

- [x] Linux
- [ ] Windows
- [ ] MacOS

## TODO

- [ ] `localhost:3000` is hardcoded dev URL
- [ ] `app/dist/index.html` is hardcoded prod URL
