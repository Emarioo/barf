/*
    Command Line Interface for BARF runtime loader and BARF file dump.
*/

#include "barf/types.h"
#include "barf/barf.h"

#define BARF_VERSION "0.0.1-dev"

int main(int argc, char** argv) {
    const char* input_file = NULL;
    bool dump = false;
    bool print_help = false;
    bool print_version = false;
    bool combine = false;

    const char** extra_files = malloc(50 * sizeof(char*));
    int extra_files_len = 0;

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
        } else {
            if (input_file) {
                if (combine) {
                    extra_files[extra_files_len++] = arg;
                    continue;
                }
                fprintf(stderr, "ERROR barf: Input file already provided, remove '%s'\n", arg);
                return 1;
            }
            input_file = arg;
        }
    }
    if (print_help) {
        printf("Usage:\n");
        printf("  barf -v                         Version\n");
        printf("  barf file.barf               Load and run file\n");
        printf("  barf file.barf -- [args...]  Load and run file with arguments\n");
        printf("  barf -d file.barf            Dump BARF information\n");
        printf("  barf -tb file.barf <ofiles...>  Convert ELF to BARF\n");
        printf("  barf -te file.o <bfiles...>  Convert BARF to ELF\n");
        return 0;
    }

    if (print_version) {
        printf("version: %s\n", BARF_VERSION);
        printf("commit: 0\n");
        return 0;
    }

    if (dump) {
        BarfObject* object = barf_parse_header_from_file(input_file);
        if (!object)
            return 1;

        barf_dump(object);

        barf_free_object(object);
        return 0;
    }

    if (combine) {
        // @TODO For loop all extra files and turn them to BARF objects
        //     Combine objects into one.
        bool res = barf_convert_from_elf_file(extra_files[0], input_file);
        if (!res) {
            return 1;
        }
        // return 0;
    }

    bool res = barf_load_file(input_file);
    if (!res)
        return 1;

    return 0;
}
