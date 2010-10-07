#!/usr/bin/env awk -f
BEGIN {
    out=0
}

# Common sections -- include in output
/^### *section common/ {
    out=1
    next
}

# Build sections -- include in output
/^### *section build/ {
    out=1
    next
}

# Configure sections -- omit from output
/^### *section configure/ {
    out=0
    next
}

# % comments -- include in output
/^#%/ {
    print $0
    next
}

# Normal comments -- omit from output
/^[\t ]*#/ {
    next
}

# Blank lines -- omit from output
/^[\t ]*$/ {
    next
}

# Normal line -- include in output if in included section
{
    if (out)
    {
        print $0;
    }
}


