/*
    Command Line Interface for BARF runtime loader and BARF file dump.
*/

#include "barf/types.h"
#include "barf/barf.h"

#define BARF_VERSION "0.0.1-dev"

int main(int argc, char** argv) {
    bool dump = false;
    bool print_help = false;
    bool print_version = false;
    bool combine = false;

    const char* output_file = NULL;

    int user_arg_index = -1;

    const char** input_files = malloc(50 * sizeof(char*));
    int input_files_len = 0;

    int argi = 1;
    while (argi < argc) {
        const char* arg = argv[argi];
        argi++;

        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            print_help = true;
            break;
        } else if (!strcmp(arg, "-v")) {
            print_version = true;
            break;
        } else if (!strcmp(arg, "-d")) {
            dump = true;
        } else if (!strcmp(arg, "-c") || !strcmp(arg, "--combine")) {
            combine = true;
        } else if (!strcmp(arg, "--")) {
            user_arg_index = argi;
            break;
        } else if (!strcmp(arg, "-o")) {
            if (argi >= argc) {
                fprintf(stderr, "ERROR barf: Expected file after '%s'\n", arg);
                return 1;
            }
            if (output_file) {
                fprintf(stderr, "ERROR barf: Only one output file allowed. (extra file was '%s')\n", arg);
                return 1;
            }
            output_file = argv[argi];
            argi++;
        } else {
            // @TODO realloc
            ASSERT(input_files_len < 50);
            input_files[input_files_len++] = arg;
        }
    }
    if (print_help) {
        printf("Usage:\n");
        printf("  barf -v                         Version\n");
        printf("  barf file.ba                    Load and run file\n");
        printf("  barf file.ba -- [args...]       Load and run file with arguments\n");
        printf("  barf -d file.ba                 Dump BARF information\n");
        printf("  barf -c -o file.ba <ofiles...>  Convert/combine COFF/ELF/BA to BA\n");
        printf("  barf -c -o file.o <bfiles...>   Convert BARF to ELF\n");
        return 0;
    }

    if (print_version) {
        printf("version: %s\n", BARF_VERSION);
        printf("commit: 0\n");
        return 0;
    }

    if (dump) {
        BarfObject* object = barf_parse_header_from_file(input_files[0]);
        if (!object)
            return 1;

        barf_dump(object);

        barf_free_object(object);
        return 0;
    }

    if (combine) {
        bool res = barf_combine_to_artifact(input_files_len, input_files, output_file);
        if (!res) {
            return 1;
        }
        // return 0;
    }
    bool res;
    if (user_arg_index != -1) {
        // @TODO Do we load and relocate all input files that were passed in or
        //   do user have to merge them into one first?
        res = barf_load_file(input_files[0], argc - user_arg_index, (const char**)argv + user_arg_index);
    } else {
        res = barf_load_file(input_files[0], 0, NULL);
    }
    if (!res)
        return 1;

    return 0;
}
