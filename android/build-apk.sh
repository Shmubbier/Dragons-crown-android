#!/usr/bin/env bash
# Packages build/libmain.so (android/CMakeLists.txt) as an installable APK without Gradle:
#   SDL3's Java activity + DcpActivity -> javac -> d8; manifest -> aapt2; zipalign; apksigner (local debug key).
# Usage: android/build-apk.sh <libmain.so> <SDL3 source dir> <output.apk>
set -euo pipefail
LIBMAIN="$1"; SDL="$2"; OUT_APK="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
SDK="${ANDROID_HOME:-$LOCALAPPDATA/Android/Sdk}"
BT="$(ls -d "$SDK"/build-tools/* | sort -V | tail -1)"
PLATFORM="$(ls -d "$SDK"/platforms/android-* | sort -V | tail -1)/android.jar"
NDK_BIN="$(ls -d "$SDK"/ndk/* | sort -V | tail -1)/toolchains/llvm/prebuilt/windows-x86_64/bin"
JAVA_BIN="${JAVA_HOME:+$JAVA_HOME/bin/}"
WORK="$(dirname "$OUT_APK")/apk-work"
rm -rf "$WORK"; mkdir -p "$WORK/classes" "$WORK/dex" "$WORK/lib/arm64-v8a"

# Java: SDL's activity and ours, compiled for the platform, converted to dex.
find "$SDL/android-project/app/src/main/java" "$HERE/app/java" -name '*.java' | while read -r f; do cygpath -m "$f"; done > "$WORK/sources.txt"
"${JAVA_BIN}javac" -nowarn -encoding UTF-8 -source 11 -target 11 -classpath "$PLATFORM" -d "$WORK/classes" @"$WORK/sources.txt"
find "$WORK/classes" -name '*.class' | while read -r f; do cygpath -m "$f"; done > "$WORK/classes.txt"
"$BT/d8.bat" --release --min-api 33 --lib "$PLATFORM" --output "$WORK/dex" @"$WORK/classes.txt" 2>/dev/null || \
    "$BT/d8" --release --min-api 33 --lib "$PLATFORM" --output "$WORK/dex" @"$WORK/classes.txt"

# Manifest and resources table.
"$BT/aapt2" link -o "$WORK/base.apk" -I "$PLATFORM" --manifest "$HERE/app/AndroidManifest.xml" \
    --min-sdk-version 33 --target-sdk-version 35

# Native library without debug sections (symbols kept for crash reports).
"$NDK_BIN/llvm-strip" --strip-debug -o "$WORK/lib/arm64-v8a/libmain.so" "$LIBMAIN"
# libadrenotools' hook libraries (custom Vulkan driver loading), when the build has them.
for hook in libhook_impl.so libmain_hook.so libfile_redirect_hook.so libgsl_alloc_hook.so; do
    if [ -f "$(dirname "$LIBMAIN")/$hook" ]; then
        "$NDK_BIN/llvm-strip" --strip-debug -o "$WORK/lib/arm64-v8a/$hook" "$(dirname "$LIBMAIN")/$hook"
    fi
done

python - "$WORK" <<'EOF'
import sys, zipfile, os
work = sys.argv[1]
libs = os.path.join(work, "lib", "arm64-v8a")
with zipfile.ZipFile(os.path.join(work, "base.apk"), "a") as apk:
    apk.write(os.path.join(work, "dex", "classes.dex"), "classes.dex")
    for name in sorted(os.listdir(libs)):
        apk.write(os.path.join(libs, name), "lib/arm64-v8a/" + name, zipfile.ZIP_DEFLATED)
EOF

"$BT/zipalign" -p -f 4 "$WORK/base.apk" "$WORK/aligned.apk"

# Local debug signing key (the standard Android debug-key convention), created once next to the output.
KEY="$(dirname "$OUT_APK")/dcp-debug.keystore"
if [ ! -f "$KEY" ]; then
    "${JAVA_BIN}keytool" -genkeypair -keystore "$KEY" -storepass android -keypass android -alias androiddebugkey \
        -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US" >/dev/null 2>&1
fi
"$BT/apksigner.bat" sign --ks "$KEY" --ks-pass pass:android --key-pass pass:android --out "$OUT_APK" "$WORK/aligned.apk" 2>/dev/null || \
    "$BT/apksigner" sign --ks "$KEY" --ks-pass pass:android --key-pass pass:android --out "$OUT_APK" "$WORK/aligned.apk"
ls -la "$OUT_APK"
