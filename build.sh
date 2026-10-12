#!/bin/bash

# Default values
CMAKE_ARGS=""

CLEAN=false
DEPLOY_MACOS=false
INSTALL_MACOS=false
INSTALL_MACOS_PATH="/Applications/qView.app"
MACOS_CODESIGN_IDENTITY="${CODESIGN_CERT_NAME:--}"

copy_macos_dependency() {
    local dep_name="$1"
    local frameworks_dir="$2"
    local search_dir

    if [[ -e "$frameworks_dir/$dep_name" ]]; then
        return 0
    fi

    for search_dir in \
        "$(qmake -query QT_INSTALL_LIBS 2>/dev/null)" \
        "/opt/homebrew/lib" \
        "/usr/local/lib" \
        "/opt/homebrew/opt/brotli/lib" \
        "/usr/local/opt/brotli/lib"; do
        if [[ -n "$search_dir" && -e "$search_dir/$dep_name" ]]; then
            echo "Copying missing dependency $dep_name"
            ditto "$search_dir/$dep_name" "$frameworks_dir/$dep_name"
            install_name_tool -id "@rpath/$dep_name" "$frameworks_dir/$dep_name"
            return 0
        fi
    done

    echo "Could not find missing dependency $dep_name" >&2
    return 1
}

find_macos_binaries() {
    local frameworks_dir="$1"
    local plugins_dir="$2"
    local candidate

    find "$frameworks_dir" -maxdepth 1 -type f -name '*.dylib' 2>/dev/null
    find "$plugins_dir" -type f -name '*.dylib' 2>/dev/null
    while IFS= read -r candidate; do
        if [[ "$(dirname "$candidate")" == */Versions/A ]]; then
            echo "$candidate"
        fi
    done < <(find "$frameworks_dir" -type f 2>/dev/null)
}

