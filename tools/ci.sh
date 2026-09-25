#!/usr/bin/env bash

set -euo pipefail

repositoryRoot=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

runStep()
{
    local activity=$1
    shift

    echo "ci: $activity"

    if ! "$@"
    then
        echo "ci: $activity failed"
        exit 1
    fi
}

cd "$repositoryRoot"

if ! source tools/env.sh
then
    echo "ci: the build environment is not ready"
    exit 1
fi

logicalProcessors=$(getconf _NPROCESSORS_ONLN)
export CTEST_PARALLEL_LEVEL=$((logicalProcessors > 1 ? logicalProcessors / 2 : 1))

runStep "workflow clang-debug" cmake --workflow --preset clang-debug
runStep "workflow clang-release" cmake --workflow --preset clang-release
runStep "clang-format" bash -c 'git ls-files -z "*.cpp" "*.hpp" | xargs -0 "$UNISON_CLANG_FORMAT" --dry-run --Werror'

echo "ci: ok"
