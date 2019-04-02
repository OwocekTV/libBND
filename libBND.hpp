#ifndef LIBBND_H
#define LIBBND_H

#include <iostream>
#include <vector>
#include <fstream>

class libBND
{
    public:
    uint32_t head,version,val3,val4,info_off,file_off,file_count,entry_count,zero_bytes;
    int fsize;

    struct FileEntry
    {
        uint32_t CRC;
        uint32_t info_offset;
        uint32_t file_offset;
        uint32_t file_size;

        int8_t folder_level;
        uint8_t prev_size;
        uint8_t cur_size;
        uint32_t crc_offset;
        std::string filename;
        std::string full_filename;
    };

    struct shiftQueue
    {
        uint32_t sizeBiggerThan;
        uint32_t amountToShift;
    };

    struct repackQueue
    {
        /**
        Important!
        Repacking tool should always calculate the next offset including the previous file.
        AKA. Repacking 2 files:
        first at 0x2000, diff: +0x250
        second at 0x3000, diff +0x100

        first file's difference should be counted into second's file offset, AKA:
        first at 0x2000, diff: +0x250
        second at 0x3250, diff +0x100

        or could it be automated?
        */

        uint32_t fileOffset;
        uint32_t fileOldSize;
        std::string filePath;
        int id;
    };

    struct addQueue
    {
        std::string filePath;
        int zeroes;
    };

    ///Dictionary
    std::vector<FileEntry> v_file_entries;

    ///Shifting
    std::vector<shiftQueue> v_shifts;

    ///Repacking
    std::vector<repackQueue> v_repacks;

    ///New files
    std::vector<addQueue> v_addFiles;

    bool debug = false;

    libBND();
    void setDebug(bool state);
    void debugOutput(int type, std::string message);
    void debugListAllFiles();

    int load(std::string file);
    int addToRepackQueue(int id, std::string replace_file);
    int repack(std::string file);
    int save(std::string file);
    int addFile(int id, std::string file);
    int removeFile(int id);
    int addFolder(int id, std::string file);
    int removeFolder(int id, int mode);
    int rename(int id, std::string new_name);

    std::vector<std::string> listNames();
    uint32_t getOffset(int ID);
};


#endif // LIBBND_H
