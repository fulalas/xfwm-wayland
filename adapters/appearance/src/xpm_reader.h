#ifndef APPEARANCE_XPM_READER_H
#define APPEARANCE_XPM_READER_H

typedef struct xpm_reader xpm_reader;

xpm_reader *appearance_xpm_reader_create(const char *path);
void appearance_xpm_reader_destroy(xpm_reader *reader);
const char *appearance_xpm_reader_header(xpm_reader *reader);
const char *appearance_xpm_reader_colormap(xpm_reader *reader);
const char *appearance_xpm_reader_row(xpm_reader *reader);

#endif
