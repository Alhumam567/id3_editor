#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "id3_editor.h"
#include "id3_parse_util.c"
#include "file_util.c"


/**
 * @brief Parses command-line arguments to retrieve new frame data and list of files to edit
 * Allocates memory for required for string of filepaths and returns the amount of files for editing
 * in <path_size>. 
 * 
 * @param argc - Command-line argument count 
 * @param argv - Command-line arguments
 * @param edit_fids - Array of frame ID strings
 * @param frame_args - Bool array of frames to edit
 * @param edit_fids_str - Array of respective frame data for frame IDs 
 * @param fid_len - Count of editable frame IDs  
 * @param path - Pointer to array of variable length strings that are paths of files to be edited
 * @param path_size - Int pointer containing count of files to be edited
 * @param dir_len - Length of filepath directory-to prefix, 0 if arg passed is file.
 * @param num_titles - Pointer to int to save number of titles if provided in args
 */
void parse_args(int argc, char *argv[], 
                char edit_fids[E_FIDS][5],
                int frame_args[E_FIDS],
                char edit_fids_str[E_FIDS][256],
                int fid_len,
                char ***path, 
                int *path_size,
                int *dir_len,
                int *num_titles) {
    
    //File or Dir path is required at minimum
    if (argc < 2) {
        printf("Invalid number of arguments.\n");
        exit(1);
    }

    int opt, errflag=0;
    extern char *optarg;
    extern int optind, optopt;

    while((opt = getopt(argc, argv, "+a:b:t:nh")) != -1) {
        switch(opt) {
            case 'a': // TPE1: Artist name 
                if (strlen(optarg) > 256) errflag++;
                else {
                    strncpy(edit_fids_str[0], optarg, strlen(optarg));
                    printf("Option detected: %s - %s\n", edit_fids[0], edit_fids_str[0]);

                    frame_args[0] = 0;
                }
                break;
            case 'b': // TALB: Album name
                if (strlen(optarg) > 256) errflag++;
                else {
                    strncpy(edit_fids_str[1], optarg, strlen(optarg));
                    printf("Option detected: %s - %s\n", edit_fids[1], edit_fids_str[1]);
                
                    frame_args[1] = 0;
                }
                break;
            case 't': // TIT2: Title
                if (strlen(optarg) > 256) errflag++;
                else {
                    strncpy(edit_fids_str[2], optarg, strlen(optarg));   
                    printf("Option detected: %s - %s\n", edit_fids[2], edit_fids_str[2]);
                
                    frame_args[2] = 0;

                    char titles[256];
                    strncpy(titles, optarg, strlen(optarg));
                    char *tok = strtok(titles, ",");
                    while (tok != NULL) {
                        (*num_titles)++;
                        tok = strtok(NULL, ",");
                    }
                }
                break;
            case 'n': // TRCK: Track number
                strncpy(edit_fids_str[3], "1", 1);
                printf("Option detected: %s\n", edit_fids[3]);
                
                frame_args[3] = 0;
                break;
            case 'h':
                printf("Usage: ./mp3.exe [OPTION]... PATH\n");
                printf("Reads and edits ID3 metadata tags.\n");
                printf("Options:\n");
                printf("\t%-14s\tWrite new artist name ARTIST for all files in path\n", "-a ARTIST, ");
                printf("\t%-14s\tWrite new album name ALBUM for all files in path\n", "-b ALBUM, ");
                printf("\t%-14s\tWrite new title(s) for all files in path. If PATH\n\t%-11s\tcontains more than one file, TITLE can contain an\n\t%-11s\tequivalent number of titles, separated by commas.\n", "-t TITLE, ", " ");
                printf("\t%-14s\tWrite track number for all files in path. If this\n\t%-11s\toption is selected, the track number for the file\n\t%-11s\tmust be contained in beginning of the filename.\n", "-n, ", " ");
                
                exit(0);
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

    // Read filepath argument
    char *filepath;
    if (optind == argc) {
        printf("Missing path argument.\n");
        exit(1);
    } else {
        filepath = argv[optind];
    }

    // POSIX Compliant file info retrieval
    struct stat statbuf; 
    if (stat(filepath, &statbuf) != 0) {
        printf("Error reading input path file %s, errno: %d", filepath, errno);
        exit(1);
    }
        
    // If filepath arg is a DIR, retrieve all child filepaths for editing
    if (S_ISDIR(statbuf.st_mode)) {
        *dir_len = strlen(filepath);

        DIR *dir = opendir(filepath);
        DIR *item_dir;
        struct dirent *entry;
        int file_count = 0;
        int max_filename_len = 0;

        char *filepath_prefix = calloc(strlen(filepath) + 1 + 1, 1);
        strncpy(filepath_prefix, filepath, strlen(filepath));
        filepath_prefix[strlen(filepath)] = '\\';
        
        // Count number of files in DIR and find longest length filename
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

        // Allocate memory for all filepaths
        *path = malloc(sizeof(char *)*file_count);
        int j = 0;
        *path_size = file_count;

        // Validate number of files with number of titles
        if (frame_args[2] == 0 && (*num_titles != 1 && *num_titles != file_count)) {
            printf("Error, number of titles provided is invalid with the number of files being edited.\n");
            exit(0);
        }

        for (; j < file_count; j++) 
            (*path)[j] = calloc(max_filename_len + strlen(filepath), 1);

        rewinddir(dir);
        j = 0;

        // Save filepaths into <path>
        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);

            if (stat(full_path, &statbuf) != 0) {
                free(full_path);
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode)) {
                if (edit_fids_str[3][0] == '1' && atoi(entry->d_name) <= 0) { // Input validate filename includes track num if opt is set
                    printf("Error obtaining file number for input dir file %s", full_path);
                    exit(1);    
                }

                strncpy((*path)[j++], full_path, strlen(full_path));
            }

            free(full_path); 
        }

        free(filepath_prefix);
    } else { // Filepath argument is a file
        *dir_len = 0;
        *path_size = 1;

        // Validate number of files with number of titles
        if (frame_args[2] == 0 && *num_titles > 1) {
            printf("Error, number of titles provided is invalid with the number of files being edited.\n");
            exit(0);
        }

        *path = malloc(sizeof(char *));
        **path = malloc(strlen(filepath));
        **path = filepath;
    }

    printf("\n");
}



