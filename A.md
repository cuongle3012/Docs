# Load keys from file1
NR == FNR {
    keys[++n] = $0
    next
}

{
    delete_line = 0
    # for each key, check literal substring
    for (i = 1; i <= n; i++) {
        if (index($0, keys[i]) > 0) {
            delete_line = 1
            break
        }
    }
    if (!delete_line) print
}
