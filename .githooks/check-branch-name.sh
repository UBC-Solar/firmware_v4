#!/bin/sh
# Shared branch-name validator, sourced by pre-commit and pre-push.
#
# Allowed formats:
#   user/<name>/<feature>
#   user/<name>/<project>/<feature>
#   EMB-<ticket#>/<board>-<feature>         (<board> is not validated against a list)

EXEMPT_BRANCHES="main"

branch_name_usage()
{
    cat >&2 <<USAGE
ERROR: branch name '$1' does not follow the firmware_v4 naming convention.

Allowed formats:
  user/<name>/<feature>                    e.g. user/gregorybian/hexfix
  user/<name>/<project>/<feature>          e.g. user/gregorybian/tel/feature_name
  EMB-<ticket#>/<board>-<feature>          e.g. EMB-24/tel-gps-implementation

Rename the current branch with:  git branch -m <new-name>
Bypass (not recommended):        --no-verify
USAGE
}

check_branch_name()
{
    name=$1

    for exempt in $EXEMPT_BRANCHES; do
        [ "$name" = "$exempt" ] && return 0
    done

    if printf '%s' "$name" | grep -Eq '^user/[^/[:space:]]+/[^/[:space:]]+(/[^/[:space:]]+)?$'; then
        return 0
    fi

    if printf '%s' "$name" | grep -Eq '^EMB-[0-9]+/[^/[:space:]-]+-[^[:space:]]+$'; then
        return 0
    fi

    branch_name_usage "$name"
    return 1
}
