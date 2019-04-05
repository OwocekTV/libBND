#include "libBND.hpp"
#include "CRC.h"
#include <algorithm>

using namespace std;

void copyFile(string src, string dst)
{
    ifstream i(src, ios::binary | ios::in);
    std::string f((std::istreambuf_iterator<char>(i)),
    std::istreambuf_iterator<char>());
    i.close();

    ofstream o(dst, ios::binary | ios::trunc);
    o.write(f.c_str(),f.size());
    o.close();
}

libBND::libBND()
{

}


void libBND::setDebug(bool state)
{
    cout << "Debug output set to " << state << endl;
    debug = state;
}

void libBND::debugOutput(int type, std::string message)
{
    if(debug)
    {
        if(type == 0)
        {
            cout << "[INFO] " << message << endl;
        }

        if(type == 1)
        {
            cout << "[WARNING] " << message << endl;
        }

        if(type == 2)
        {
            cout << "[ERROR] " << message << endl;
        }
    }
}

void libBND::debugListAllFiles()
{
    if(debug)
    {
        for(int i=0; i<v_file_entries.size(); i++)
        {
            cout << "[INFO] [ENTRY " << i << "]: " << v_file_entries[i].full_filename << " level: " << int(v_file_entries[i].folder_level) << endl;
        }
    }
}

int libBND::load(std::string file)
{
    og_dictionary = file;
    dictionary_name = og_dictionary;
    og_filename = file;

    std::ifstream sz(file, ios::ate | ios::binary);
    fsize = sz.tellg();
    sz.close();

    ifstream bnd(file, ios::binary);

    bnd.seekg(0x0);
    bnd.read(reinterpret_cast<char*>(&head), sizeof(uint32_t));

    debugOutput(0,string("BND header: ")+to_string(head));

    if(head == 4476482)
    {
        bnd.seekg(0x4);
        bnd.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));

        debugOutput(0,string("BND version: ")+to_string(version));

        bnd.seekg(0x8);
        bnd.read(reinterpret_cast<char*>(&val3), sizeof(uint32_t));

        debugOutput(0,string("BND val3: ")+to_string(val3));

        bnd.seekg(0xC);
        bnd.read(reinterpret_cast<char*>(&val4), sizeof(uint32_t));

        debugOutput(0,string("BND val4: ")+to_string(val4));

        if(version == 5)
        {
            debugOutput(0,string("Loading BND archive version 5 (regular BND)"));

            bnd.seekg(0x10);
            bnd.read(reinterpret_cast<char*>(&info_off), sizeof(uint32_t));

            debugOutput(0,string("Info offset: ")+to_string(info_off));

            bnd.seekg(0x14);
            bnd.read(reinterpret_cast<char*>(&file_off), sizeof(uint32_t));

            debugOutput(0,string("File offset: ")+to_string(file_off));

            bnd.seekg(0x20);
            bnd.read(reinterpret_cast<char*>(&file_count), sizeof(uint32_t));

            debugOutput(0,string("Files detected: ")+to_string(file_count));

            bnd.seekg(0x24);
            bnd.read(reinterpret_cast<char*>(&entry_count), sizeof(uint32_t));

            debugOutput(0,string("Entries detected: ")+to_string(entry_count));

            ///Read filename dictionary (irrevelant in Patapon BND reading)
            bool name_read = true;
            int name_off = info_off;

            vector<string> current_folder;

            while(name_read)
            {
                debugOutput(0,string(" Reading new file name entry at ")+to_string(name_off));

                FileEntry tmp_entry;

                bnd.seekg(name_off);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.folder_level), sizeof(int8_t));

                debugOutput(0,string("Folder level: ")+to_string(tmp_entry.folder_level));

                bnd.seekg(name_off+0x1);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.prev_size), sizeof(int8_t));

                debugOutput(0,string("Previous entry size: ")+to_string(tmp_entry.prev_size));

                bnd.seekg(name_off+0x2);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.cur_size), sizeof(uint8_t));

                debugOutput(0,string("Current entry size: ")+to_string(tmp_entry.cur_size));

                bnd.seekg(name_off+0x3);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.crc_offset), sizeof(uint32_t));

                debugOutput(0,string("Offset to CRC dictionary: ")+to_string(tmp_entry.crc_offset));

                char tmp = 0xFF;
                int i = 0;

                while(tmp != 0x0)
                {
                    bnd.seekg(name_off+7+i);
                    tmp = bnd.get();

                    if(tmp != 0x0)
                    tmp_entry.filename += tmp;

                    i++;
                }

                debugOutput(0,string("File name: ")+tmp_entry.filename);

                ///Handling directory names
                if(tmp_entry.folder_level > 0) ///>0 for directories, <0 for files
                {
                    while(tmp_entry.folder_level <= current_folder.size())
                    {
                        current_folder.pop_back();
                    }

                    if(tmp_entry.folder_level >= current_folder.size())
                    {
                        current_folder.push_back(tmp_entry.filename);
                    }
                }

                string full_dir = "";

                for(int i=0; i<current_folder.size(); i++)
                {
                    full_dir += current_folder[i];
                }


                debugOutput(0,string("Current directory: ")+full_dir);

                string full_file_name = "";

                if(tmp_entry.folder_level < 0)
                {
                    tmp_entry.full_filename = full_dir + tmp_entry.filename;
                }

                if(tmp_entry.folder_level > 0)
                {
                    tmp_entry.full_filename = full_dir;
                }

                debugOutput(0,string("Full file name: ")+tmp_entry.full_filename);

                if(tmp_entry.cur_size == 0xFF)
                {
                    name_read = false;
                }

                name_off += tmp_entry.cur_size;


                bnd.seekg(tmp_entry.crc_offset);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.CRC), sizeof(uint32_t));

                debugOutput(0,string("File CRC: ")+to_string(tmp_entry.CRC));

                bnd.seekg(tmp_entry.crc_offset+4);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.info_offset), sizeof(uint32_t));

                debugOutput(0,string("File info offset: ")+to_string(tmp_entry.info_offset));

                bnd.seekg(tmp_entry.crc_offset+8);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.file_offset), sizeof(uint32_t));

                debugOutput(0,string("File data offset: ")+to_string(tmp_entry.file_offset));

                bnd.seekg(tmp_entry.crc_offset+12);
                bnd.read(reinterpret_cast<char*>(&tmp_entry.file_size), sizeof(uint32_t));

                debugOutput(0,string("File data size: ")+to_string(tmp_entry.file_size));

                v_file_entries.push_back(tmp_entry);
            }

            zero_bytes = (entry_count - v_file_entries.size()) * 16;
            debugOutput(0,string("Detected zero bytes: ")+to_string(zero_bytes));

            debugListAllFiles();
        }
    }

    bnd.close();

    return 1;
}

