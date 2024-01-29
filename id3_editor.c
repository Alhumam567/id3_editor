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
 * @brief Updates arguments that vary between files if needed (titles, track number)
 * 
 * @param arg_data - Argument table
 * @param file - Next file to edit
 * @param dir_len - Length of directory prefix to file 
 * @param title - Next title
 * @param num_titles - Total number of files to be edited
 * @param verbose - Bool to print out details
 */
void update_arg_data(DIRECT_HT *arg_data, char *file, int dir_len, char *title, int num_titles, int verbose) {
    if (direct_address_search(arg_data, "TRCK")) { // Updating track name data for next file
        if (verbose) printf("Updating track index.\n");

        char *trck = calloc(5, sizeof(char));
        int int_trck = get_trck(file, dir_len);
        snprintf(trck, 4, "%d", int_trck);
        direct_address_insert(arg_data, "TRCK", trck);
    } 
    
    if (direct_address_search(arg_data, "TIT2") && num_titles > 1) { // Updating title data for next file
        if (verbose) printf("Updating track title.\n");

        direct_address_insert(arg_data, "TIT2", title);
    }
}


/**
 * @brief Calculates the change in metadata size to detect if file needs to be extended 
 * 
 * @param header_metainfo - File metainfo struct
 * @param arg_data - Argument data for file
 * @return int - Total size difference in current metadata and metadata with the new data
 */
int mtdt_sz_diff(const ID3_METAINFO *header_metainfo, const DIRECT_HT *arg_data) {
    int mtdt_sz_diff = 0;
    DIRECT_HT *curr_fid_sz = header_metainfo->fid_sz;

    for (int i = 0; i < arg_data->buckets; i++) {
        if (!arg_data->entries[i]) continue;

        int ind = dt_hash(curr_fid_sz, arg_data->entries[i]->key);
        if (curr_fid_sz->entries[ind]) mtdt_sz_diff += sizeof_frame_data(curr_fid_sz->entries[ind]->key, (char *)arg_data->entries[i]->val) - *(int*)curr_fid_sz->entries[ind]->val;
        else mtdt_sz_diff += sizeof(ID3V2_FRAME_HEADER) + sizeof_frame_data(arg_data->entries[i]->key, (char *)arg_data->entries[i]->val);
    }

    return mtdt_sz_diff;
}


/**
 * @brief Frees filepath strings and titles if necessary
 * 
 * @param path - Pointer to filepath strings
 * @param path_size - Filepaths count
 * @param titles - List of track titles
 * @param num_titles - Number of titles
 */
void free_str_arr(char **path, const int path_size, char **titles, const int num_titles) {
    for (int i = 0; i < path_size; i++) free(path[i]);
    free(path);
    if (num_titles > 1) free(titles);
}

void parse_args(int argc, char *argv[], 
                DIRECT_HT *arg_data,
                char ***path, 
                int *path_size,
                int *is_dir,
                int *dir_len,
                char ***titles,
                int *num_titles,
                int *verbose);

void print_args(int path_size, char **path, DIRECT_HT *arg_data, int dir_len, int is_dir);