repair_macos_bundle_dependencies() {
    local app_bundle="$1"
    local executable="$app_bundle/Contents/MacOS/qView"
    local frameworks_dir="$app_bundle/Contents/Frameworks"
    local plugin
    local binary
    local dep
    local dep_name
    local qt_lib_dir

    qt_lib_dir="$(qmake -query QT_INSTALL_LIBS 2>/dev/null)"

    if otool -l "$executable" | grep -q "path /opt/homebrew/lib "; then
        install_name_tool -rpath /opt/homebrew/lib @executable_path/../Frameworks "$executable"
    elif otool -l "$executable" | grep -q "path /usr/local/lib "; then
        install_name_tool -rpath /usr/local/lib @executable_path/../Frameworks "$executable"
    elif ! otool -l "$executable" | grep -q "path @executable_path/../Frameworks "; then
        install_name_tool -add_rpath @executable_path/../Frameworks "$executable"
    fi

    while IFS= read -r plugin; do
        if ! otool -l "$plugin" | grep -q "path @loader_path/../../Frameworks "; then
            install_name_tool -add_rpath @loader_path/../../Frameworks "$plugin"
        fi
    done < <(find "$app_bundle/Contents/PlugIns" -type f -name '*.dylib' 2>/dev/null)

    for dep_name in QtCore.framework QtGui.framework QtNetwork.framework QtWidgets.framework QtSvg.framework QtDBus.framework; do
        if [[ ! -e "$frameworks_dir/$dep_name" && -n "$qt_lib_dir" && -e "$qt_lib_dir/$dep_name" ]]; then
            echo "Copying Qt framework $dep_name"
            ditto "$qt_lib_dir/$dep_name" "$frameworks_dir/$dep_name"
            install_name_tool -id "@rpath/$dep_name/Versions/A/${dep_name%.framework}" \
                "$frameworks_dir/$dep_name/Versions/A/${dep_name%.framework}"
        fi
    done

    while IFS= read -r dep_name; do
        if [[ ! -e "$frameworks_dir/$dep_name" && -n "$qt_lib_dir" && -e "$qt_lib_dir/$dep_name" ]]; then
            echo "Copying missing Qt framework $dep_name"
            ditto "$qt_lib_dir/$dep_name" "$frameworks_dir/$dep_name"
            install_name_tool -id "@rpath/$dep_name/Versions/A/${dep_name%.framework}" \
                "$frameworks_dir/$dep_name/Versions/A/${dep_name%.framework}"
        fi
    done < <(find_macos_binaries "$frameworks_dir" "$app_bundle/Contents/PlugIns" \
        | while IFS= read -r binary; do otool -L "$binary"; done 2>/dev/null \
        | awk '/@rpath\/Qt.*\.framework/ { dep = $1; sub("@rpath/", "", dep); sub("/Versions/.*", "", dep); print dep }' \
        | sort -u)

    while IFS= read -r dep_name; do
        copy_macos_dependency "$dep_name" "$frameworks_dir"
    done < <(find_macos_binaries "$frameworks_dir" "$app_bundle/Contents/PlugIns" \
        | while IFS= read -r binary; do otool -L "$binary"; done 2>/dev/null \
        | awk '/@rpath\/lib.*\.dylib/ { dep = $1; sub("@rpath/", "", dep); print dep }' \
        | sort -u)

    while IFS= read -r binary; do
        while IFS= read -r dep; do
            dep_name="${dep#@rpath/}"
            case "$dep_name" in
                Qt*.framework/*)
                    dep_name="${dep_name%%/*}.framework"
                    if [[ ! -e "$frameworks_dir/$dep_name" && -n "$qt_lib_dir" && -e "$qt_lib_dir/$dep_name" ]]; then
                        echo "Copying missing Qt framework $dep_name"
                        ditto "$qt_lib_dir/$dep_name" "$frameworks_dir/$dep_name"
                        install_name_tool -id "@rpath/$dep_name/Versions/A/${dep_name%.framework}" \
                            "$frameworks_dir/$dep_name/Versions/A/${dep_name%.framework}"
                    fi
                    ;;
                *.dylib)
                    copy_macos_dependency "$dep_name" "$frameworks_dir"
                    ;;
            esac
        done < <(otool -L "$binary" | awk '/@rpath\// { print $1 }')
    done < <(find_macos_binaries "$frameworks_dir" "$app_bundle/Contents/PlugIns")
}

# Find a valid macOS SDK and set it for CMake to fix a potential mismatch
if [[ "$(uname)" == "Darwin" ]]; then
    SDK_PATH=$(xcrun --sdk macosx --show-sdk-path 2>/dev/null)
    if [ -n "$SDK_PATH" ]; then
        CMAKE_ARGS="-DCMAKE_OSX_SYSROOT=$SDK_PATH"
    fi
fi

# Parse command-line arguments
for arg in "$@"
do
    case $arg in
        --format)
        clang-format -i **/*.cpp **/*.h **/*.mm
        exit 0
        ;;
        --format-check)
        clang-format -i **/*.cpp **/*.h **/*.mm --dry-run -Werror
        exit 0
        ;;
        --tidy)
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_CLANG_TIDY=clang-tidy"
        shift # Remove --tidy from processing
        ;;
        --tidy-fix)
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_CXX_CLANG_TIDY='clang-tidy;-fix-errors'"
        shift # Remove --tidy-fix from processing
        ;;
        --clean)
        CLEAN=true
        shift
        ;;
        --deploy-macos)
        DEPLOY_MACOS=true
        shift
        ;;
        --install-macos)
        DEPLOY_MACOS=true
        INSTALL_MACOS=true
        shift
        ;;
        --install-macos=*)
        DEPLOY_MACOS=true
        INSTALL_MACOS=true
        INSTALL_MACOS_PATH="${arg#*=}"
        shift
        ;;
        --macos-codesign=*)
        MACOS_CODESIGN_IDENTITY="${arg#*=}"
        shift
        ;;
        *)
        CMAKE_ARGS="$CMAKE_ARGS $arg"
        ;;
    esac
done

# Clean build directory for a fresh configuration
if $CLEAN && [ -d "build" ]; then
    echo "Removing existing build directory."
    rm -rf build
fi

echo "Configuring with: cmake -B build -G Ninja $CMAKE_ARGS"

# Run CMake configuration.
cmake -B build $CMAKE_ARGS

# Run the build
echo "Building project..."
cmake --build build --parallel

if [[ "$(uname)" == "Darwin" && "$DEPLOY_MACOS" == true ]]; then
    APP_BUNDLE="build/qView.app"
    if [ ! -d "$APP_BUNDLE" ]; then
        echo "Could not find $APP_BUNDLE" >&2
        exit 1
    fi

    if ! command -v macdeployqt >/dev/null 2>&1; then
        echo "macdeployqt is required to deploy the macOS app bundle." >&2
        exit 1
    fi

    echo "Deploying macOS app bundle..."
    rm -rf "$APP_BUNDLE/Contents/Frameworks" "$APP_BUNDLE/Contents/PlugIns" \
        "$APP_BUNDLE/Contents/Resources/qt.conf"
    macdeployqt "$APP_BUNDLE" -always-overwrite
    rm -rf "$APP_BUNDLE/Contents/PlugIns/platforminputcontexts"
    repair_macos_bundle_dependencies "$APP_BUNDLE"

    IMF_DIR="$APP_BUNDLE/Contents/PlugIns/imageformats"
    if [[ (-f "$IMF_DIR/kimg_heif.dylib" || -f "$IMF_DIR/kimg_heif.so") && -f "$IMF_DIR/libqmacheif.dylib" ]]; then
        # Prefer kimageformats HEIF plugin for proper color space handling.
        rm "$IMF_DIR/libqmacheif.dylib"
    fi
    if [[ (-f "$IMF_DIR/kimg_tga.dylib" || -f "$IMF_DIR/kimg_tga.so") && -f "$IMF_DIR/libqtga.dylib" ]]; then
        # Prefer kimageformats TGA plugin which supports more formats.
        rm "$IMF_DIR/libqtga.dylib"
    fi

    if command -v codesign >/dev/null 2>&1; then
        echo "Signing macOS app bundle with identity '$MACOS_CODESIGN_IDENTITY'..."
        codesign --force --deep --sign "$MACOS_CODESIGN_IDENTITY" "$APP_BUNDLE"
    fi

    if [[ "$INSTALL_MACOS" == true ]]; then
        echo "Installing macOS app bundle to $INSTALL_MACOS_PATH..."
        rm -rf "$INSTALL_MACOS_PATH"
        ditto "$APP_BUNDLE" "$INSTALL_MACOS_PATH"
    fi
fi
