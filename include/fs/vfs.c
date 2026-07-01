#include "vfs.h"

partitionid_t str_partid(char *str)
{
    partitionid_t part;
    part.drive_id = 0xFF;
    part.partition = 0xFF;
    if (str[0] == 'd') // ensure correct format
    {
        char buff[10];
        int i = 0;
        while ((str[i+1] >= '0' && str[i+1] <= '9') && i < 9) // get numbers
        {
            buff[i] = str[i+1];
            i++;
        }
        buff[i] = '\0';
        int drive_id = atoi(buff);
        if (str[i+1] == 's')
        {
            char buff[10];
            int j = 0;
            while ((str[j+i+1] >= '0' && str[j+i+1] <= '9') && j < 9)
            {
                buff[j] = str[j+i+1];
                j++;
            }
            buff[j] = '\0';
            int partnum = atoi(buff); // aka slot
            part.drive_id = drive_id;
            part.partition = partnum;
        }
    }
    return part;
}

static struct file _fbuffer[128];

struct file resolve_path(const char *path, struct file *parents)
{
    const char *current_pos = path;
    char token[256];

    partitionid_t part;
    bool isrelative = false;
    part.drive_id = 0xFE;
    part.partition = 0xFE;

    struct file f;
    f.starting_lba = 0;
    f.offset = 0;
    f.partition = part;
    f.isdir = false;
    f.parent = NULL;

    int depth = 0;

    while (next_path_token(&current_pos, token))
    {
        if (part.drive_id == 0xFE)
        {
            part = str_partid(token);
            if (part.drive_id == 0xFF)
                isrelative = true;

            f.partition = part;
        }
        else
        {
            if (depth > 0)
            {
                f.isdir = true;
                parents[depth-1] = f;
                f.parent = &parents[depth-1];
                strcpy(f.filename, token);
                f.isdir = false;
            }
            else {
                strcpy(f.filename, token);
                f.isdir = false;
            }
            depth++;
        }
    }
    return f;
}
