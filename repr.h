#pragma once

#ifndef __APP_REPR_H_
#define __APP_REPR_H_

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
    char name[MAX_NAME_LEN];
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

// auto __trival = std::is_trivially_copyable<InventoryItem>::value;
// auto __sz = sizeof(InventoryItem);

struct Inventory
{
    InventoryItem* items;
    uint32_t count;
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


void InvUtil_AllocateFor(Inventory& inv, uint32_t capacity)
{
    if (capacity > inv.capacity)
    {
        auto old_list = inv.items;

        // inv.capacity = grow(inv.capacity);
        // inv.items = new InventoryItem[inv.capacity];
        inv.items = new InventoryItem[capacity];
        inv.capacity = capacity;

        if (inv.count > 0)
        {
            memcpy(inv.items, old_list, sizeof(InventoryItem) * inv.count);

            delete[] old_list;
        }
    }
}
#endif