int libBND::setDataSource(string file)
{
    og_filename = file;
    external = true;
}

int libBND::setDictionaryName(string file)
{
    string p = og_dictionary.substr(0,og_dictionary.find_first_of("\\/")+1);
    string n = p+file;

    dictionary_name = n;
}

int libBND::addToRepackQueue(int id, std::string replace_file)
{
    std::ifstream sz(og_filename, ios::ate | ios::binary);
    int main_filesize = sz.tellg();
    sz.close();

    cout << "Size: " << main_filesize << endl;

    repackQueue newRepack;
    newRepack.fileOffset = v_file_entries[id].file_offset;

    struct tmpstruct
    {
        uint32_t offset;
        int id;
    };

    cout << "Convert database to find real size" << endl;
    vector<tmpstruct> tmp;
    for(int i=0; i<v_file_entries.size(); i++)
    {
        tmpstruct a;
        a.offset = v_file_entries[i].file_offset;
        a.id = i;

        //cout << "Transferring " << hex << a.offset << " " << a.id << dec << endl;

        tmp.push_back(a);
    }

    cout << "TMP struct size: " << tmp.size() << endl;

    cout << "Sort the database based on file offsets" << endl;
    std::sort(tmp.begin(), tmp.end(), [](auto const &a, auto const &b) { return a.offset < b.offset; });

    ///replace with some finding method
    for(int i=0; i<tmp.size(); i++)
    {
        //cout << tmp[i].id << " vs " << id << " " << dec << tmp[i].offset << " " << main_filesize << hex << endl;

        if(tmp[i].id == id)
        {
            if(i != tmp.size()-1)
            {
                newRepack.fileOldSize = tmp[i+1].offset - tmp[i].offset;
                cout << "Full size detected 1: " << newRepack.fileOldSize << endl;
            }
            else
            {
                newRepack.fileOldSize = main_filesize - tmp[i].offset;
                cout << "Full size detected 2: " << newRepack.fileOldSize << endl;
                cout << hex << main_filesize << " " << tmp[i].offset << dec << endl;
            }

            break;
        }
    }

    ///calculate the file size based on the one entry higher to include the zeroes into the file!!!!
    /*
    if(id != v_file_entries.size()-1)
    {
        newRepack.fileOldSize = v_file_entries[id+1].file_offset - v_file_entries[id];
    }
    else
    {

    }*/

    newRepack.filePath = replace_file;
    newRepack.id = id;

    cout << "Added to repack queue: " << hex;
    cout << newRepack.fileOffset << " ";
    cout << newRepack.fileOldSize << " ";
    cout << newRepack.filePath << " ";
    cout << dec << newRepack.id << endl;

    v_repacks.push_back(newRepack);
}

