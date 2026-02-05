




System has Resources, Capabilities, Files, Binary Artifacts, Execution Domain (process), threads.

Resources: devices, memory ranges, files
Capabilities: Max memory, max file storage, CPU usage?
Files: data on storage devices (NVMe, SDD, HDD)
Binary Artifacts: Libraries, programs and code.
Execution domain: An isolation of one or more programs with resources and capabilities.
Threads: Responsible for executing code in the domain. Can be many for parallelism, just one or none if it's idle or stopped.


Programs are isolated. User gives programs access to files and directories. User declares the max and max file storage limits. Program declares it's minimum requirements.


Where are the program's capabilities stored, can the program lie about them?
What is a program. Is a binary artifact a program? Is a program a directory of images, settings, config files, libraries and main binary artifact?



We want a program to 
