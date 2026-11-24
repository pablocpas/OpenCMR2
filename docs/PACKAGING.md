# Linux Packaging (AppImage & Flatpak)

OpenCMR2 shares one set of FreeDesktop metadata files across all Linux packaging
formats (`cmake --install`, distribution packages, AppImage, and Flatpak):

* `packaging/linux/io.github.pablocpas.OpenCMR2.desktop`
* `packaging/linux/io.github.pablocpas.OpenCMR2.metainfo.xml`
* `res/opencmr2_256.png` (installed as `io.github.pablocpas.OpenCMR2.png`)

## Validating desktop metadata

```sh
desktop-file-validate packaging/linux/io.github.pablocpas.OpenCMR2.desktop
appstreamcli validate --no-net packaging/linux/io.github.pablocpas.OpenCMR2.metainfo.xml
```

## Standard install (`cmake --install`)

```sh
cmake --preset linux -DOPENCMR2_BUILD_TESTS=OFF
cmake --build --preset linux
DESTDIR="$PWD/staging" cmake --install build/linux --prefix /usr
```

When `-DOPENCMR2_VENDORED_SDL=ON` is used, `cmake --install` also installs
`libSDL3.so.0` into `${CMAKE_INSTALL_LIBDIR}` and sets `INSTALL_RPATH` to
`$ORIGIN/../${CMAKE_INSTALL_LIBDIR}`.

## AppImage

GitHub Actions builds portable `x86_64` and `arm64` AppImages inside a
`debian:bookworm` container (`glibc 2.36`) so they run across older and newer
Linux distributions.

To build one locally:

```sh
# 1. Build with vendored SDL3 so libSDL3.so.0 is bundled inside the AppImage
cmake --preset linux -DOPENCMR2_VENDORED_SDL=ON -DOPENCMR2_BUILD_TESTS=OFF
cmake --build --preset linux

# 2. Download appimagetool (once)
ARCH="$(uname -m)"
curl -fL "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-${ARCH}.AppImage" \
  -o /tmp/appimagetool
chmod +x /tmp/appimagetool

# 3. Package
packaging/linux/build_appimage.sh \
  --build-dir build/linux \
  --appimagetool /tmp/appimagetool \
  --output "dist/OpenCMR2-Linux-${ARCH}.AppImage"
```

Note: an AppImage built on a rolling or bleeding-edge host (for example Fedora
or Arch) will require that host's `glibc` version. For distribution to other
machines, build inside a `debian:bookworm` container (matching CI).

## Flatpak (Flathub)

The manifest lives at `packaging/flatpak/io.github.pablocpas.OpenCMR2.yml` and
uses `org.freedesktop.Platform//26.08` with `org.freedesktop.Sdk.Extension.llvm23`.

### Local build and test

Install `flatpak-builder` and the SDK once:

```sh
# Fedora: sudo dnf install flatpak-builder
# Debian/Ubuntu: sudo apt install flatpak-builder
flatpak install --user flathub \
  org.freedesktop.Platform//26.08 \
  org.freedesktop.Sdk//26.08 \
  org.freedesktop.Sdk.Extension.llvm23//26.08
```

Build, install for the current user, and run:

```sh
flatpak-builder --user --install --force-clean build-flatpak \
  packaging/flatpak/io.github.pablocpas.OpenCMR2.yml
flatpak run io.github.pablocpas.OpenCMR2
```

Verify inside the sandbox:
1. First launch folder picker selects the original CMR2 installation folder and
   remembers it after restarting `flatpak run io.github.pablocpas.OpenCMR2`.
2. Saves and settings are written under
   `~/.var/app/io.github.pablocpas.OpenCMR2/data/OpenCMR2/`.
3. No extra "Add to applications menu" prompt appears (disabled automatically
   when running inside Flatpak).
4. Vulkan/OpenGL rendering, audio, gamepads/wheels (and force feedback) work.

### Submitting to Flathub

Flathub builds from a dedicated repository under `flathub/io.github.pablocpas.OpenCMR2`,
created automatically once your initial PR against `flathub/flathub` (`new-pr` branch)
is merged. A ready-to-submit manifest is prepared at:

* `packaging/flatpak/flathub/io.github.pablocpas.OpenCMR2.yml`

Step-by-step checklist:

1. **Add screenshots (required by Flathub)**
   Take 1–2 PNG screenshots (16:9, e.g. 1280×720 or 1920×1080) of OpenCMR2 running
   and save them in `docs/screenshots/` (for example `stage.png` and `menu.png`).
   Uncomment the `<screenshots>` block in
   `packaging/linux/io.github.pablocpas.OpenCMR2.metainfo.xml` pointing to the
   tagged raw GitHub URLs, and add a `<release>` entry for the new version.

2. **Push and tag a new release in `pablocpas/OpenCMR2`**
   Flathub builds from an immutable Git tag that already contains
   `packaging/linux/` and the CMake `install()` rules:
   ```sh
   git tag -a v1.9.2 -m "OpenCMR2 1.9.2"
   git push origin main v1.9.2
   git rev-parse v1.9.2^{commit}
   ```
   Put that exact tag and commit SHA into
   `packaging/flatpak/flathub/io.github.pablocpas.OpenCMR2.yml`
   (the `glslang`, `spirv-cross`, and `sdl3` commits are already pinned).

3. **Run Flathub's official linter (`flatpak-builder-lint`)**
   Install `org.flatpak.Builder` once and lint both the manifest and a built repo:
   ```sh
   flatpak install --user flathub org.flatpak.Builder

   flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest \
     packaging/flatpak/flathub/io.github.pablocpas.OpenCMR2.yml

   flatpak run org.flatpak.Builder --force-clean --sandbox --user \
     --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ \
     --repo=build-flathub-repo build-flathub \
     packaging/flatpak/flathub/io.github.pablocpas.OpenCMR2.yml

   flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo build-flathub-repo
   ```
   *(Make sure `flatpak-builder-lint` reports zero errors before opening the PR.)*

4. **Open the PR on `flathub/flathub`**
   1. Fork `https://github.com/flathub/flathub` on GitHub.
   2. Clone your fork and branch from `new-pr` (not `master`):
      ```sh
      git clone --branch=new-pr git@github.com:pablocpas/flathub.git
      cd flathub
      git checkout -b opencmr2
      cp /path/to/OpenCMR2/packaging/flatpak/flathub/io.github.pablocpas.OpenCMR2.yml .
      git add io.github.pablocpas.OpenCMR2.yml
      git commit -m "Add io.github.pablocpas.OpenCMR2"
      git push -u origin opencmr2
      ```
   3. Open a Pull Request from `pablocpas/flathub:opencmr2` into `flathub/flathub:new-pr`.
   4. Comment `bot, build io.github.pablocpas.OpenCMR2` on the PR if the build bot
      does not trigger automatically.

5. **Verify your app after approval**
   Once merged, Flathub creates `flathub/io.github.pablocpas.OpenCMR2` and invites
   you as maintainer. Log in to [flathub.org](https://flathub.org) with GitHub
   and click **Verify** (automatic because the ID matches `io.github.pablocpas.*`).
   From then on, `x-checker-data` in the manifest lets `flathubbot` open update
   PRs automatically whenever you push a new `vX.Y.Z` tag.