int libBND::repack()
{
    cout << "Repacking all files from queue..." << endl;

    ///Sort all repacks in order by offset
    std::sort(v_repacks.begin(), v_repacks.end(), [](auto const &a, auto const &b) { return a.fileOffset < b.fileOffset; });

    ofstream r("REPACK_TMP.BND", ios::trunc);
    r.close();

    std::ifstream sz2(og_filename, ios::ate | ios::binary);
    int main_filesize = sz2.tellg();
    sz2.close();

    int offset = 0;
    int chunk_size = 1024*1024;

    for(int i=0; i<v_repacks.size(); i++)
    {
        cout << "Repacking file " << i+1 << "/" << v_repacks.size() << endl;

        std::ifstream sz(v_repacks[i].filePath, ios::ate | ios::binary);
        int new_filesize = sz.tellg();
        sz.close();

        int add_zero = 0;
        while((new_filesize+add_zero)%2048 > 0)
        {
            add_zero++;
        }

        cout << add_zero << " zeroes added!" << endl;

        int diff = new_filesize - v_repacks[i].fileOldSize + add_zero;

        int destination = v_repacks[i].fileOffset;
        int bytes_left = destination - offset;

        cout << hex << "offset: " << offset << " destination: " << destination << " bytes_left: " << bytes_left << " new filesize: " << new_filesize << " diff: " << dec << diff << endl;

        while(bytes_left > chunk_size)
        {
            ifstream bnd(og_filename, ios::binary);
            char data[chunk_size];
            bnd.seekg(offset);
            bnd.read(data,chunk_size);
            bnd.close();

            ofstream out("REPACK_TMP.BND",ios::binary | ios::app);
            out.write(data,chunk_size);
            out.close();

            offset += chunk_size;
            bytes_left -= chunk_size;

            //cout << "bytes: " << std::hex << bytes_left << " offset: " << offset << endl;
        }

        ifstream bnd(og_filename, ios::binary);
        char data[bytes_left];
        bnd.seekg(offset);
        bnd.read(data,bytes_left);
        bnd.close();

        ofstream out("REPACK_TMP.BND",ios::binary | ios::app);
        out.write(data,bytes_left);
        out.close();

        offset += bytes_left;
        bytes_left -= bytes_left;

        cout << "Adding the new file... size: " << hex << (new_filesize+add_zero) << " oldEnd: " << offset + v_repacks[i].fileOldSize << " newEnd: " << offset + (new_filesize+add_zero) << dec << endl;

        ifstream repack_file(v_repacks[i].filePath, ios::binary | ios::in);

        std::string new_file_char((std::istreambuf_iterator<char>(repack_file)),
        std::istreambuf_iterator<char>());

        repack_file.close();

        ofstream out2("REPACK_TMP.BND", ios::binary | ios::app);
        out2.write(new_file_char.c_str(),new_filesize);

        for(int z=0; z<add_zero; z++)
        out2.put(0x0);

        out2.close();

        offset += v_repacks[i].fileOldSize;

        cout << "Adding shift to shiftQueue..." << endl;

        shiftQueue newShift;
        newShift.sizeBiggerThan = v_repacks[i].fileOffset;
        newShift.amountToShift = diff;
        v_shifts.push_back(newShift);

        cout << "Shift files on more than " << hex << v_repacks[i].fileOffset << " by " << diff << dec << endl;

        v_file_entries[v_repacks[i].id].file_size = new_filesize;
    }

    cout << "Save the rest of the file..." << endl;

    int destination = main_filesize;
    int bytes_left = destination - offset;

    cout << hex << "offset: " << offset << " destination: " << destination << " bytes_left: " << bytes_left << dec << endl;

    while(bytes_left > chunk_size)
    {
        ifstream bnd(og_filename, ios::binary);
        char data[chunk_size];
        bnd.seekg(offset);
        bnd.read(data,chunk_size);
        bnd.close();

        ofstream out("REPACK_TMP.BND",ios::binary | ios::app);
        out.write(data,chunk_size);
        out.close();

        offset += chunk_size;
        bytes_left -= chunk_size;

        //cout << "bytes: " << std::hex << bytes_left << " offset: " << offset << endl;
    }

    ifstream bnd(og_filename, ios::binary);
    char data[bytes_left];
    bnd.seekg(offset);
    bnd.read(data,bytes_left);
    bnd.close();

    ofstream out("REPACK_TMP.BND",ios::binary | ios::app);
    out.write(data,bytes_left);
    out.close();

    cout << "Finished repacking file" << endl;
}

