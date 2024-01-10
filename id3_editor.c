#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include "id3.h"
#include "id3_parse.h"
#include "file_util.h"
#include "util.h"
#include "hashtable.h"

char t_fids[T_FIDS][5] = {t_fids_arr}; // Supported frame IDs for editing
char s_fids[S_FIDS][5] = {s_fids_arr}; // Supported text frames
char fids[E_FIDS][5] = {t_fids_arr , s_fids_arr}; // Special non-text frames

/**
 * @brief Parses command-line arguments to retrieve new frame data and list of files to edit
 * Allocates memory for required for string of filepaths and returns the amount of files for editing
 * in <path_size>. 
 * 
 * @param argc - Command-line argument count 
 * @param argv - Command-line arguments
 * @param arg_data - Data for arguments provided 
 * @param path - Array of variable length strings that are paths of files to be edited
 * @param path_size - Int pointer containing count of files to be edited
 * @param is_dir - Boolean for given path is directory
 * @param dir_len - Length of filepath directory-to prefix, 0 if arg passed is file.
 * @param num_titles - Pointer to int to save number of titles if provided in args
 * @param verbose - Verbose option selected
 */
void parse_args(int argc, char *argv[], 
                DIRECT_HT *arg_data,
                char ***path, 
                int *path_size,
                int *is_dir,
                int *dir_len,
                int *num_titles,
                int *verbose) {
    
    //File or Dir path is required at minimum
    if (argc < 2) {
        printf("Invalid number of arguments.\n");
        exit(1);
    }

    int opt, errflag=0;
    extern char *optarg;
    extern int optind, optopt;

    while((opt = getopt(argc, argv, "+a:b:t:p:nhv")) != -1) {
        switch(opt) {
            case 'a': // TPE1: Artist name 
                if (strlen(optarg) > 256) errflag++;
                else {
                    direct_address_insert(arg_data, "TPE1", optarg);
                    // printf("Option detected: %s - %s\n", edit_fids[0], arg_data[0]);
                }
                break;
            case 'b': // TALB: Album name
                if (strlen(optarg) > 256) errflag++;
                else {
                    direct_address_insert(arg_data, "TALB", optarg);
                    // printf("Option detected: %s - %s\n", edit_fids[1], arg_data[1]);
                }
                break;
            case 't': // TIT2: Title
                if (strlen(optarg) > 256) errflag++;
                else {
                    direct_address_insert(arg_data, "TIT2", optarg);
                    char *optarg_cp = calloc(strlen(optarg), sizeof(char));
                    strncpy(optarg_cp, optarg, strlen(optarg));   

                    char *tok = strtok(optarg_cp, ",");
                    while (tok) {
                        (*num_titles)++;
                        tok = strtok(NULL, ",");
                    }
                    
                    strncpy(optarg_cp, optarg, strlen(optarg)); 
                    if (*num_titles > 1) {
                        tok = strtok(optarg_cp, ",");
                        direct_address_insert(arg_data, "TIT2", tok);
                    } else {
                        direct_address_insert(arg_data, "TIT2", optarg_cp);
                    }
                    // printf("Option detected: %s - %s\n", edit_fids[2], arg_data[2]);
                }
                break;
            case 'n': // TRCK: Track number
                char *x = calloc(2, sizeof(char));
                strncpy(x, "1", 2);
                direct_address_insert(arg_data, "TRCK", x);
                // printf("Option detected: %s\n", edit_fids[3]);
                break;
            case 'p': // APIC: Attached Picture
                direct_address_insert(arg_data, "APIC", optarg);
                // printf("Option detected: %s\n", edit_fids[4]);

                if (!isJPEG(optarg)) {
                    printf("Image specified is not a JPEG.\n");
                    exit(1);
                }
                break;
            case 'h':
                printf("Usage: ./mp3.exe [OPTION]... PATH\n");
                printf("Reads and edits ID3V2.4 metadata tags.\n\n");
                printf("Supports editing the following tags:\n");
                printf("\tText Information:\n");
                for (int i = 0; i < T_FIDS; i++) printf("\t\t%d. %s\n", i+1, t_fids[i]);
                printf("\tSpecial Information:\n");
                for (int i = 0; i < S_FIDS; i++) printf("\t\t%d. %s\n", i+1, s_fids[i]);
                printf("Options:\n");
                printf("\t%-14s\tWrite new artist name ARTIST for all files in path\n", "-a ARTIST, ");
                printf("\t%-14s\tWrite new album name ALBUM for all files in path\n", "-b ALBUM, ");
                printf("\t%-14s\tWrite new title(s) for all files in path. If PATH\n\t%-11s\tcontains more than one file, TITLE can contain an\n\t%-11s\tequivalent number of titles, separated by commas.\n", "-t TITLE, ", " ", " ");
                printf("\t%-14s\tWrite track number for all files in path. If this\n\t%-11s\toption is selected, the track number for the file\n\t%-11s\tmust be contained in beginning of the filename.\n", "-n, ", " ", " ");
                printf("\t%-14s\tAttach image to all files in path, must be JPEG.\n", "-p IMAGE_PATH, ");
                
                exit(0);
                break;
            case 'v':
                *verbose = 1;
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
    char *filepath = calloc(strlen(argv[optind])+1, sizeof(char));
    if (optind == argc) {
        printf("Missing path argument.\n");
        exit(1);
    } else strncpy(filepath, argv[optind], strlen(argv[optind])+1);

    // POSIX Compliant file info retrieval
    struct stat statbuf; 
    if (stat(filepath, &statbuf) != 0) {
        printf("Error reading input path file %s, errno: %d", filepath, errno);
        exit(1);
    }
        
        
    // If filepath arg is a DIR, retrieve all child filepaths for editing
    if (S_ISDIR(statbuf.st_mode)) {
        *is_dir = 1;
        *dir_len = strlen(filepath);
        if (!(filepath[*dir_len - 1] == '/' || filepath[*dir_len - 1] == '\\')) *dir_len += 1; // check to see last character is directory delimiter

        DIR *dir = opendir(filepath);
        struct dirent *entry;
        int file_count = 0;
        int max_filename_len = 0;

        char *filepath_prefix = calloc(strlen(filepath) + 1 + 1, sizeof(char));
        strncpy(filepath_prefix, filepath, strlen(filepath));
        filepath_prefix[strlen(filepath)] = '\\';
        
        // Count number of files in DIR and find longest length filename
        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);
            
            if (stat(full_path, &statbuf) != 0) {
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                free(full_path);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode) && 
                        (strlen(entry->d_name) > 1 || strncmp(entry->d_name, ".", 2) != 0) && 
                        (strlen(entry->d_name) > 2 || strncmp(entry->d_name, "..", 3) != 0)) {
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
        if (direct_address_search(arg_data, "TIT2") == NULL && (*num_titles != 1 && *num_titles != file_count)) {
            printf("Error, number of titles provided is invalid with the number of files being edited.\n");
            exit(1);
        }

        for (; j < file_count; j++) 
            (*path)[j] = calloc(max_filename_len + strlen(filepath), 1);

        rewinddir(dir);
        j = 0;

        // Save filepaths into <path>
        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);

            if (stat(full_path, &statbuf) != 0) {
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                free(full_path);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode)) {
                if (*(char *)(direct_address_search(arg_data, "TRCK")->val) == '1' && atoi(entry->d_name) <= 0) { // Input validate filename includes track num if opt is set
                    printf("Error obtaining file number for input dir file %s", full_path);
                    exit(1);    
                }

                //save entire path to file in <path>
                strncpy((*path)[j++], full_path, strlen(full_path)); 
            }

            free(full_path); 
        }

        free(filepath_prefix);
    } else { // Filepath argument is a file
        *is_dir = 0;

        char filename[100] = {0};
        strncpy(filename, filepath, 100);
        char *tok = strtok(filename, "/\\");
        int last_tok_len = 0;
        while (tok != NULL) {
            *dir_len += strlen(tok);
            last_tok_len = strlen(tok);
            tok = strtok(NULL, "/\\");
            if (tok != NULL) *dir_len += 1;
        }
        *dir_len -= last_tok_len;

        *path_size = 1;

        // Validate number of files with number of titles
        if (direct_address_search(arg_data, "TIT2") == NULL && *num_titles > 1) {
            printf("Error, number of titles provided is invalid with the number of files being edited.\n");
            exit(1);
        }

        *path = malloc(sizeof(char *));
        **path = filepath;
    }
}



