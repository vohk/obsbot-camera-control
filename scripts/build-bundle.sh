#!/usr/bin/env bash
# Run inside the build Distrobox. The resulting directory runs on the host.
set -euo pipefail
command -v patchelf >/dev/null || { echo "Install patchelf in the build container." >&2; exit 1; }
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build="${OBSBOT_BUILD_DIR:-$root/build-bundle}"
output="$build/obsbot-control"
cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$output" -DCMAKE_INSTALL_LIBDIR=lib \
    -DOBSBOT_BUNDLE_RUNTIME=ON -DQT_DEPLOY_USE_PATCHELF=ON
cmake --build "$build" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-4}"
# Regenerate only the bundle output, avoiding stale libraries after rebuilds.
rm -rf -- "$output"
cmake --install "$build"
patchelf --remove-rpath "$output/lib/libdev.so.1.0.2"
cat > "$output/obsbot-control" <<'LAUNCHER'
#!/usr/bin/env bash
set -euo pipefail
bundle=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# Keep dependency and Qt plugin resolution inside this bundle.
unset QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH
export LD_LIBRARY_PATH="$bundle/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$bundle/bin/obsbot-gui" "$@"
LAUNCHER
chmod +x "$output/obsbot-control"
tar -C "$build" -czf "$build/obsbot-control-linux-$(uname -m).tar.gz" obsbot-control
printf 'Bundle: %s\nRun on the host: %s/obsbot-control\n' "$output" "$output"