int libBND::save(std::string file)
{
    repack();

    std::ifstream sz("REPACK_TMP.BND", ios::ate | ios::binary);
    int og_filesize = sz.tellg();
    sz.close();

    cout << "og_filesize: " << og_filesize << endl;

    ///Rebuild dictionary
    ofstream bnd("tmp.bnd", ios::binary | ios::trunc);

    for(int i=0; i<v_removeFiles.size(); i++)
    {
        cout << "Removing file with id " << v_removeFiles[i] << "(" << v_file_entries[v_removeFiles[i]].filename << ")" << endl;

        for(int a=0; a<v_removeFiles.size(); a++)
        {
            ///remove file with id 5
            ///files with id 6, 7, etc... shift -1
            //cout << "Checking " << v_removeFiles[a] << " > " << v_removeFiles[i] << endl;

            if(v_removeFiles[a] > v_removeFiles[i])
            v_removeFiles[a]--;
        }

        v_file_entries.erase(v_file_entries.begin() + v_removeFiles[i]);
    }

    /**
    Need to put all CRCs in the order, point info offset stuff properly on both sides
    */

    struct CRCEntry
    {
        uint32_t crc;
        uint32_t info;
        uint32_t f_data;
        uint32_t f_size;
        int id;
    };

    struct TMPFileEntry
    {
        int8_t folder_level;
        uint8_t prev_size;
        uint8_t cur_size;
        uint32_t crc_offset;
        std::string filename;
        std::string full_filename;
    };

    vector<CRCEntry> tmp_entries;
    vector<TMPFileEntry> tmp_file_entries;

    uint32_t start_offset = 0x28 + zero_bytes + (v_file_entries.size() * 16);
    uint32_t filename_size = 0x0;
    ///cout << "Starting offset: " << std::hex << start_offset << endl;

    file_count = 0;
    entry_count = v_file_entries.size() + (zero_bytes / 16);

    uint8_t p_size = 255;
    uint8_t c_size = 255;

    for(int i=0; i<v_file_entries.size(); i++)
    {
        CRCEntry tmp_entry;
        TMPFileEntry tmp_file_entry;

        tmp_entry.crc = CRC::Calculate(v_file_entries[i].full_filename.c_str(), v_file_entries[i].full_filename.size(), CRC::CRC_32());
        tmp_entry.info = start_offset;
        tmp_entry.f_data = v_file_entries[i].file_offset;
        tmp_entry.f_size = v_file_entries[i].file_size;
        tmp_entry.id = i;

        start_offset += 0x8 + v_file_entries[i].filename.size();
        filename_size += 0x8 + v_file_entries[i].filename.size();

        if(c_size != 255)
        p_size = c_size;

        c_size = 0x8 + v_file_entries[i].filename.size();

        if(i == v_file_entries.size()-1)
        c_size = 255;

        ///cout << "Start offset: " << std::hex << start_offset << endl;

        tmp_file_entry.folder_level = v_file_entries[i].folder_level;
        tmp_file_entry.prev_size = p_size;
        tmp_file_entry.cur_size = c_size;
        tmp_file_entry.crc_offset = 0;
        tmp_file_entry.filename = v_file_entries[i].filename;
        tmp_file_entry.full_filename = v_file_entries[i].full_filename;

        if(tmp_file_entry.folder_level < 0)
        file_count++;

        tmp_entries.push_back(tmp_entry);
        tmp_file_entries.push_back(tmp_file_entry);
    }

    std::sort(tmp_entries.begin(), tmp_entries.end(), [](auto const &a, auto const &b) { return a.crc < b.crc; });

    uint32_t zero = 0x0;
    int dictionary_size = 0x28 + zero_bytes + (v_file_entries.size() * 16) + filename_size;

    while((dictionary_size % 2048) > 0)
    {
        dictionary_size++;
    }

    int dictionary_difference = dictionary_size - file_off;
    int old_file_off = file_off;

    cout << "File offset: " << file_off << endl;

    info_off = 0x28 + zero_bytes + (v_file_entries.size() * 16);
    file_off = dictionary_size;

    cout << "Dictionary size: " << dictionary_size << endl;
    cout << "Dictionary difference: " << dictionary_difference << endl;

    ///time to save it!
    bnd.write((char*)&head, sizeof(uint32_t));
    bnd.write((char*)&version, sizeof(uint32_t));
    bnd.write((char*)&val3, sizeof(uint32_t));
    bnd.write((char*)&val4, sizeof(uint32_t));

    bnd.write((char*)&info_off, sizeof(uint32_t));
    bnd.write((char*)&file_off, sizeof(uint32_t));

    bnd.write((char*)&zero, sizeof(uint32_t));
    bnd.write((char*)&zero, sizeof(uint32_t));

    bnd.write((char*)&file_count, sizeof(uint32_t));
    bnd.write((char*)&entry_count, sizeof(uint32_t));

    for(int i=0; i<zero_bytes; i++)
    {
        bnd.put(0x0);
    }

    for(int i=0; i<tmp_entries.size(); i++)
    {
        ///cout << "CRC entry [" << i << "]: " << std::hex << tmp_entries[i].crc << " " << tmp_entries[i].info << " " << tmp_entries[i].f_data << " " << std::dec << tmp_entries[i].f_size << endl;
        tmp_file_entries[tmp_entries[i].id].crc_offset = 0x28 + zero_bytes + (i*16);

        ///here all the shifts happen
        if(tmp_entries[i].f_data != 0)
        {
            //cout << "Shifting " << std::hex << tmp_entries[i].f_data << " to ";
            tmp_entries[i].f_data += dictionary_difference;
            //cout << tmp_entries[i].f_data << std::dec << endl;
        }

        for(int v=v_shifts.size()-1; v>=0; v--)
        {
            if(tmp_entries[i].f_data > v_shifts[v].sizeBiggerThan)
            {
                //cout << "Shift [" << v << "]: " << hex << tmp_entries[i].f_data << " to ";
                tmp_entries[i].f_data += v_shifts[v].amountToShift;
                //cout << tmp_entries[i].f_data << " (" << dec << v_shifts[v].amountToShift << hex << " if >" << v_shifts[v].sizeBiggerThan << dec << endl;
            }
        }

        bnd.write((char*)&tmp_entries[i].crc, sizeof(uint32_t));
        bnd.write((char*)&tmp_entries[i].info, sizeof(uint32_t));
        bnd.write((char*)&tmp_entries[i].f_data, sizeof(uint32_t));
        bnd.write((char*)&tmp_entries[i].f_size, sizeof(uint32_t));
    }

    for(int i=0; i<tmp_file_entries.size(); i++)
    {
        ///cout << "FILE entry [" << i << "]: " << std::hex << int(tmp_file_entries[i].folder_level) << " " << int(tmp_file_entries[i].prev_size) << " " << int(tmp_file_entries[i].cur_size) << " " << int(tmp_file_entries[i].crc_offset) << " " << tmp_file_entries[i].filename << std::dec << endl;

        bnd.write((char*)&tmp_file_entries[i].folder_level, sizeof(int8_t));
        bnd.write((char*)&tmp_file_entries[i].prev_size, sizeof(uint8_t));
        bnd.write((char*)&tmp_file_entries[i].cur_size, sizeof(uint8_t));
        bnd.write((char*)&tmp_file_entries[i].crc_offset, sizeof(uint32_t));
        bnd << tmp_file_entries[i].filename;
        bnd.put(0x0);
    }

    while((bnd.tellp() % 2048) > 0)
    bnd.put(0x0);

    bnd.close();

    ///now put the rest of the file

    if(external)
    {
        copyFile("tmp.bnd",dictionary_name);

        ofstream t("tmp.bnd", ios::trunc);
        t.close();
    }

    cout << "packing up the dictionary" << endl;

    int bytes_left = og_filesize - old_file_off;
    int offset = old_file_off;
    int chunk_size = 1024*1024;

    if(external)
    {
        bytes_left += old_file_off;
        offset -= old_file_off;
    }

    //cout << "bytes: " << std::hex << bytes_left << " offset: " << offset << endl;

    while(bytes_left > chunk_size)
    {
        ifstream og("REPACK_TMP.BND", ios::binary);
        char data[chunk_size];
        og.seekg(offset);
        og.read(data,chunk_size);
        og.close();

        ofstream out("tmp.bnd",ios::binary | ios::app);
        out.write(data,chunk_size);
        out.close();

        offset += chunk_size;
        bytes_left -= chunk_size;

        //cout << "bytes: " << std::hex << bytes_left << " offset: " << offset << endl;
    }

    ifstream og("REPACK_TMP.BND", ios::binary);
    char data[bytes_left];
    og.seekg(offset);
    og.read(data,bytes_left);
    og.close();

    //cout << "bytes: " << std::hex << bytes_left << " offset: " << offset << endl;

    ofstream out("tmp.bnd", ios::binary | ios::app);
    out.write(data,bytes_left);
    out.close();

    for(int i=0; i<v_addFiles.size(); i++)
    {
        ifstream add_file(v_addFiles[i].filePath, ios::binary | ios::in);

        std::string new_file_char((std::istreambuf_iterator<char>(add_file)),
        std::istreambuf_iterator<char>());

        add_file.close();

        ofstream nf("tmp.bnd", ios::binary | ios::app);
        nf.write(new_file_char.c_str(),new_file_char.size());

        for(int z=0; z<v_addFiles[i].zeroes; z++)
        nf.put(0x0);

        nf.close();
    }

    copyFile("tmp.bnd", file);
    return 1;
}

