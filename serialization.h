#pragma once

#ifndef __APP_SRZ_H_
#define __APP_SRZ_H_

#include <fstream>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <cmath>

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
    inline void write_bytes(fstream& fout, const T& x)
    {
        fout.write(TO_BYTES(&(x)), sizeof(T));
    }

    template <typename T, size_t N>
    inline void write_bytes(fstream& fout, const T (&x)[N])
    {
        fout.write(TO_BYTES(x), sizeof(T) * N);
    }

// #define WRITE_ARRAY(fout, arr) fout.write(TO_BYTES(arr), sizeof(arr))
// #define WRITE_OPAQUE(fout, x) fout.write(TO_BYTES(&(x)), sizeof(x))

    inline void WriteItem(fstream& fout, const InventoryItem& item)
    {
        // WRITE_OPAQUE(fout, item);
        // write_bytes(fout, item);
        write_bytes(fout, item.item_id);
        write_bytes(fout, item.meta);
        write_bytes(fout, item.item_count);
        write_bytes(fout, item.assigned_count);
        write_bytes(fout, item.active);
    }

    inline void WriteSingleMember(fstream& fout, const Member* mem)
    {
        // WRITE_ARRAY(fout, mem->name);
        // WRITE_OPAQUE(fout, mem->borrow_count);
        write_bytes(fout, mem->name);
        write_bytes(fout, mem->borrow_count);
    }

    template<typename T = void>
    void WriteMembers(fstream& fout, const InventoryItem& item)
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
    // void WriteToFile(const Inventory& inv)
    void WriteToFile(fstream& fout, const Inventory& inv)
    {
        cout << "Writing" << endl;

        fout.clear();
        fout.seekp(0, ios::beg);

        // std::ofstream fout(MAIN_FILE_NAME, ios::binary | ios::ate);
    
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
    inline bool read_bytes(fstream& fin, T& x)
    {
        fin.read(TO_BYTES_R(&(x)), sizeof(T));
        return !fin.fail();
    }

    template <typename T, size_t N>
    inline bool read_bytes(fstream& fin, T (&x)[N])
    {
        fin.read(TO_BYTES_R(x), sizeof(T) * N);
        return !fin.fail();
    }

    inline bool check_bytes(fstream& fin)
    {
        using bytes_t = std::remove_cv<decltype(MAGIC_BYTES)>::type;
        bytes_t file_bytes;
        fin.read(TO_BYTES_R(file_bytes), sizeof(file_bytes));

        if (fin.fail())
            return false;

        static constexpr size_t len = std::extent<bytes_t, 0>::value;
        return memcmp(file_bytes, MAGIC_BYTES, len) == 0;
    }

    using DataFile = fstream*; 

    inline DataFile OpenFile()
    {
        auto f = new std::fstream();
        f->open(MAIN_FILE_NAME, ios::binary | ios::out | ios::app);
        f->close();

        f->open(MAIN_FILE_NAME, ios::binary | ios::in | ios::out | ios::ate);

        f->seekp(0, ios::beg);
        f->seekg(0, ios::beg);

        if (!(*f))
        {
            delete f;
            return nullptr;
        }

        return f;
    }

    inline bool IsFileValid(DataFile f)
    {
        return check_bytes(*f);
    }

    inline void CloseFile(DataFile f)
    {
        delete f;
    }

    inline item_count_t ReadCount(DataFile f, bool seek = true)
    {
        if (seek)
            f->seekg(sizeof(MAGIC_BYTES), ios::beg);

        item_count_t count;
        if (read_bytes(*f, count))
            return count;

        return -1;
    }

    template<typename T = void>
    bool ReadItem(DataFile f, InventoryItem& item)
    {
        bool res = 1;

        res &= read_bytes(*f, item.item_id);
        res &= read_bytes(*f, item.meta);
        res &= read_bytes(*f, item.item_count);
        res &= read_bytes(*f, item.assigned_count);
        res &= read_bytes(*f, item.active);

        return res;
    }

    inline bool ReadItem(DataFile f, InventoryItem& item, size_t index)
    {
        f->seekg(sizeof(MAGIC_BYTES) + sizeof(item_count_t), ios::beg);
        f->seekg(sizeof(InventoryItem) * index, ios::cur);
        return ReadItem(f, item);
    }

    template<typename T = void>
    bool ReadFromFile(DataFile f, Inventory& inv)
    {

#define ERROR() \
    do { \
        cout << "[WARN] Invalid or corrupted file -- Skipping" << endl; \
        return false; \
    } while (false)

        cout << "Reading\n" << endl;

        auto count = ReadCount(f, false); 
        if (count == -1)
            ERROR();

        cout << count << endl;

        InvUtil_AllocateFor(inv, pow(2, ceil(log2(count))));

        static InventoryItem item;
        for (int i = 0; i < count; ++i)
        {
            if (!ReadItem(f, item))
                ERROR();

            inv.items[i] = item;
        }

        inv.count = count;

        return false;
    }

} // namespace Serialization

#endif