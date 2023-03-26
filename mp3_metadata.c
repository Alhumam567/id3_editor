#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "mp3_metadata.h"
#include "id3_parse_util.c"
#include "file_util.c"


void parse_args(int argc, char *argv[], 
                char ***path, int *path_size, 
                char ***edit_fids,
                char ***edit_fids_str,
                int *fid_len,
                int *is_dir) {
    if (argc < 2) {
        printf("Invalid number of arguments.\n");
        exit(1);
    }

    char temp[4][5] = {"TPE1", "TALB", "TIT2", "TRCK"};
    *fid_len = 4;
    *edit_fids = calloc(*fid_len, sizeof(char [5]));
    for (int i = 0; i < *fid_len; i++) {
        (*edit_fids)[i] = calloc(1, sizeof(char [5]));
        strncpy((*edit_fids)[i], temp[i], 4);
    }
    *edit_fids_str = calloc(*fid_len, sizeof(char [256]));
    for (int i = 0; i < *fid_len; i++) (*edit_fids_str)[i] = calloc(1, sizeof(char [256]));

    int opt, errflag=0;
    extern char *optarg;
    extern int optind, optopt;

    while((opt = getopt(argc, argv, "+a:b:n:t:")) != -1) {
        switch(opt) {
            case 'a': // Artist name 
                if (strlen(optarg) > 256) errflag++;
                else {
                    strncpy((*edit_fids_str)[0], optarg, strlen(optarg));
                    printf("Option detected: %s - %s\n", edit_fids[0], (*edit_fids_str)[0]);
                }
                break;
            case 'b': // Album name
                if (strlen(optarg) > 256) errflag++;
                else {
                    strncpy((*edit_fids_str)[1], optarg, strlen(optarg));
                    printf("Option detected: %s - %s\n", edit_fids[1], (*edit_fids_str)[1]);
                }
                break;
            case 't': // Title
                if (strlen(optarg) > 256) errflag++;
                else {
                    strncpy((*edit_fids_str)[2], optarg, strlen(optarg));   
                    printf("Option detected: %s - %s\n", edit_fids[2], (*edit_fids_str)[2]);
                }
                break;
            case 'n': // Track number
                if (strncmp(optarg, "inc", 3) != 0 && strncmp(optarg, "dec", 3) != 0) errflag++;
                else {
                    strncpy((*edit_fids_str)[3], optarg, 3);
                    printf("Option detected: %s - %s\n", edit_fids[3], (*edit_fids_str)[3]);
                }
                break;
            case '?':
                printf("Option \'%c\' is not recognized.\n", optopt);
                errflag++;
                break;
            case ':':
                printf("Option \'%c\' is missing an argument.\n", optopt);
                errflag++;
                break;
        }
    }

    if (errflag) exit(1);

    char *filepath;
    if (optind == argc) {
        printf("Missing path argument.\n");
        exit(1);
    } else {
        filepath = argv[optind];
    }

    struct stat statbuf;
    if (stat(filepath, &statbuf) != 0) {
        printf("Error reading input path file %s, errno: %d", filepath, errno);
        exit(1);
    }
        
    if (S_ISDIR(statbuf.st_mode)) {
        *is_dir = 1;

        DIR *dir = opendir(filepath);
        DIR *item_dir;
        struct dirent *entry;
        int file_count = 0;
        int max_filename_len = 0;

        char *filepath_prefix = calloc(strlen(filepath) + 1 + 1, 1);
        strncpy(filepath_prefix, filepath, strlen(filepath));
        filepath_prefix[strlen(filepath)] = '\\';

        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);
            
            if (stat(full_path, &statbuf) != 0) {
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                free(full_path);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode) && entry->d_name != "." && entry->d_name != "..") {
                file_count++;

                if (strlen(entry->d_name) > max_filename_len) max_filename_len = strlen(entry->d_name);
            }
            
            free(full_path);
        }

        *path = malloc(sizeof(char *)*file_count);
        int j = 0;
        *path_size = file_count;

        for (; j < file_count; j++) 
            (*path)[j] = calloc(max_filename_len + strlen(filepath), 1);

        rewinddir(dir);

        j = 0;
        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);

            if (stat(full_path, &statbuf) != 0) {
                free(full_path);
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode)) {
                strncpy((*path)[j++], full_path, strlen(full_path));
            }

            free(full_path); 
        }

        free(filepath_prefix);
    } else {
        *is_dir = 0;
        *path_size = 1;

        *path = malloc(sizeof(char *));
        **path = malloc(strlen(filepath));
        **path = filepath;
    }

    printf("\n");
}