int libBND::reloadNames()
{
    vector<string> current_folder;

    for(int i=0; i<v_file_entries.size(); i++)
    {
        ///Handling directory names
        if(v_file_entries[i].folder_level > 0) ///>0 for directories, <0 for files
        {
            while(v_file_entries[i].folder_level <= current_folder.size())
            {
                current_folder.pop_back();
            }

            if(v_file_entries[i].folder_level >= current_folder.size())
            {
                current_folder.push_back(v_file_entries[i].filename);
            }
        }

        string full_dir = "";

        for(int i=0; i<current_folder.size(); i++)
        {
            full_dir += current_folder[i];
        }

        debugOutput(0,string("Current directory: ")+full_dir);

        string full_file_name = "";

        if(v_file_entries[i].folder_level < 0)
        {
            v_file_entries[i].full_filename = full_dir + v_file_entries[i].filename;
        }

        if(v_file_entries[i].folder_level > 0)
        {
            v_file_entries[i].full_filename = full_dir;
        }

        debugOutput(0,string("Full file name: ")+v_file_entries[i].full_filename);
    }
}

int libBND::rename(int id, std::string new_name)
{
    v_file_entries[id].filename = new_name;
    reloadNames();
}

int libBND::addFile(int folder_id, std::string file)
{
    cout << "Adding " << file << " to the archive" << endl;
    cout << "Fsize: " << hex << fsize << dec << endl;

    std::ifstream sz(file, ios::ate | ios::binary);
    int new_filesize = sz.tellg();
    sz.close();

    string onlyName = file.substr(file.find_last_of("\\/")+1);

    FileEntry new_file;
    if(v_file_entries[folder_id].folder_level > 0)
    {
        new_file.folder_level = v_file_entries[folder_id].folder_level * (-1) - 1;
    }
    else
    {
        new_file.folder_level = v_file_entries[folder_id].folder_level;
    }
    new_file.file_offset = fsize;
    new_file.file_size = new_filesize;
    new_file.filename = onlyName;

    int f_level_target = new_file.folder_level * (-1) - 1;
    vector<string> folders;
    string folder;

    for(int i=folder_id; i>=0; i--)
    {
        if(v_file_entries[i].folder_level == f_level_target)
        {
            folder = v_file_entries[i].full_filename;
            break;
        }
    }

    new_file.full_filename = folder + onlyName;
    cout << "Full filename: " << new_file.full_filename << endl;

    int zeroes = 0;

    while((new_filesize+zeroes) % 2048 > 0)
    {
        zeroes++;
        cout << "Adding a zero for alignment" << endl;
    }

    fsize += new_filesize + zeroes;

    ///add to queue for save() function to append the file on the end
    addQueue add;
    add.filePath = file;
    add.zeroes = zeroes;
    v_addFiles.push_back(add);

    v_file_entries.insert(v_file_entries.begin() + folder_id + 1, new_file);
}