void print_args(int path_size, char **path, DIRECT_HT *arg_data, int dir_len, int is_dir) {
    // Print arguments
    printf("Configuration: \n");
    printf("\tEditing Files: %d\n", path_size);
    for (int i = 0; i < path_size; i++) {
        printf("\t\t%d. %s\n", i+1, path[i]);
    }
    printf("\tEditing strings: \n");
    for (int i = 0; i < E_FIDS; i++) {
        printf("\t\t%s: ", fids[i]);
        if (arg_data->entries[i]) printf("%s\n", (char *)arg_data->entries[i]->val);
        else printf("\n");
    }
    printf("\tDirectory length: %d\n", dir_len);
    if (is_dir)
        printf("\tDir: true\n\n");
    else 
        printf("\tDir: false\n\n");
}



void update_arg_data(DIRECT_HT *arg_data, char **path, int id, int dir_len, int num_titles, int verbose) {
    // Updating track name data for next file
    if (direct_address_search(arg_data, "TRCK")) {
        if (verbose) printf("Updating track index.\n");

        char *trck = calloc(5, sizeof(char));
        int int_trck = get_trck(path[id], dir_len);
        snprintf(trck, 4, "%d", int_trck);
        direct_address_insert(arg_data, "TRCK", trck);
    } 
    
    // Updating title data for next file
    if (direct_address_search(arg_data, "TIT2") && num_titles > 1) { 
        if (verbose) printf("Updating track title.\n");

        char *tok = strtok(NULL, ",");
        direct_address_insert(arg_data, "TIT2", tok);
    }
}



