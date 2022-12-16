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

    using DataFile = fstream*;

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

    inline void WriteItem(DataFile f, const InventoryItem& item)
    {
        write_bytes(*f, item.item_id);
        write_bytes(*f, item.meta);
        write_bytes(*f, item.item_count);
        write_bytes(*f, item.assigned_count);
        write_bytes(*f, item.active);
    }

    inline void WriteSingleMember(fstream& fout, const Member* mem)
    {
        write_bytes(fout, mem->name);
        write_bytes(fout, mem->borrow_count);
    }

    template<typename T = void>
    void WriteMembers(DataFile f, const InventoryItem& item)
    {
        auto count_pos = f->tellp();

        uint32_t count = 0;
        write_bytes(*f, count);

        auto mem = item.allocated_to;
        while (mem != nullptr)
        {
            WriteSingleMember(*f, mem);
            ++count;
            mem = mem->next;
        }

        auto next_write_spos = f->tellp();

        f->seekp(count_pos);
        write_bytes(*f, count);

        f->seekp(next_write_spos);
    }

    template<typename T = void>
    void WriteToFile(DataFile f, const Inventory& inv)
    {
        f->clear();
        f->seekp(0, ios::beg);

        write_bytes(*f, MAGIC_BYTES);
        write_bytes(*f, inv.count);

        for (int i = 0; i < inv.count; ++i)
            WriteItem(f, inv.items[i]);

        for (int i = 0; i < inv.count; ++i)
            WriteMembers(f, inv.items[i]);
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

    inline DataFile OpenFile()
    {
        /* Create the file if it does not exist */
        { std::ofstream(MAIN_FILE_NAME, ios::binary | ios::out | ios::app); }

        auto f = new std::fstream();
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

    inline item_count_t ReadCount(DataFile f)
    {
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

    inline bool ReadSingleMember(DataFile f, Member* mem)
    {
        bool res = 1;

        res &= read_bytes(*f, mem->name);
        res &= read_bytes(*f, mem->borrow_count);

        return res;
    }


    inline bool ReadMembers(DataFile f, InventoryItem& item)
    {
        uint32_t count;
        if (!read_bytes(*f, count))
            return false;

        Member* tail = nullptr;
        Member* head = nullptr;

        for (int i = 0; i < count; ++i)
        {
            Member* current = CreateMember("");
            if (!ReadSingleMember(f, current))
                return false;

            if (i == 0)
                head = current;

            current->prev = tail;
            current->next = nullptr;

            if (tail != nullptr)
                tail->next = current;

            tail = current;
        }

        item.allocated_to = head;

        return true;
    }

    template<typename T = void>
    bool ReadFromFile(DataFile f, Inventory& inv)
    {
        auto count = ReadCount(f); 
        if (count == -1)
            return false;

        InvUtil_AllocateFor(inv, pow(2, ceil(log2(count))));

        static InventoryItem item;
        for (int i = 0; i < count; ++i)
        {
            if (!ReadItem(f, item))
                return false;

            inv.items[i] = item;
        }

        for (int i = 0; i < count; ++i)
            if (!ReadMembers(f, inv.items[i]))
                return false;

        inv.count = count;

        return true;
    }

} // namespace Serialization

#endif