int libBND::removeFile(int id)
{
    ofstream z("ZERO_TMP.BND", ios::trunc);
    z.close();

    addToRepackQueue(id,"ZERO_TMP.BND");

    v_removeFiles.push_back(id);
}

int libBND::addFolder(int id, std::string name)
{
    cout << "Adding " << name << " to the archive" << endl;

    FileEntry new_file;
    if(v_file_entries[id].folder_level > 0)
    {
        new_file.folder_level = v_file_entries[id].folder_level + 1;
    }
    else
    {
        new_file.folder_level = v_file_entries[id].folder_level * (-1);
    }
    new_file.file_offset = 0;
    new_file.file_size = 0;
    new_file.filename = name;

    int f_level_target = new_file.folder_level - 1;
    vector<string> folders;
    string folder;

    for(int i=id; i>=0; i--)
    {
        if(v_file_entries[i].folder_level == f_level_target)
        {
            folder = v_file_entries[i].full_filename;
            break;
        }
    }

    new_file.full_filename = folder + name;
    cout << "Full filename: " << new_file.full_filename << endl;

    v_file_entries.insert(v_file_entries.begin() + id + 1, new_file);

    reloadNames();
}

int libBND::removeFolder(int id)
{
    cout << "Removing folder with ID " << id << "(" << v_file_entries[id].filename << ")" << endl;

    int dest = v_file_entries[id].folder_level;
    cout << "dest: " << dest << endl;

    v_removeFiles.push_back(id);

    for(int i=id+1; i<v_file_entries.size(); i++)
    {
        int f = v_file_entries[i].folder_level;
        int of = f;

        if(f < 0)
        f = f*(-1);

        cout << "f: " << f << " of: " << of << endl;

        if(f == dest+1)
        {
            if(of < 0) ///file
            {
                removeFile(i);
            }

            if(of > 0)
            {
                removeFolder(i);
            }
        }
        else
        {
            if(f <= dest)
            {
                cout << "Destination found!" << endl;
                break;
            }
        }
    }
}