int get_additional_mtdt_sz(const ID3_METAINFO *header_metainfo,
                           const DIRECT_HT *arg_data) {
    int additional_mtdt_sz = 0;
    DIRECT_HT *curr_fid_sz = header_metainfo->fid_sz;

    for (int i = 0; i < arg_data->buckets; i++) {
        if (!arg_data->entries[i]) continue;

        int ind = curr_fid_sz->hash_func(arg_data->entries[i]->key) % curr_fid_sz->buckets;
        if (curr_fid_sz->entries[ind]) additional_mtdt_sz += sizeof_frame_data(curr_fid_sz->entries[ind]->key, (char *)arg_data->entries[i]->val) - *(int*)curr_fid_sz->entries[ind]->val;
        else additional_mtdt_sz += sizeof(ID3V2_FRAME_HEADER) + sizeof_frame_data(arg_data->entries[i]->key, (char *)arg_data->entries[i]->val);
    }

    return additional_mtdt_sz;
}



/**
 * @brief Free header metainfo data
 * 
 * @param metainfo - pointer to metainfo struct to free data 
 */
void free_id3_data(ID3_METAINFO *metainfo) {
    direct_address_destroy(metainfo->fid_sz);
}


/**
 * @brief Free 2D dynamically allocated filepaths array
 * 
 * @param path - Pointer to filepath strings
 * @param path_size - Filepaths count
 */
void free_arg_data(char **path, const int path_size) {
    for (int i = 0; i < path_size; i++) {
        free(path[i]);
    }

    free(path);
}



