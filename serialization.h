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
        0x49, 0x4E, 0x56, 0x4D, 0x47, 0x4D, 0x54, 0x53, //
        0x59, 0x53, 0x54, 0x45, 0x4D, 0x41, 0x42, 0x43,
    }; // hexdump of "INVMGMTSYSTEMABC"

    using DataFile = fstream*;

    inline DataFile OpenFile()
    {
        /* Create the file if it does not exist */
        {
            std::ofstream(MAIN_FILE_NAME, ios::binary | ios::out | ios::app);
        }

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

    inline void CloseFile(DataFile f)
    {
        delete f;
    }


    /* To bytes immutable */
#define TO_BYTES_I(ptr) reinterpret_cast<const char*>(ptr)
    /* To bytes mutable */
#define TO_BYTES_M(ptr) reinterpret_cast<char*>(ptr)

    template<typename T, typename S = void>
    using enable_if_copyable = typename std::enable_if<std::is_trivially_copyable<T>::value, S>::type;

    template<typename T>
    inline enable_if_copyable<T> write_bytes(fstream& fout, const T& x)
    {
        fout.write(TO_BYTES_I(&(x)), sizeof(T));
    }

    // Address of an array is not guaranteed to be its start. Thus another overload is required that deals with arrays
    template<typename T, size_t N>
    inline enable_if_copyable<T> write_bytes(fstream& fout, const T (&x)[N])
    {
        fout.write(TO_BYTES_I(x), sizeof(T) * N);
    }

    // Unbounded array
    template<typename T>
    inline enable_if_copyable<T> write_bytes(fstream& fout, const T* x, size_t count)
    {
        fout.write(TO_BYTES_I(x), sizeof(T) * count);
    }

    inline void write_bytes(fstream& fout, const std::string& x)
    {
        auto len = x.length();
        write_bytes(fout, len);
        write_bytes(fout, x.c_str(), len);
    }

    template<typename T>
    inline enable_if_copyable<T, bool> read_bytes(fstream& fin, T& x)
    {
        fin.read(TO_BYTES_M(&(x)), sizeof(T));
        return !fin.fail();
    }

    template<typename T, size_t N>
    inline enable_if_copyable<T, bool> read_bytes(fstream& fin, T (&x)[N])
    {
        fin.read(TO_BYTES_M(x), sizeof(T) * N);
        return !fin.fail();
    }

    template<typename T>
    inline enable_if_copyable<T, bool> read_bytes(fstream& fin, T* x, size_t count)
    {
        fin.read(TO_BYTES_M(x), sizeof(T) * count);
        return !fin.fail();
    }

    inline bool read_bytes(fstream& fin, std::string& x)
    {
        int res = 1;
        decltype(x.length()) len;
        res &= read_bytes(fin, len);
        if (res)
        {
            static char buf[4096];
            res &= read_bytes(fin, buf, len);
            x.assign(buf, len);
        }
        return res;
    }

#undef TO_BYTES_I
#undef TO_BYTES_M

    /* --------------------------------------------------------------- */
    /* --------------------------- WRITING --------------------------- */
    /* --------------------------------------------------------------- */

    inline void WriteItem(DataFile f, const InventoryItem& item)
    {
        auto& fout = *f;
        write_bytes(fout, item.item_id);

        /* Meta */
        {
            // 1. Name
            write_bytes(fout, item.meta.name);

            // 2. Cat
            write_bytes(fout, item.meta.cat);
        }

        write_bytes(fout, item.item_count);
        write_bytes(fout, item.assigned_count);
        write_bytes(fout, item.active);
    }

    inline void WriteSingleMember(fstream& fout, const Member* mem)
    {
        write_bytes(fout, mem->name);
        write_bytes(fout, mem->borrow_count);
    }

    template<typename = void> /* Just to silence warning */
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

    template<typename = void>
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

        std::flush(*f);
    }

    /* --------------------------------------------------------------- */
    /* --------------------------- READING --------------------------- */
    /* --------------------------------------------------------------- */

    inline bool ValidateMagicBytes(fstream& fin)
    {
        using bytes_t = std::remove_cv<decltype(MAGIC_BYTES)>::type;
        bytes_t file_bytes;

        if (!read_bytes(fin, file_bytes))
            return false;

        return memcmp(file_bytes, MAGIC_BYTES, sizeof(file_bytes)) == 0;
    }

    inline bool IsFileValid(DataFile f)
    {
        return ValidateMagicBytes(*f);
    }

    inline item_count_t ReadCount(DataFile f)
    {
        item_count_t count;
        if (read_bytes(*f, count))
            return count;

        return -1;
    }

    template<typename = void>
    bool ReadItem(DataFile f, InventoryItem& item)
    {
        int res = 1;

        auto& fin = *f;

        res &= read_bytes(fin, item.item_id);

        /* Meta */
        {
            // 1. Name
            res &= read_bytes(fin, item.meta.name);

            // 2. Cat
            res &= read_bytes(fin, item.meta.cat);
        }
        res &= read_bytes(fin, item.item_count);
        res &= read_bytes(fin, item.assigned_count);
        res &= read_bytes(fin, item.active);

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

    template<typename = void>
    bool ReadFromFile(DataFile f, Inventory& inv)
    {
        auto count = ReadCount(f);
        if (count == -1)
            return false;

        if (count > 0)
        {
            inventory_allocate_capacity(inv, pow(2, ceil(log2(count))));

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
        }

        inv.count = count;

        return true;
    }

} // namespace Serialization

#endif