int main(int argc, char *argv[]) {   
    char **path; //Array of filepaths
    int path_size; //Number of files in <path>;
    int is_dir = 0; //Boolean flag for if given path is directory 
    int dir_len = 0; //Length of directory prefix in filepath
    int verbose = 0;
    char **titles  = NULL;
    int num_titles = 0;

    DIRECT_HT *arg_data = direct_address_create(E_FIDS, e_fids_hash); // Direct Address Hash Table for argument data

    parse_args(argc, argv, arg_data, &path, &path_size, &is_dir, &dir_len, &titles, &num_titles, &verbose);
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

        char *t = (titles) ? titles[id] : NULL;
        update_arg_data(arg_data, path[id], dir_len, t, num_titles, verbose);

        if (verbose) printf("Calculating additional metadata...\n");
        
        // Calculate new metadata size to predict if metadata header has to be extended
        int sz_diff = mtdt_sz_diff(&header_metainfo, arg_data);
        int allocated_mtdt_sz = synchsafeint32ToInt(header.size);
        if (header_metainfo.metadata_sz + sz_diff >= allocated_mtdt_sz) {
            if (verbose) printf("Extending file size...\n");
            f = extend_header(sz_diff, header_metainfo, f, path[id]);
            direct_address_destroy(header_metainfo.fid_sz);
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
            int ind = dt_hash(arg_data, frame_header.fid);

            if (in_key_set(arg_data, frame_header.fid) && !readonly) {
                int remaining_metadata_sz = header_metainfo.metadata_sz - (bytes_read + sizeof(ID3V2_FRAME_HEADER) + len_data);
                int new_frame_len = sizeof_frame_data(frame_header.fid, (char *)arg_data->entries[ind]->val);
                char *frame_data = get_frame_data(frame_header.fid, (char *)arg_data->entries[ind]->val);
                edit_frame_data(frame_data, new_frame_len, len_data, remaining_metadata_sz, additional_bytes, f);
                free(frame_data);
                len_data = new_frame_len;
            }
            read_frame_data(f, len_data);
            bytes_read += sizeof(ID3V2_FRAME_HEADER) + additional_bytes + len_data; 
        }

        if (verbose) printf("Appending frames to file...\n");

        // Append necessary new frames
        for (int i = 0; i < E_FIDS; i++) {
            if (!arg_data->entries[i] || in_key_set(header_metainfo.fid_sz, e_fids_reverse_lookup[i])) continue;

            // Construct new frame header
            ID3V2_FRAME_HEADER frame_header;
            char flags[2] = {'\0', '\0'};
            strncpy(frame_header.fid, e_fids_reverse_lookup[i], 4);
            int new_frame_len = sizeof_frame_data(frame_header.fid, (char *)arg_data->entries[i]->val);
            char *frame_data = get_frame_data(frame_header.fid, (char *)arg_data->entries[i]->val);
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
        
        if (verbose) { // Print all ID3 tags
            printf("Reading %s metadata :\n", path[id]);
            print_data(f, &header_metainfo); 
        }
        
        direct_address_destroy(header_metainfo.fid_sz);
        if (id == path_size - 1) direct_address_destroy(arg_data);
        fclose(f);
    }

    free_str_arr(path, path_size, titles, num_titles);

    return 0;
}


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
                char ***titles,
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
    char *t;

    while((opt = getopt(argc, argv, "+a:b:t:p:nhv")) != -1) {
        switch(opt) {
            case 'a':; // TPE1: Artist name 
                t = calloc(strlen(optarg) + 1, sizeof(char));
                strncpy(t, optarg, strlen(optarg));
                direct_address_insert(arg_data, "TPE1", t);
                break;
            case 'b':; // TALB: Album name
                t = calloc(strlen(optarg) + 1, sizeof(char));
                strncpy(t, optarg, strlen(optarg));
                direct_address_insert(arg_data, "TALB", t);
                break;
            case 't':; // TIT2: Title
                t = calloc(strlen(optarg) + 1, sizeof(char));
                strncpy(t, optarg, strlen(optarg));

                direct_address_insert(arg_data, "TIT2", t);
                char *optarg_cp = calloc(strlen(optarg) + 1, sizeof(char));
                strncpy(optarg_cp, optarg, strlen(optarg) + 1);   

                char *tok = strtok(optarg_cp, ",");
                while (tok) {
                    (*num_titles)++;
                    tok = strtok(NULL, ",");
                }
                
                strncpy(optarg_cp, optarg, strlen(optarg)); 
                if (*num_titles > 1) {
                    *titles = calloc(*num_titles, sizeof(char *));
                    tok = strtok(optarg_cp, ",");
                    int i = 0;
                    while (tok) {
                        (*titles)[i] = calloc(256, sizeof(char));
                        strncpy((*titles)[i++], tok, strlen(tok));
                        tok = strtok(NULL, ",");
                    }
                }
                free(optarg_cp);
                break;
            case 'n':; // TRCK: Track number
                char *x = calloc(2, sizeof(char));
                strncpy(x, "1", 2);
                direct_address_insert(arg_data, "TRCK", x);

                break;
            case 'p':; // APIC: Attached Picture
                t = calloc(strlen(optarg) + 1, sizeof(char));
                strncpy(t, optarg, strlen(optarg));
                direct_address_insert(arg_data, "APIC", t);

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
                
                direct_address_destroy(arg_data);
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

        char *filepath_prefix = calloc(*dir_len + 1, sizeof(char));
        strncpy(filepath_prefix, filepath, strlen(filepath) + 1);
        if (!(filepath[*dir_len - 1] == '/' || filepath[*dir_len - 1] == '\\')) strncat(filepath_prefix, "/", 2);
        
        // Count number of files in DIR and find longest length filename
        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);
            
            if (stat(full_path, &statbuf) != 0) {
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                free(full_path);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode) && 
                        (strlen(entry->d_name) > 1 || strncmp(entry->d_name, ".", 2) != 0) && 
                        (strlen(entry->d_name) > 2 || strncmp(entry->d_name, "..", 3) != 0) &&
                        (strlen(entry->d_name) > 4 && !strncmp(entry->d_name + strlen(entry->d_name) - 4, ".mp3", 4))) {
                file_count++;

                if (strlen(entry->d_name) > max_filename_len) max_filename_len = strlen(entry->d_name);
            }
            free(full_path);
        }
        // Validate number of files with number of titles
        if (direct_address_search(arg_data, "TIT2") != NULL && (*num_titles != 1 && *num_titles != file_count)) {
            printf("Error, number of titles provided is invalid with the number of files being edited.\n");
            exit(1);
        }

        // Allocate memory for all filepaths
        *path = calloc(file_count, sizeof(char *));
        int j = 0;
        *path_size = file_count;
        for (; j < file_count; j++) 
            (*path)[j] = calloc(max_filename_len + strlen(filepath) + 1, 1);

        rewinddir(dir);
        j = 0;
        // Save filepaths into <path>
        while ((entry = readdir(dir)) != NULL) {
            char *full_path = concatenate(filepath_prefix, entry->d_name);

            if (stat(full_path, &statbuf) != 0) {
                printf("Error reading input dir file %s, errno: %d", full_path, errno);
                free(full_path);
                exit(1);
            } else if (S_ISREG(statbuf.st_mode) && (strlen(entry->d_name) > 4 && !strncmp(entry->d_name + strlen(entry->d_name) - 4, ".mp3", 4))) {
                HT_ENTRY *e = direct_address_search(arg_data, "TRCK");
                if ((e != NULL && *(char *)(e->val) == '1') && atoi(entry->d_name) <= 0) { // Input validate filename includes track num if opt is set
                    printf("Error obtaining file number for input dir file %s", full_path);
                    exit(1);    
                }
                strncpy((*path)[j++], full_path, strlen(full_path) + 1); //save entire path to file in <path>
            }

            free(full_path); 
        }
        closedir(dir);
        free(filepath);
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
        if (direct_address_search(arg_data, "TIT2") != NULL && *num_titles > 1) {
            printf("Error, number of titles provided is invalid with the number of files being edited.\n");
            exit(1);
        }

        *path = malloc(sizeof(char *));
        **path = filepath;
    }
}

void print_args(int path_size, char **path, DIRECT_HT *arg_data, int dir_len, int is_dir) {
    printf("Configuration: \n");
    printf("\tEditing Files: %d\n", path_size);
    for (int i = 0; i < path_size; i++) {
        printf("\t\t%d. %s\n", i+1, path[i]);
    }
    printf("\tEditing strings: \n");
    for (int i = 0; i < E_FIDS; i++) {
        printf("\t\t%s: ", e_fids_reverse_lookup[i]);
        if (arg_data->entries[i]) printf("%s\n", (char *)arg_data->entries[i]->val);
        else printf("\n");
    }
    printf("\tDirectory length: %d\n", dir_len);
    if (is_dir)
        printf("\tDir: true\n\n");
    else 
        printf("\tDir: false\n\n");
}
