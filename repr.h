#pragma once

#ifndef __APP_REPR_H_
#define __APP_REPR_H_

#include <string>
#include <cstdint>
#include <cstring>

enum ItemCategory
{
    IC_INVALID,

    IC_STATIONARY,
    IC_MACHINERY,
    IC_ACCESSORY,
};

static constexpr std::size_t MAX_NAME_LEN = 24;

typedef char name_str_t[MAX_NAME_LEN];
typedef std::string name_str_new_t;

struct Member
{
    char name[MAX_NAME_LEN];
    int borrow_count = 0;

    Member* prev = nullptr;
    Member* next = nullptr;
};

typedef uint16_t item_id_t;
typedef uint32_t item_count_t;

struct ItemMeta
{
    // char name[MAX_NAME_LEN];
    std::string name;
    ItemCategory cat;
};

struct InventoryItem
{
    item_id_t item_id = 0;
    ItemMeta meta {};
    item_count_t item_count = 0;
    item_count_t assigned_count = 0;
    bool active = true;

    // Do not reorder allocated_to from being the last field
    Member* allocated_to = nullptr;
};

struct Inventory
{
    InventoryItem* items;
    uint32_t count = 0;
    uint32_t capacity;
};

inline constexpr uint32_t grow(uint32_t old)
{
    return old < 8 ? 8 : 2 * old;
}

inline Member* CreateMember(const char* name)
{
    Member* m = new Member;

    strcpy(m->name, name);
    m->borrow_count = 0;
    m->next = nullptr;
    m->prev = nullptr;

    return m;
}

inline void DeleteMember(Member* m)
{
    delete m;
}


void inventory_allocate_capacity(Inventory& inv, uint32_t capacity)
{
    if (capacity > inv.capacity)
    {
        auto old_list = inv.items;

        // inv.capacity = grow(inv.capacity);
        // inv.items = new InventoryItem[inv.capacity];
        auto new_list = inv.items = new InventoryItem[capacity];
        inv.capacity = capacity;

        if (inv.count > 0)
        {
            // memcpy(inv.items, old_list, sizeof(InventoryItem) * inv.count);
            for (int i = 0; i < inv.count; ++i)
                new_list[i] = std::move(old_list[i]);

            delete[] old_list;
        }
    }
}
#endif
