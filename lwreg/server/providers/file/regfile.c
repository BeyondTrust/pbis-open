

DWORD
RegFileOpen(
    int fildes,
    off_t offset,
    int whence
    )
{
    DWORD dwError = 0;

    open(fildes, offset, whence);

    if (errno) {
        convert errno to lwerr
    return dwError;

}

DWORD
RegFileWrite(
    int fildes,
    const void *buffer,
    size
    )
{

}

DWORD
RegFileRead(
    int fildes,
    void *buf,
    size_t nbytes,
    size_t *pbytesread
    )
{


]

DWORD
RegFileSeek(
    int fildes,
    off_t offset,
    int whence
    off_t *pnewoffset
    )
{
    DWORD dwError = 0;

    return dwError;

}

}
