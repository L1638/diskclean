#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fts.h>
#include <assert.h>
#include "diskclean.h"

/*
#define FTW_F    1
#define FTW_D    2

static char *fullpath;
static size_t pathlen;

static int diskclean(const char *path, const struct dc_options *opt)
{
    pathlen = PATH_MAX + 1;
    fullpath = (char *)malloc(pathlen);
    if (pathlen <= strlen(path)) {
        pathlen = strlen(path) * 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL) {
            perror("realloc error");
            return errno;
        }
    }
    strcpy(fullpath, path);
    return dopath(opt);
}

int dopath(struct dc_options const *opt)
{
    struct stat statbuf;
    struct dirent *dirp;
    struct timespec prev_atime, prev_mtime;
    DIR *dp;
    int ret, n;

    if (lstat(fullpath, &statbuf) < 0) {
        perror("stat error");
        return errno;
    }
    if (S_ISDIR(statbuf.st_mode) == 0) {
        // Not a directory, try to delete it
        return dc(fullpath, opt, FTW_F);
    }
    prev_atime = statbuf.st_atim;
    prev_mtime = statbuf.st_mtim;

    n = strlen(fullpath);
    if (n + NAME_MAX + 2 > pathlen) {
        pathlen *= 2;
        if ((fullpath = realloc(fullpath, pathlen)) == NULL) {
            perror("realloc error");
            return errno;
        }
    }
    fullpath[n++] = '/';
    fullpath[n] = 0;

    if ((dp = opendir(fullpath)) == NULL) {
        // can't read directory
        return 0;
    }
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 ||
            strcmp(dirp->d_name, "..") == 0)
                continue;
        strcpy(&fullpath[n], dirp->d_name);
        if ((ret = dopath(opt)) != 0)
            break;
    }
    fullpath[n-1] = 0;
    if (closedir(dp) < 0) {
        perror("can't close directory");
        return errno;
    }
    ret = dc(fullpath, opt, FTW_D);
}
*/

bool
yesno (void)
{
    bool yes;
    int c = getchar();
    yes = (c == 'y' || c == 'Y');
    while (c != '\n' && c != EOF)
        c = getchar();
    return yes;
}

/* A wrapper for readdir so that callers don't see entries for '.' or '..'. */
static inline struct dirent const *
readdir_ignoring_dot_and_dotdot (DIR *dirp)
{
    while (true)
    {
        struct dirent const *dp = readdir (dirp);
        if (dp == NULL || !(strcmp(dp->d_name, ".") == 0
                            || strcmp(dp->d_name, "..") == 0))
            return dp;
    }
}

/* Return true if DIR is determined to be an empty directory.
   Return false with ERRNO==0 if DIR is a non empty directory.
   Return false if not able to determine if directory empty. */
static inline bool
is_empty_dir (char const *dir)
{
    DIR *dirp;
    struct dirent const *dp;
    int saved_errno;
    int fd = open (dir, (O_RDONLY | O_DIRECTORY
                        | O_NOCTTY | O_NOFOLLOW | O_NONBLOCK));
    if (fd < 0)
        return false;
    dirp = fdopendir (fd);
    if (dirp == NULL)
    {
        close (fd);
        return false;
    }
    errno = 0;
    dp = readdir_ignoring_dot_and_dotdot (dirp);
    saved_errno = errno;
    closedir (dirp);
    errno = saved_errno;
    if (dp != NULL)
        return false;
    return saved_errno == 0 ? true : false;
}

/* Tell fts not to traverse into the hierarchy at ENT. */
static void
fts_skip_tree (FTS *fts, FTSENT *ent)
{
    fts_set(fts, ent, FTS_SKIP);
    /* Ensure that we do not process ENT a second time. */
    fts_read(fts);
}

/* Upon unlink failure, or when the user declines to remove ENT, mark
   each of its ancestor directories, so that we know not to prompt for
   its removal. */
static void
mark_ancestor_dirs (FTSENT *ent)
{
    FTSENT *p;
    for (p = ent->fts_parent; FTS_ROOTLEVEL <= p->fts_level; p = p->fts_parent)
    {
        if (p->fts_number)
            break;
        p->fts_number |= HAS_ENTRY;
    }
}

/* Prompt whether to remove FILENAME.
   If the file may be removed, return DC_OK.
   If the user declines to remove the file, return DC_USER_DECLINED.
   If we cannot lstat FILENAME, then return DC_ERROR.
   IS_DIR is true if ENT designates a directory, false otherwise.*/
static enum DC_status
prompt (FTS const *fts, FTSENT const *ent, bool is_dir,
        struct dc_options const *opt)
{
    char const *path = ent->fts_path;
    if (is_dir && opt->keep_directory)
        return DC_USER_DECLINED;