/**
 * @brief Reverse lookup for frame ID to index
 * 
 * @param fids - Array of frame IDs
 * @param fids_len - Count of frame IDs
 * @param fid - Frame ID string
 * @return int - Index of given frame ID
 */
int get_fid_index(char fids[E_FIDS][5], char *fid) {
    for (int i = 0; i < E_FIDS; i++) {
        if (strncmp(fids[i], fid, 4) == 0) return i;
    }

    return -1;
}


/**
 * @brief Free 2D dynamically allocated filepaths array
 * 
 * @param path - Pointer to filepath strings
 * @param path_size - Filepaths count
 */
void free_arg_data(char **path, int path_size) {
    for (int i = 0; i < path_size; i++) {
        free(path[i]);
    }

    free(path);
}



int main(int argc, char *argv[]) {
    extern char fids[E_FIDS][5]; //Array of supported frame IDs for editing
    
    char **path; //Array of filepaths
    int path_size; //Number of files in <path>;
    int dir_len; //Length of directory prefix in filepath

    int frame_args[E_FIDS] = { 1 }; //Array of bool flags representing frames that need to be edited
    char new_fid_data[E_FIDS][256] = {'\0'}; //New frame data
    int num_titles = 0;

    parse_args(argc, argv, fids, frame_args, new_fid_data, E_FIDS, &path, &path_size, &dir_len, &num_titles);

    // Print arguments
    printf("Configuration: \n");
    printf("\tEditing Files: %d\n", path_size);
    for (int i = 0; i < path_size; i++) {
        printf("\t\t%d. %s\n", i+1, path[i]);
    }
    printf("\tEditing strings: \n");
    for (int i = 0; i < E_FIDS; i++) {
        printf("\t\t%s: %s\n", fids[i], new_fid_data[i]);
    }
    if (dir_len > 0)
        printf("\tDir: true\n\n");
    else 
        printf("\tDir: false\n\n");

    char titles[256];
    strncpy(titles, new_fid_data[get_fid_index(fids, "TIT2")], 256);
    char *init_tok = strtok(titles, ",");
    strncpy(new_fid_data[get_fid_index(fids, "TIT2")], init_tok, strlen(init_tok));

    // Open, edit, and print ID3 metadata for each file  
    for (int id = 0; id < path_size; id++) {
        FILE *f = fopen(path[id], "r+b");  
        if (f == NULL) {
            printf("File does not exist.\n");
            exit(1);
        }

        ID3V2_HEADER header;
        read_header(&header, f, path[id], 1);

        ID3_METAINFO header_metainfo;
        get_ID3_metainfo(&header_metainfo, &header, f, 1);
        
        int frames_edited[E_FIDS];
        memcpy(frames_edited, frame_args, sizeof(frames_edited));
        
        printf("Editing file...\n\n");

        // Special case for TRCK frame and retrieving track number from filename
        int trck_ind = get_fid_index(fids, "TRCK");
        if (frame_args[trck_ind] == 0) {
            char trck[4] = {'\0'};
            itoa(get_trck(path[id], dir_len + 1), trck, 10);
            strncpy(new_fid_data[trck_ind], trck, 4);
        } 

        int tit2_ind = get_fid_index(fids, "TIT2");
        if (id > 0 && frame_args[tit2_ind] == 0 && num_titles > 1) { 
            char *tok = strtok(NULL, ",");
            strncpy(new_fid_data[tit2_ind], tok, strlen(tok));
        }

        int bytes_read = 0;
        int metadata_sz = header_metainfo.metadata_sz;
        char *data;
        char fid_str[5] = {'\0'};

        // Search and edit existing frames
        for(int i = 0; i < header_metainfo.frame_count; i++) {
            ID3V2_FRAME_HEADER frame_header;
            read_frame_header(&frame_header, f);    

            int len_data = get_frame_data_len(frame_header);
            strncpy(fid_str, frame_header.fid, 4);
            int fid_index = get_fid_index(fids, fid_str);

            // If frame is editable and argument passed for editing it
            if (fid_index != -1 && frame_args[fid_index] == 0) {
                frames_edited[fid_index] = 1;

                int new_len_data = write_new_len(strlen(new_fid_data[fid_index]), f, 0);
                int remaining_metadata_sz = header_metainfo.metadata_sz - (bytes_read + 10 + len_data);
                overwrite_frame_data(new_fid_data[fid_index], len_data, remaining_metadata_sz, f);  

                len_data = new_len_data;
            }

            read_frame_data(f, len_data);
            bytes_read += 10 + len_data; 
        }

        // Append necessary new frames
        for (int i = 0; i < E_FIDS; i++) {
            if (!frames_edited[i]) {
                // Special case for TRCK frame and retrieving track number from filename
                if (strncmp(fids[i], "TRCK", 4) == 0) {
                    char trck[4] = {'\0'};
                    itoa(get_trck(path[id], dir_len + 1), trck, 10);
                    strncpy(new_fid_data[i], trck, 4);
                }

                ID3V2_FRAME_HEADER frame_header;               
                char flags[2] = {'\0'};
                strncpy(frame_header.fid, fids[i], 4);
                int_to_header_ssint(1 + strlen(new_fid_data[i]), frame_header.size);
                strncpy(frame_header.flags, flags, 2);

                write_new_frame(frame_header, new_fid_data[i], f);
                
                char (*tmp_fids)[4] = malloc((header_metainfo.frame_count+1) * sizeof(char *));
                for (int j = 0; j < header_metainfo.frame_count; j++) strncpy(tmp_fids[j], header_metainfo.fids[j], 4);
                strncpy(tmp_fids[header_metainfo.frame_count], fids[i], 4);
                free(header_metainfo.fids);
                
                header_metainfo.fids = tmp_fids; 
                header_metainfo.frame_count++;
            }
        }
        
        // Print all ID3 data
        printf("Reading %s metadata :\n\n", path[id]);
        print_data(f, header_metainfo);

        fclose(f);
    }

    free_arg_data(path, path_size);

    return 0;
}