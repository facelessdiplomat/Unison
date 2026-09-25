requireUnisonTools()
{
    local tool

    for tool in c++ cmake ninja brew
    do
        if ! command -v "$tool" >/dev/null 2>&1
        then
            echo "unison: $tool was not found on PATH" >&2
            return 1
        fi
    done
}

exportUnisonClangFormat()
{
    local clangFormatMajor=20
    local clangFormat

    clangFormat="$(brew --prefix)/opt/llvm@$clangFormatMajor/bin/clang-format"

    if [ ! -x "$clangFormat" ]
    then
        echo "unison: $clangFormat was not found; install it with: brew install llvm@$clangFormatMajor" >&2
        return 1
    fi

    export UNISON_CLANG_FORMAT="$clangFormat"
}

if requireUnisonTools && exportUnisonClangFormat
then
    unset -f requireUnisonTools exportUnisonClangFormat
    echo "Unison build environment: $(c++ --version | head -n 1), $("$UNISON_CLANG_FORMAT" --version)"
else
    unset -f requireUnisonTools exportUnisonClangFormat
    return 1
fi