    if (is_dir && !is_empty_dir(path))
        return DC_USER_DECLINED;

    if (ent->fts_number & HAS_ENTRY)
        return DC_USER_DECLINED;

    if (opt->yes) {
        printf("\n");
        return DC_OK;
    }

    printf(" (y/n)? ");
    return yesno() ? DC_OK : DC_USER_DECLINED;
}

/* Remove the file system object specified by ENT. IS_DIR specifies
   whether it is expected to be a directory or non-directory.
   Return DC_OK upon success, else DC_ERROR. */
static enum DC_status
excise (FTS *fts, FTSENT *ent, struct dc_options const *opt, bool is_dir)
{
    if (is_dir) {
        if (rmdir (ent->fts_path) == 0)
            return DC_OK;
    }
    else if (unlink (ent->fts_path) == 0)
        return DC_OK;
    mark_ancestor_dirs (ent);
    return DC_ERROR;
}

static enum DC_status
dc_fts(FTS *fts, FTSENT *ent, const struct dc_options *opt)
{
    /* statable */
    if (ent->fts_info != FTS_NS && ent->fts_info != FTS_NSOK) {
        /* File is not old enough, just skip */
        if (ent->fts_statp->st_atim.tv_sec > opt->time
            || ent->fts_statp->st_mtim.tv_sec > opt->time) {
            return DC_OK;
        }
        ent->fts_number |= OLD;
    }
    switch (ent->fts_info)
    {
        case FTS_NS:         /* stat(2) failed */
        case FTS_NSOK:       /* e.g., dangling symlink */
            if (ent->fts_level == 0 && ent->fts_errno == ENOENT) {
                fprintf(stderr, "%s: No such file or directory\n", ent->fts_path);
                return DC_ERROR;
            }
            else return DC_OK;
        case FTS_D:          /* preorder directory */
        case FTS_DNR:        /* unreadable directory */
            if (opt->keep_directory) {
                mark_ancestor_dirs (ent);
                fts_skip_tree (fts, ent);
                return DC_OK;
            }
            break;
        case FTS_F:          /* regular file */
        case FTS_SL:         /* symbolic link */
        case FTS_SLNONE:     /* symbolic link without target */
        case FTS_DEFAULT:    /* none of the above */
            /* File is not old enough */
            if (!(ent->fts_number & OLD))
                return DC_OK;

            if (!opt->quiet)
                printf("%s", ent->fts_path);

            if (opt->print) {
                printf("\n");
                return DC_OK;
            }
            return prompt(fts, ent, false, opt);

        case FTS_DP:         /* postorder directory */
            /* Directory is not old enough */
            if (!(ent->fts_number & OLD))
                return DC_OK;
            
            if (!opt->quiet)
                printf("%s", ent->fts_path);
            
            if (opt->print) {
                printf("\n");
                return DC_OK;
            }
            return prompt(fts, ent, true, opt);

        case FTS_DC:         /* directory that causes cycles */
            fprintf(stderr, "cycle detected: %s\n", ent->fts_path);
            fts_skip_tree(fts, ent);
            return DC_ERROR;

        case FTS_ERR:        /* Various failures */
            fprintf(stderr, "traversal failed: %s\n", ent->fts_path);
            errno = ent->fts_errno;
            perror("errno");
            fts_skip_tree(fts, ent);
            return DC_ERROR;

        default:
            fprintf(stderr, "unexpected failure: fts_info=%d: %s\n",
                    ent->fts_info, ent->fts_path);
            errno = ent->fts_errno;
            perror("errno");
            exit(EXIT_FAILURE);
    }
    return DC_OK;
}

enum DC_status
dc (char *const *file, struct dc_options const *opt)
{
    enum DC_status dc_status = DC_OK;
    if (*file)
    {
        int flags = FTS_PHYSICAL;
        FTS *fts;
        if ((fts = fts_open(file, flags, NULL)) == NULL) {
            perror("fts_open");
            dc_status = DC_ERROR;
        }
        while (true)
        {
            FTSENT *ent;
            ent = fts_read(fts);
            if (ent == NULL)
            {
                if (errno != 0)
                {
                    perror("fts_read failed");
                    dc_status = DC_ERROR;
                }
                break;
            }
            enum DC_status s = dc_fts(fts, ent, opt);
            assert (VALID_STATUS(s));
            UPDATE_STATUS (dc_status, s);
        }
        if (fts_close (fts) != 0)
        {
            perror("fts_close failed");
            dc_status = DC_ERROR;
        }
    }
    return dc_status;
}