int get_fid_index(char *(fids[4]), int fids_len, char *fid) {
    for (int i = 0; i < fids_len; i++) {
        if (strncmp(fids[i], fid, 4) == 0) return i;
    }

    return -1;
}



void free_fid_data(char **fids, char **fid_data, int fid_len) {
    for (int i = 0; i < fid_len; i++) {
        free(fids[i]);
    }

    free(fids);

    for (int i = 0; i < fid_len; i++) {
        free(fid_data[i]);
    }

    free(fid_data);
}



void free_arg_data(char **path, int path_size) {
    for (int i = 0; i < path_size; i++) {
        free(path[i]);
    }

    free(path);
}



int main(int argc, char *argv[]) {
    char **path;
    char **editable_fid;
    char **new_fid_data;
    int fid_len, is_dir, path_size;

    parse_args(argc, argv, &path, &path_size, &editable_fid, &new_fid_data, &fid_len, &is_dir);

    // Print arguments
    printf("Configuration: \n");
    printf("\tEditing Files: %d\n", path_size);
    for (int i = 0; i < path_size; i++) {
        printf("\t\t%d. %s\n", i+1, path[i]);
    }
    printf("\tEditing strings: \n");
    for (int i = 0; i < fid_len; i++) {
        printf("\t\t%s: %s\n", editable_fid[i], new_fid_data[i]);
    }
    printf("\tis_dir: %d\n\n", is_dir);


    for (int id = 0; id < path_size; id++) {
        FILE *f = fopen(path[id], "r+b");
        
        if (f == NULL) {
            printf("File does not exist.\n");
            exit(1);
        }

        ID3V2_HEADER header;
        read_header(&header, f, path[id], 1);

        ID3_METAINFO header_metainfo;
        get_ID3_meta_info(&header_metainfo, &header, f, 1);
        
        // Search and edit existing FIDs
        int bytes_read = 0; 
        char *data;
        char fid_str[5] = {'\0'};
        for(int i = 0; i < header_metainfo.frame_count; i++) {
            ID3V2_FRAME_HEADER frame_header;
            read_frame_header(&frame_header, f);    

            int len_data = get_frame_data_len(frame_header);
            strncpy(fid_str, frame_header.fid, 4);
            int fid_index = get_fid_index(editable_fid, fid_len, fid_str);

            if (fid_index != -1 && new_fid_data[fid_index][0] != '\0') {
                fseek(f, -6, SEEK_CUR); // Seek back to frame header size 
                len_data = write_new_len(strlen(new_fid_data[fid_index]), f, 0);
                fseek(f, 2 + 1, SEEK_CUR); // Seek past flag and constant first null byte

                int remaining_metadata_sz = header_metainfo.metadata_sz - (bytes_read + 10);
                int bytes_written = write_new_data(new_fid_data[fid_index], frame_header, remaining_metadata_sz, f);
            }
            read_frame_data(f, len_data);
            bytes_read += 10 + len_data; 
        }
        
        printf("Reading %s metadata :\n\n", path[id]);
        print_data(f, header_metainfo.frame_count);


        fclose(f);
    }

    free_fid_data(editable_fid, new_fid_data, fid_len);
    free_arg_data(path, path_size);

    return 0;
}