int main(int argc, char *argv[]) {   
    char **path; //Array of filepaths
    int path_size; //Number of files in <path>;
    int is_dir = 0; //Boolean flag for if given path is directory 
    int dir_len = 0; //Length of directory prefix in filepath
    int verbose = 0;
    int num_titles = 0;

    DIRECT_HT *arg_data = direct_address_create(E_FIDS, e_fids_hash); // Direct Address Hash Table for argument data

    parse_args(argc, argv, arg_data, &path, &path_size, &is_dir, &dir_len, &num_titles, &verbose);
    if (verbose) print_args(path_size, path, arg_data, dir_len, is_dir);

    // Open, edit, and print ID3 metadata for each file  
    for (int id = 0; id < path_size; id++) {
        FILE *f = fopen(path[id], "r+b");  
        if (f == NULL) {
            printf("File does not exist.\n");
            exit(1);
        }

        ID3V2_HEADER header;
        ID3_METAINFO header_metainfo;
        read_header(&header, f, path[id], verbose);
        get_ID3_metainfo(&header_metainfo, &header, f, verbose);

        update_arg_data(arg_data, path, id, dir_len, num_titles, verbose);

        if (verbose) printf("Calculating additional metadata.\n");
        
        // Calculate new metadata size to predict if metadata header has to be extended
        int additional_mtdt_sz = get_additional_mtdt_sz(&header_metainfo, arg_data);
        int allocated_mtdt_sz = synchsafeint32ToInt(header.size);
        if (header_metainfo.metadata_sz + additional_mtdt_sz >= allocated_mtdt_sz) {
            f = extend_header(additional_mtdt_sz, header_metainfo, f, path[id]);
            get_ID3_metainfo(&header_metainfo, &header, f, 0);
        }

        if (verbose) printf("Editing file...\n");

        int bytes_read = 0;

        // Search and edit existing frames
        for(int i = 0; i < header_metainfo.frame_count; i++) {
            ID3V2_FRAME_HEADER frame_header;
            read_frame_header(&frame_header, f);

            int readonly = 0;
            int additional_bytes = parse_frame_header_flags(frame_header.flags, &readonly, f);
            int len_data = synchsafeint32ToInt(frame_header.size);

            if (!in_key_set(arg_data, frame_header.fid)) {
                read_frame_data(f, len_data);
                bytes_read += sizeof(ID3V2_FRAME_HEADER) + additional_bytes + len_data;
                continue;
            } 

            int ind = arg_data->hash_func(frame_header.fid) % arg_data->buckets;
            int new_frame_len = len_data;
            
            // If frame not readonly, is editable, and must be edited
            if (!readonly && arg_data->entries[ind]) {
                int remaining_metadata_sz = header_metainfo.metadata_sz - (bytes_read + sizeof(ID3V2_FRAME_HEADER) + len_data);
                new_frame_len = sizeof_frame_data(frame_header.fid, (char *)arg_data->entries[ind]->val);
                char *frame_data = get_frame_data(frame_header.fid, (char *)arg_data->entries[ind]->val);
                edit_frame_data(frame_data, new_frame_len, len_data, remaining_metadata_sz, additional_bytes, f);

                free(frame_data);
            }

            read_frame_data(f, new_frame_len);
            bytes_read += sizeof(ID3V2_FRAME_HEADER) + additional_bytes + len_data; 
        }

        if (verbose) printf("Appending frames to file...\n");

        // Append necessary new frames
        for (int i = 0; i < E_FIDS; i++) {
            if (!arg_data->entries[i] || in_key_set(header_metainfo.fid_sz, e_fids_reverse_lookup[i])) continue;

            // Construct new frame header
            ID3V2_FRAME_HEADER frame_header;
            strncpy(frame_header.fid, e_fids_reverse_lookup[i], 4);
            int new_frame_len = sizeof_frame_data(frame_header.fid, (char *)arg_data->entries[i]->val);
            char *frame_data = get_frame_data(frame_header.fid, (char *)arg_data->entries[i]->val);

            char flags[2] = {'\0', '\0'};
            intToSynchsafeint32(new_frame_len, frame_header.size);
            strncpy(frame_header.flags, flags, 2);

            append_new_frame(frame_header, frame_data, new_frame_len, f);
            free(frame_data);
            
            // Update metainfo struct
            int *fid_sz_new_frame = calloc(1, sizeof(int));
            *fid_sz_new_frame = new_frame_len;
            direct_address_insert(header_metainfo.fid_sz, frame_header.fid, fid_sz_new_frame);

            header_metainfo.frame_count++;
        }
        
        if (verbose) {
            // Print all ID3 tags
            printf("Reading %s metadata :\n\n", path[id]);
            print_data(f, header_metainfo); 
        }
        
        free_id3_data(&header_metainfo);
        fclose(f);
    }

    free_arg_data(path, path_size);

    return 0;
}