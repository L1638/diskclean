#ifndef DISKCLEAN_H
#define DISKCLEAN_H

struct dc_options
{
    bool yes;
    bool print;
    bool keep_directory;
    time_t time;
    bool quiet;
};

#define HAS_ENTRY 1
#define OLD 2

enum DC_status
{
    DC_OK = 2,
    DC_USER_DECLINED,
    DC_ERROR,
    DC_NONEMPTY_DIR
};

#define VALID_STATUS(S) \
    ((S) == DC_OK || (S) == DC_USER_DECLINED || (S) == DC_ERROR)

#define UPDATE_STATUS(S, New_value)                               \
    do                                                            \
    {                                                             \
        if ((New_value) == DC_ERROR                               \
            || ((New_value) == DC_USER_DECLINED && (S) == DC_OK)) \
            (S) = (New_value);                                    \
    }                                                             \
    while(0)

enum DC_status
dc(char *const *file, struct dc_options const *opt);

#endif