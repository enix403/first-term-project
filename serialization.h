#pragma once

#ifndef __APP_SRZ_H_
#define __APP_SRZ_H_

#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstring>

#include "repr.h"

namespace Serialization
{
    using namespace std;

    static constexpr const char* MAIN_FILE_NAME = "inventory_data.bin";

    static constexpr uint8_t MAGIC_BYTES[16] = {
        0x49, 0x4E, 0x56, 0x4D, 0x47, 0x4D, 0x54, 0x53,
        0x59, 0x53, 0x54, 0x45, 0x4D, 0x41, 0x42, 0x43,
    }; // hexdump of "INVMGMTSYSTEMABC"


    /* --------------------------------------------------------------- */
    /* --------------------------- WRITING --------------------------- */
    /* --------------------------------------------------------------- */

#define TO_BYTES(ptr) reinterpret_cast<const char*>(ptr)
#define TO_BYTES_R(ptr) reinterpret_cast<char*>(ptr)

    template <typename T>
    inline void write_bytes(ofstream& fout, const T& x)
    {
        fout.write(TO_BYTES(&(x)), sizeof(x));
    }

    template <typename T, size_t N>
    inline void write_bytes(ofstream& fout, const T (&x)[N])
    {
        fout.write(TO_BYTES(x), sizeof(x));
    }

// #define WRITE_ARRAY(fout, arr) fout.write(TO_BYTES(arr), sizeof(arr))
// #define WRITE_OPAQUE(fout, x) fout.write(TO_BYTES(&(x)), sizeof(x))

    inline void WriteItem(ofstream& fout, const InventoryItem& item)
    {
        // WRITE_OPAQUE(fout, item);
        // write_bytes(fout, item);
        write_bytes(fout, item.item_id);
        write_bytes(fout, item.meta);
        write_bytes(fout, item.item_count);
        write_bytes(fout, item.assigned_count);
        write_bytes(fout, item.active);
    }

    inline void WriteSingleMember(ofstream& fout, const Member* mem)
    {
        // WRITE_ARRAY(fout, mem->name);
        // WRITE_OPAQUE(fout, mem->borrow_count);
        write_bytes(fout, mem->name);
        write_bytes(fout, mem->borrow_count);
    }

    template<typename T = void>
    void WriteMembers(ofstream& fout, const InventoryItem& item)
    {
        auto count_pos = fout.tellp();

        uint32_t count = 0;
        // WRITE_OPAQUE(fout, count);
        write_bytes(fout, count);

        auto mem = item.allocated_to;
        while (mem != nullptr)
        {
            WriteSingleMember(fout, mem);
            ++count;
            mem = mem->next;
        }

        auto next_write_spos = fout.tellp();

        fout.seekp(count_pos);
        // WRITE_OPAQUE(fout, count);
        write_bytes(fout, count);

        fout.seekp(next_write_spos);
    }

    template<typename T = void>
    void WriteToFile(const Inventory& inv)
    {
        cout << "Writing" << endl;
        std::ofstream fout(MAIN_FILE_NAME, ios::binary | ios::ate);
    
        // WRITE_ARRAY(fout, MAGIC_BYTES);
        // WRITE_OPAQUE(fout, inv.count);

        write_bytes(fout, MAGIC_BYTES);
        write_bytes(fout, inv.count);

        for (int i = 0; i < inv.count; ++i)
            WriteItem(fout, inv.items[i]);

        for (int i = 0; i < inv.count; ++i)
            WriteMembers(fout, inv.items[i]);

        fout.close();
    }

    /* --------------------------------------------------------------- */
    /* --------------------------- READING --------------------------- */
    /* --------------------------------------------------------------- */

    template <typename T>
    inline void read_bytes(ifstream& fin, T& x)
    {
        fin.read(TO_BYTES_R(&(x)), sizeof(x));
    }

    template <typename T, size_t N>
    inline void read_bytes(ifstream& fin, T (&x)[N])
    {
        fin.read(TO_BYTES_R(x), sizeof(x));
    }

    inline bool check_bytes(ifstream& fin)
    {
        using bytes_t = std::remove_cv<decltype(MAGIC_BYTES)>::type;
        bytes_t file_bytes;
        fin.read(TO_BYTES_R(file_bytes), sizeof(file_bytes));
        static constexpr size_t len = std::extent<bytes_t, 0>::value;
        return memcmp(file_bytes, MAGIC_BYTES, len) == 0;
    }

    template<typename T = void>
    bool ReadFromFile(Inventory& inv)
    {
        cout << "Reading\n" << endl;
        std::ifstream fin(MAIN_FILE_NAME, ios::binary);

        if (!fin)
        {
            return false;
        }

        if (!check_bytes(fin))
        {
            cout << "Invalid file (magic bytes)" << endl;
            return false;
        }

        return true;
    }

} // namespace Serialization

#endif