#pragma once

#ifndef __APP_REPR_H_
#define __APP_REPR_H_

#include <string>
#include <cstdint>

struct Member
{
    std::string name;
    int borrow_count = 0;

    Member* prev = nullptr;
    Member* next = nullptr;
};

typedef uint16_t item_id_t;
typedef uint32_t item_count_t;

struct ItemMeta
{
    std::string name;
    std::string cat;

    ItemMeta& operator= (const ItemMeta& other) = default;
    ItemMeta& operator= (ItemMeta&& other) = default;
};

struct InventoryItem
{
    item_id_t item_id = 0;
    ItemMeta meta {};
    item_count_t item_count = 0;
    item_count_t assigned_count = 0;
    bool active = true;

    Member* allocated_to = nullptr;

    InventoryItem& operator= (const InventoryItem& other) = default;
    InventoryItem& operator= (InventoryItem&& other) = default;
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

    m->name = name;
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

        auto new_list = inv.items = new InventoryItem[capacity];
        inv.capacity = capacity;

        if (inv.count > 0)
        {
            for (int i = 0; i < inv.count; ++i)
                new_list[i] = std::move(old_list[i]);

            delete[] old_list;
        }
    }
}
#endif
